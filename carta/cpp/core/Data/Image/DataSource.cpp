#include "DataSource.h"
#include "Globals.h"
#include "MainConfig.h"
#include "PluginManager.h"
#include "CartaLib/IImage.h"
#include "Data/Util.h"
#include "CartaLib/Hooks/LoadAstroImage.h"
#include "CartaLib/Hooks/GetPersistentCache.h"
#include "CartaLib/Hooks/PercentileToPixelHook.h"
#include "CartaLib/IPCache.h"
#include "../../Algorithms/percentileAlgorithms.h"
#include <QDebug>
#include <QElapsedTimer>
#include "CartaLib/UtilCASA.h"
#include <zfp.h>
#include <cmath>
#include <QFuture>
#include <QtConcurrent>

using Carta::Lib::AxisInfo;
using Carta::Lib::AxisDisplayInfo;

#define SPECTRAL_PROGRESS_COMPLETE 1

namespace Carta {
namespace Data {

const QString DataSource::DATA_PATH = "file";
const QString DataSource::CLASS_NAME = "DataSource";
const bool DataSource::IS_MULTITHREAD_ZFP = true;
const int DataSource::MAX_SUBSETS = 8;

DataSource::DataSource() :
    m_image( nullptr ),
    m_permuteImage( nullptr),
    m_coordinateFormatter( nullptr ),
    m_axisIndexX( 0 ),
    m_axisIndexY( 1 ) {

    m_cmapCacheSize = 1000;

    // initialize disk cache
    auto res = Globals::instance()-> pluginManager()
                   -> prepare < Carta::Lib::Hooks::GetPersistentCache > ().first();

    if ( res.isNull() || ! res.val() ) {
        qWarning( "Could not find a disk cache plugin." );
        m_diskCache = nullptr;
        m_diskCacheHelper = nullptr;
    }
    else {
        m_diskCache = res.val();
        m_diskCacheHelper = std::make_shared<Carta::Lib::IntensityCacheHelper>(m_diskCache);
    }
}

std::vector<int> DataSource::_getPermOrder() const
{
    //Build a vector showing the permute order.
    int imageDim = m_image->dims().size();
    std::vector<int> indices( imageDim );
    indices[0] = m_axisIndexX;
    indices[1] = m_axisIndexY;
    int vectorIndex = 2;
    for ( int i = 0; i < imageDim; i++ ){
        if ( i != m_axisIndexX && i != m_axisIndexY ){
            indices[vectorIndex] = i;
            vectorIndex++;
        }
    }
    return indices;
}

int DataSource::_getFrameIndex( int sourceFrameIndex, const std::vector<int>& sourceFrames ) const {
    int frameIndex = 0;
    if (m_image ){
        AxisInfo::KnownType axisType = static_cast<AxisInfo::KnownType>( sourceFrameIndex );
        int axisIndex = Util::getAxisIndex( m_image, axisType );
        //The image doesn't have this particular axis.
        if ( axisIndex >=  0 ) {
            //The image has the axis so make the frame bounded by the image size.
            frameIndex = Carta::Lib::clamp( sourceFrames[sourceFrameIndex], 0, m_image-> dims()[axisIndex] - 1 );
        }
    }
    return frameIndex;
}

std::vector<int> DataSource::_fitFramesToImage( const std::vector<int>& sourceFrames ) const {
    int sourceFrameCount = sourceFrames.size();
    std::vector<int> outputFrames( sourceFrameCount );
    for ( int i = 0; i < sourceFrameCount; i++ ){
        outputFrames[i] = _getFrameIndex( i, sourceFrames );
    }
    return outputFrames;
}

std::vector<AxisInfo::KnownType> DataSource::_getAxisTypes() const {
    std::vector<AxisInfo::KnownType> types;
    casa_mutex.lock();
    CoordinateFormatterInterface::SharedPtr cf(
                   m_image-> metaData()-> coordinateFormatter()-> clone() );
    int axisCount = cf->nAxes();
    for ( int axis = 0 ; axis < axisCount; axis++ ) {
        const AxisInfo & axisInfo = cf-> axisInfo( axis );
        AxisInfo::KnownType axisType = axisInfo.knownType();
        if ( axisType != AxisInfo::KnownType::OTHER ){
            types.push_back( axisInfo.knownType() );
        }
    }
    casa_mutex.unlock();

    return types;
}

void DataSource::_setCoordinateSystem( Carta::Lib::KnownSkyCS cs ){
    casa_mutex.lock();
    m_coordinateFormatter->setSkyCS( cs );
    casa_mutex.unlock();
}

std::vector<AxisInfo> DataSource::_getAxisInfos() const {
    std::vector<AxisInfo> Infos;
    casa_mutex.lock();

    int axisCount = m_coordinateFormatter->nAxes();
    for ( int axis = 0 ; axis < axisCount; axis++ ) {
        const AxisInfo & axisInfo = m_coordinateFormatter-> axisInfo( axis );
        if ( axisInfo.knownType() != AxisInfo::KnownType::OTHER ){
            Infos.push_back( axisInfo );
        }
    }

    casa_mutex.unlock();

    return Infos;
}

AxisInfo::KnownType DataSource::_getAxisType( int index ) const {
    AxisInfo::KnownType type = AxisInfo::KnownType::OTHER;
    casa_mutex.lock();
    CoordinateFormatterInterface::SharedPtr cf(
                       m_image-> metaData()-> coordinateFormatter()-> clone() );
    int axisCount = cf->nAxes();
    if ( index < axisCount && index >= 0 ){
        AxisInfo axisInfo = cf->axisInfo( index );
        type = axisInfo.knownType();
    }
    casa_mutex.unlock();

    return type;
}

AxisInfo::KnownType DataSource::_getAxisXType() const {
    return _getAxisType( m_axisIndexX );
}

AxisInfo::KnownType DataSource::_getAxisYType() const {
    return _getAxisType( m_axisIndexY );
}

std::vector<AxisInfo::KnownType> DataSource::_getAxisZTypes() const {
    std::vector<AxisInfo::KnownType> zTypes;
    if ( m_image ){
        int imageDims = m_image->dims().size();
        for ( int i = 0; i < imageDims; i++ ){
            if ( i != m_axisIndexX && i!= m_axisIndexY ){
                AxisInfo::KnownType type = _getAxisType( i );
                if ( type != AxisInfo::KnownType::OTHER ){
                    zTypes.push_back( type );
                }
            }
        }
    }
    return zTypes;
}

QStringList DataSource::_getCoordinates( double x, double y,
        Carta::Lib::KnownSkyCS system, const std::vector<int>& frames ) const{
    std::vector<int> mFrames = _fitFramesToImage( frames );
    casa_mutex.lock();
    CoordinateFormatterInterface::SharedPtr cf( m_image-> metaData()-> coordinateFormatter()-> clone() );
    cf-> setSkyCS( system );
    int imageSize = m_image->dims().size();
    std::vector < double > pixel( imageSize, 0.0 );
    for ( int i = 0; i < imageSize; i++ ){
        if ( i == m_axisIndexX ){
            pixel[i] = x;
        }
        else if ( i == m_axisIndexY ){
            pixel[i] = y;
        }
        else {
            AxisInfo::KnownType axisType = _getAxisType( i );
            int axisIndex = static_cast<int>( axisType );
            pixel[i] = mFrames[axisIndex];
        }
    }
    QStringList list = cf-> formatFromPixelCoordinate( pixel );
    casa_mutex.unlock();
    return list;
}

std::vector<AxisDisplayInfo> DataSource::_getAxisDisplayInfo() const {
    std::vector<AxisDisplayInfo> axisInfo;
    //Note that permutations are 1-based whereas the axis
    //index is zero-based.
    if ( m_image ){
        int imageSize = m_image->dims().size();
        axisInfo.resize( imageSize );

        //Indicate the display axes by  putting -1 in for the display frames.
        //We will later fill in fixed frames for the other axes.
        axisInfo[m_axisIndexX].setFrame( -1 );
        axisInfo[m_axisIndexY].setFrame( -1 );

        //Indicate the new axis order.
        axisInfo[m_axisIndexX].setPermuteIndex( 0 );
        axisInfo[m_axisIndexY].setPermuteIndex( 1 );
        int availableIndex = 2;
        for ( int i = 0; i < imageSize; i++ ){
            axisInfo[i].setFrameCount( m_image->dims()[i] );
            axisInfo[i].setAxisType( _getAxisType( i ) );
            if ( i != m_axisIndexX && i != m_axisIndexY ){
                axisInfo[i].setPermuteIndex( availableIndex );
                availableIndex++;
            }
        }
    }
    return axisInfo;
}

int DataSource::_getFrameCount( AxisInfo::KnownType type ) const {
    int frameCount = 1;
    if ( m_image ){
        int axisIndex = Util::getAxisIndex( m_image, type );

        std::vector<int> imageShape  = m_image->dims();
        int imageDims = imageShape.size();
        if ( imageDims > axisIndex && axisIndex >= 0 ){
            frameCount = imageShape[axisIndex];
        }
    }
    return frameCount;
}

int DataSource::_getDimension( int coordIndex ) const {
    int dim = -1;
    if ( 0 <= coordIndex && coordIndex < _getDimensions()){
        dim = m_image-> dims()[coordIndex];
    }
    return dim;
}

int DataSource::_getDimensions() const {
    int imageSize = 0;
    if ( m_image ){
        imageSize = m_image->dims().size();
    }
    return imageSize;
}

std::pair<int,int> DataSource::_getDisplayDims() const {
    std::pair<int,int> displayDims(0,0);
    if ( m_image ){
        displayDims.first = m_image->dims()[m_axisIndexX];
        displayDims.second = m_image->dims()[m_axisIndexY ];
    }
    return displayDims;
}

QString DataSource::_getFileName() const {
    return m_fileName;
}

std::shared_ptr<Carta::Lib::Image::ImageInterface> DataSource::_getImage(){
    return m_image;
}

std::shared_ptr<Carta::Lib::Image::ImageInterface> DataSource::_getPermImage(){
    return m_permuteImage;
}

std::shared_ptr<Carta::Lib::IntensityValue> DataSource::_readIntensityCache(int frameLow, int frameHigh, double percentile, int stokeFrame, QString transformationLabel) const {
    if (m_diskCacheHelper) {
        return m_diskCacheHelper->get(m_fileName, frameLow, frameHigh, percentile, stokeFrame, transformationLabel);
    }
    return nullptr;
}

void DataSource::_setIntensityCache(double intensity, double error, int frameLow, int frameHigh, double percentile, int stokeFrame, QString transformationLabel) const {
    if (m_diskCacheHelper) {
        m_diskCacheHelper->set(m_fileName, intensity, error, frameLow, frameHigh, percentile, stokeFrame, transformationLabel);
    }
}

std::vector<double> DataSource::_getMinMaxIntensity(int frameLow, int frameHigh,
        const std::vector<double>& percentiles, int stokeFrame,
        Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {

    // Pick a calculator
    Carta::Lib::IPercentilesToPixels<double>::SharedPtr calculator = nullptr;

    calculator = std::make_shared<Carta::Core::Algorithms::MinMaxPercentiles<double> >();

    qDebug() << "++++++++ Chosen percentile calculator:" << calculator->label;

    std::vector<double> intensities(percentiles.size(), 0);
    std::vector<bool> found(percentiles.size(), false);
    size_t foundCount = 0;

    QString transformationLabel = converter ? converter->label : "NONE";

    // If the disk cache exists, try to look up cached intensity values

    for (size_t i = 0; i < percentiles.size(); i++) {
        std::shared_ptr<Carta::Lib::IntensityValue> cachedValue = _readIntensityCache(frameLow, frameHigh, percentiles[i], stokeFrame, transformationLabel);

        if (cachedValue /* this intensity cache exists */ &&
            cachedValue->error <= calculator->error /* already has an intensity error order smaller than the current choice */ ) {
            intensities[i] = cachedValue->value;

            qDebug() << "++++++++ Found percentile" << percentiles[i] << "in cache. Intensity:" << intensities[i] << "+/- (max-min)*" << cachedValue->error;

            // We only cache statistics for each distinct frame-dependent calculation
            // But we need to apply any constant multipliers
            if (converter) {
                intensities[i] = intensities[i] * converter->multiplier;
            }

            found[i] = true;
            foundCount++;
        }
    }

    // Not all percentiles were in the cache.  We are going to have to look some up.
    if (foundCount < percentiles.size()) {
        qDebug() << "++++++++ Calculating intensities for percentiles";

        Carta::Lib::NdArray::RawViewInterface* rawData = _getRawDataForStoke(frameLow, frameHigh, stokeFrame);

        qDebug() << "++++++++ Fetched raw image data for:" << "frameLow:" << frameLow << "frameHigh:" << frameHigh << "Stoke frame:" << stokeFrame;

        if (rawData == nullptr) {
            qCritical() << "Error: could not retrieve image data to calculate missing intensities.";
            return intensities;
        }

        // Create the view
        std::shared_ptr<Carta::Lib::NdArray::RawViewInterface> view(rawData);
        Carta::Lib::NdArray::Double doubleView(view.get(), false);

        // Make a list of the percentiles which have to be calculated
        std::vector<double> percentilesToCalculate;

        // Add the original missing percentiles
        for (size_t i = 0; i < percentiles.size(); i++) {
            if (!found[i]) {
                percentilesToCalculate.push_back(percentiles[i]);
            }
        }

        // Find Hz values if they are required for the unit transformation
        std::vector<double> hertzValues;

        // Calculate only the required percentiles
        std::map<double, double> clips_map;

        // perform the calculation on all of the percentiles
        int spectralIndex = Util::getAxisIndex( m_image, AxisInfo::KnownType::SPECTRAL );

        clips_map = calculator->percentile2pixels(doubleView, percentilesToCalculate, spectralIndex, converter, hertzValues);

        // add all the calculated values to the cache

        for (auto &m : clips_map) {
            // TODO: check what happens with the close values. Do we also need to cache the value with a different key, or does the serialisation unify them?
            // put calculated values in the disk cache if it exists
            _setIntensityCache(m.second, calculator->error, frameLow, frameHigh, m.first, stokeFrame, transformationLabel);

            qDebug() << "++++++++ [set cache] for percentile" << m.first << ", intensity=" << m.second << "+/- (max-min)*" << calculator->error;
        }

        // set return values (only the intensities which were requested)

        for (size_t i = 0; i < percentiles.size(); i++) {
            if (!found[i]) {
                intensities[i] = clips_map[percentiles[i]];
                found[i] = true; // for completeness, in case we test this later

                // apply any constant multiplier *after* caching the frame-dependent portion of the calculation
                if (converter) {
                    intensities[i] = intensities[i] * converter->multiplier;
                }
            }
        }
    }

    return intensities;
}

PBMSharedPtr DataSource::_getPixels2Histogram(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
    int numberOfBins,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {

    RegionHistogramData result = _getPixels2HistogramData(fileId, regionId, frameLow, frameHigh, stokeFrame,
                                                          numberOfBins, converter);

    // check if the result is valid
    if (result.bins.size() == 0) {
        return nullptr;
    }

    // add RegionHistogramData message
    std::shared_ptr<CARTA::RegionHistogramData> region_histogram_data(new CARTA::RegionHistogramData());
    region_histogram_data->set_file_id(result.fileId);
    region_histogram_data->set_region_id(result.regionId);
    region_histogram_data->set_stokes(result.stokeFrame);

    CARTA::Histogram* histogram = region_histogram_data->add_histograms();
    histogram->set_channel(result.frameLow);
    histogram->set_num_bins(result.num_bins);
    histogram->set_bin_width(result.bin_width);

    // the minimum value of pixels is the first bin center
    histogram->set_first_bin_center(result.first_bin_center);

    // fill in the vector of the histogram data
    for (auto intensity : result.bins) {
        histogram->add_bins(intensity);
    }

    return region_histogram_data;
}

RegionHistogramData DataSource::_getPixels2HistogramData(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
    int numberOfBins,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {

    qDebug() << "[DataSource] Calculating the regional histogram data...................................>";
    RegionHistogramData result; // results from the "percentileAlgorithms.h"

    // get the raw data
    Carta::Lib::NdArray::RawViewInterface* rawData = _getRawDataForStoke(frameLow, frameHigh, stokeFrame);
    if (rawData == nullptr) {
        qCritical() << "[DataSource] Error: could not retrieve image data to calculate missing intensities.";
        return result;
    }

    std::shared_ptr<Carta::Lib::NdArray::RawViewInterface> view(rawData);
    Carta::Lib::NdArray::Double doubleView(view.get(), false);

    // get the min/max intensities
    double minIntensity = 0.0;
    double maxIntensity = 0.0;
    std::vector<double> minMaxIntensities = _getMinMaxIntensity(frameLow, frameHigh, std::vector<double>({0, 1}), stokeFrame, converter);
    if (minMaxIntensities.size() != 2) {
        qCritical() << "[DataSource] Error: can not get the min/max intensities!!";
        return result;
    } else {
        minIntensity = minMaxIntensities[0];
        // assign the minimum of the pixel value as a private parameter
        maxIntensity = minMaxIntensities[1];
    }

    if (minIntensity > maxIntensity) {
        qCritical() << "[DataSource] Error: min intensity > max intensity!!";
        return result;
    }

    // get the calculator
    Carta::Lib::IPercentilesToPixels<double>::SharedPtr calculator = nullptr;
    calculator = std::make_shared<Carta::Core::Algorithms::MinMaxPercentiles<double> >();

    // Find Hz values if they are required for the unit transformation
    std::vector<double> hertzValues;
//    if (converter && converter->frameDependent) {
//        hertzValues = _getHertzValues(doubleView.dims());
//    }

    int spectralIndex = Util::getAxisIndex( m_image, AxisInfo::KnownType::SPECTRAL );
    result = calculator->pixels2histogram(fileId, regionId, doubleView, minIntensity, maxIntensity,
                                          numberOfBins, spectralIndex, converter, hertzValues, frameLow, stokeFrame);

    qDebug() << "[DataSource] .......................................................................Done";

    return result;
}

PBMSharedPtr DataSource::_getRasterImageData(int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
    int frameLow, int frameHigh, int stokeFrame,
    bool isZFP, int precision, int numSubsets,
    bool &changeFrame, int regionId, int numberOfBins,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {

    std::vector<float> imageData; // the image raw data with downsampling

    // start timer for computing approximate percentiles
    QElapsedTimer timer;
    timer.start();

    // get the raw data
    Carta::Lib::NdArray::RawViewInterface* view = _getRawDataForStoke(frameLow, frameHigh, stokeFrame);

    // check if the downsampling parameter "mip" is smaller than the image width or high
    if (mip <= 0 || abs(mip) > std::min(view->dims()[0], view->dims()[1])) {
        qWarning() << "[DataSource] Downsampling parameter, mip=" << mip
                   << ", which is larger than the image width=" <<  view->dims()[0]
                   << "or high=" << view->dims()[1] << ". Return nullptr";
        return nullptr;
        // [Try] it may be due to the frontend signal problem, reset the mip as 1 to pass, and then resend the next correct signal
        //mip = 1;
    }

    qDebug() << "[DataSource] Down sampling the raster image data.......................................>";
    //qDebug() << "Dawn sampling factor mip:" << mip;
    int nx = (xMax - xMin) / mip;
    int ny = (yMax - yMin) / mip;
    //qDebug() << "get the x-pixel-coordinate range: [x_min, x_max]= [" << xMin << "," << xMax << "]" << "--> W=" << nx;
    //qDebug() << "get the y-pixel-coordinate range: [y_min, y_max]= [" << yMin << "," << yMax << "]" << "--> H=" << ny;

    int prepareCols = view->dims()[0]; // get the full width length
    int prepareRows = mip;
    int area = prepareCols * prepareRows;
    std::vector<float> prepareArea(area);

    int nRows = (yMax - yMin) / mip;
    int nCols = (xMax - xMin) / mip;
    int nextRowToReadIn = yMin;

    auto updateRows = [&]() -> void {
        CARTA_ASSERT(nextRowToReadIn < view->dims()[1]); // check if the row index is beyond the length of the image high

        SliceND rowSlice;
        rowSlice.next().start( nextRowToReadIn ).end( nextRowToReadIn + prepareRows );
        auto rawRowView = view -> getView( rowSlice );

        // make a float view of this raw row view
        Carta::Lib::NdArray::Float fview( rawRowView, true );

        int t = 0;
        fview.forEach( [&] ( const float & val ) {
            // To improve the performance, the prepareArea also update only one row by computing the module
            prepareArea[(t++)] = val;
        });

        if (t != area) {
            qDebug() << "The prepared length of the raw data array:" << area
                     << "=" << prepareCols << "X" << prepareRows
                     << ", which is not consistent with the slice cut:" << t << "!!";
            qFatal("The prepared length of the raw data array is not consistent with the slice cut!!");
        }

        // Calculate the mean of each block (mip X mip)
        for (int i = 0; i < nCols; i++) {
            float rawData = 0;
            int elems = mip * mip;
            float denominator = elems;
            for (int e = 0; e < elems; e++) {
                int row = e / mip;
                int col = e % mip;
                int index = ((row * prepareCols) + (col + (xMin + (i * mip))));
                if (std::isfinite(prepareArea[index])) {
                    rawData += prepareArea[index];
                } else {
                    denominator -= 1;
                }
            }
            // set the NaN type of the pixel as the minimum of the other finite pixel values
            rawData = (denominator < 1 ? NAN : rawData / denominator);
            imageData.push_back(rawData);
        }
        nextRowToReadIn += prepareRows;
    };

    // scan the raw data for with rows for down sampling
    for (int j = 0; j < nRows; j++) {
        updateRows();
    }

    // add the RasterImageData message
    CARTA::ImageBounds* imgBounds = new CARTA::ImageBounds();
    imgBounds->set_x_min(xMin);
    imgBounds->set_x_max(xMax);
    imgBounds->set_y_min(yMin);
    imgBounds->set_y_max(yMax);

    std::shared_ptr<CARTA::RasterImageData> raster(new CARTA::RasterImageData());
    raster->set_file_id(fileId);
    raster->set_allocated_image_bounds(imgBounds);
    raster->set_channel(frameLow);
    raster->set_stokes(stokeFrame);
    raster->set_mip(mip);

    if (isZFP) {
        // use ZFP compression
        raster->set_compression_type(CARTA::CompressionType::ZFP);
        raster->set_compression_quality(precision);

        if (IS_MULTITHREAD_ZFP) {
            // ZFP compression with multi-thread
            int N = std::min(numSubsets, MAX_SUBSETS); // total number of threads
            std::vector<size_t> compressedSizes(N);
            std::vector<std::vector<int32_t> > nanEncodings(N);
            std::vector<std::vector<char> > compressionBuffers(N);
            std::vector<int> status(N);
            std::vector<QFuture<void> > futures;

            for (int i = 0; i < N; i++) {
                auto &compressionBuffer = compressionBuffers[i];
                auto &compressedSize = compressedSizes[i];

                QFuture<void> future = QtConcurrent::run(
                    [&nanEncodings, &imageData, &compressionBuffer, &compressedSize, &status, i,
                        nx, ny, N, precision, this]() {

                    int subsetRowStart = i * (ny / N);
                    int subsetRowEnd = (i + 1) * (ny / N);

                    if (i == N - 1) {
                        subsetRowEnd = ny;
                    }

                    int subsetElementStart = subsetRowStart * nx;

                    nanEncodings[i] = this->_getNanEncodingsBlock(imageData, subsetElementStart, nx, subsetRowEnd - subsetRowStart);
                    status[i] = this->_compress(imageData, subsetElementStart, compressionBuffer, compressedSize,
                                                nx, subsetRowEnd - subsetRowStart, precision);
                });

                futures.push_back(future);
            }

            // Wait for completed compression threads
            for (auto future : futures) {
                // Note that if the future finished BEFORE we call this, it will still work.
                future.waitForFinished();
            }

            // Complete the message
            for (int i = 0; i < N; i++) {
                raster->add_image_data(compressionBuffers[i].data(), compressedSizes[i]);
                raster->add_nan_encodings((char*) nanEncodings[i].data(), nanEncodings[i].size() * sizeof(int));
            }

            qDebug() << "[DataSource] Apply ZFP compression (status=" << status << ", precision=" << precision
                     << ", number of subsets=" << N << ", NaN encodings size=" << nanEncodings.size() << ")";
        } else {
            // ZFP compression with single thread
            std::vector<char> compressionBuffer;
            size_t compressedSize; // use "vector<size_t> compressedSizes(numSubsets);" for multi-thread calculations

            // get NaN type pixel distances of indices
            std::vector<int32_t> nanEncodings = _getNanEncodingsBlock(imageData, 0, nx, ny);

            // apply ZFP function
            int status = _compress(imageData, 0, compressionBuffer, compressedSize, nx, ny, precision);

            // use "raster->add_image_data(compressionBuffers[i].data(), compressedSizes[i])" for multi-thread calculations
            raster->add_image_data(compressionBuffer.data(), compressedSize);
            raster->add_nan_encodings((char*) nanEncodings.data(), nanEncodings.size() * sizeof(int)); // This item is necessary !!

            qDebug() << "[DataSource] Apply ZFP compression (status=" << status << ", precision=" << precision
                     << ", number of subsets= 1" << ", NaN encodings size=" << nanEncodings.size() << ")";
        }
    } else {
        // without compression
        raster->set_compression_type(CARTA::CompressionType::NONE);
        raster->set_compression_quality(0);
        raster->add_image_data(imageData.data(), imageData.size() * sizeof(float));
        qDebug() << "[DataSource] w/o ZFP compression!";
    }

    //qDebug() << "number of the raw data sent L=" << imageData.size() << ", WxH=" << nx * ny << ", Difference:" << (nx * ny - imageData.size());

    // end of timer for loading the raw data
    int elapsedTime = timer.elapsed();
    if (CARTA_RUNTIME_CHECKS) {
        qCritical() << "<> Time to get raster image data:" << elapsedTime << "ms";
    }

    qDebug() << "[DataSource] .......................................................................Done";

    // check if need to calculate the histogram data
    if (changeFrame) {
        RegionHistogramData result = _getPixels2HistogramData(fileId, regionId, frameLow, frameHigh, stokeFrame,
                                                              numberOfBins, converter);
        // check if the calculation result is valid
        if (result.bins.size() > 0) {
            // add RegionHistogramData in the RasterImageData message
            CARTA::RegionHistogramData* region_histogram_data = new CARTA::RegionHistogramData();
            region_histogram_data->set_file_id(result.fileId);
            region_histogram_data->set_region_id(result.regionId);
            region_histogram_data->set_stokes(result.stokeFrame);

            CARTA::Histogram* histogram = region_histogram_data->add_histograms();
            histogram->set_channel(result.frameLow);
            histogram->set_num_bins(result.num_bins);
            histogram->set_bin_width(result.bin_width);

            // the minimum value of pixels is the first bin center
            histogram->set_first_bin_center(result.first_bin_center);

            // fill in the vector of the histogram data
            for (auto intensity : result.bins) {
                histogram->add_bins(intensity);
            }
            raster->set_allocated_channel_histogram_data(region_histogram_data);
        }
        // reset the m_changeFrame[fileId] = false; in the NewServerConnector obj
        changeFrame = false;
    }

    return raster;
}

PBMSharedPtr DataSource::_getXYProfiles(int fileId, int x, int y,
    int frameLow, int frameHigh, int stokeFrame,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {

    qDebug() << "[DataSource] Get X/Y profiles...................................>";

    std::vector<float> xProfile, yProfile;

    // get the raw data of image
    Carta::Lib::NdArray::RawViewInterface* rawData = _getRawDataForStoke(frameLow, frameHigh, stokeFrame);
    if (rawData == nullptr) {
        qCritical() << "[DataSource] Error: could not retrieve image data to get X/Y profiles.";
        return nullptr;
    }
    std::shared_ptr<Carta::Lib::NdArray::RawViewInterface> view(rawData);
    Carta::Lib::NdArray::Double doubleView(view.get(), false);
    const int imgWidth = view->dims()[0];
    const int imgHeight = view->dims()[1];
//    int spectralIndex = Util::getAxisIndex(m_image, AxisInfo::KnownType::SPECTRAL);

    // start timer for computing X/Y profiles
    QElapsedTimer timer;
    timer.start();

    // TODO: need to check the spatial profile to get the corresponding spatial data
    // now only get 'x', 'y' no matter what the spatial profiles are specified

    if (converter && converter->frameDependent) {
        // Find Hz values if they are required for the unit transformation
//        std::vector<double> hertzValues = _getHertzValues(doubleView.dims());
//        for (size_t f = 0; f < hertzValues.size(); f++) {
//            double hertzVal = hertzValues[f];
//            Carta::Lib::NdArray::Double viewSlice = Carta::Lib::viewSliceForFrame(doubleView, spectralIndex, f);
//            _getXYProfiles(doubleView, imgWidth, imgHeight, x, y, xProfile, yProfile);
//        }
    } else {
        _getXYProfiles(doubleView, imgWidth, imgHeight, x, y, xProfile, yProfile);
    }

    // end of timer for computing X/Y profiles
    int elapsedTime = timer.elapsed();
    if (CARTA_RUNTIME_CHECKS) {
        qCritical() << "<> Time to get X/Y profiles:" << elapsedTime << "ms";
    }

    // create spatial profile data
    std::shared_ptr<CARTA::SpatialProfileData> spatialProfileData(new CARTA::SpatialProfileData());
    spatialProfileData->set_file_id(fileId);
    spatialProfileData->set_region_id(0);
    spatialProfileData->set_x(x);
    spatialProfileData->set_y(y);
    spatialProfileData->set_channel(frameLow);
    spatialProfileData->set_stokes(stokeFrame);
    if (xProfile.size() > x) {
        spatialProfileData->set_value(xProfile[x]);
    } else if (yProfile.size() > y) {
        spatialProfileData->set_value(yProfile[y]);
    }

    // Add X/Y profiles to spatial profile data
    if (false == _addProfile(spatialProfileData, xProfile, "x")) {
        qDebug() << "Add X profile to spatial profile data failed.";
        return nullptr;
    }
    if (false == _addProfile(spatialProfileData, yProfile, "y")) {
        qDebug() << "Add Y profile to spatial profile data failed.";
        return nullptr;
    }

    qDebug() << "[DataSource] .......................................................................Done";

    return spatialProfileData;
}

void DataSource::_getXYProfiles(Carta::Lib::NdArray::Double doubleView, const int imgWidth, const int imgHeight,
    const int x, const int y, std::vector<float> & xProfile, std::vector<float> & yProfile) const {

    // get X profile
    for (int index = 0; index < imgWidth; index++) {
        float val = (float)doubleView.get({index,y});
        std::isfinite(val) ? xProfile.push_back(val) : xProfile.push_back(NAN); // replace infinite with NaN
    }

    // get Y profile
    for (int index = 0; index < imgHeight; index++) {
        float val = (float)doubleView.get({x,index});
        std::isfinite(val) ? yProfile.push_back(val) : yProfile.push_back(NAN); // replace infinite with NaN
    }
}

bool DataSource::_addProfile(std::shared_ptr<CARTA::SpatialProfileData> spatialProfileData,
    const std::vector<float> & profile, const std::string coordinate) const {
    if(nullptr == spatialProfileData) {
        qDebug() << "Spatial profile data is null.";
        return false;
    }

    CARTA::SpatialProfile* spatialProfile = spatialProfileData->add_profiles();
    if (nullptr == spatialProfile) {
        qDebug() << "Add spatial profile to spatial profile data error.";
        return false;
    }

    spatialProfile->set_start(0);
    spatialProfile->set_end(profile.size() - 1);
    for (auto val = profile.begin(); val != profile.end(); val++) {
        spatialProfile->add_values(*val);
    }
    spatialProfile->set_coordinate(coordinate);

    return true;
}

// This function is provided by Angus
int DataSource::_compress(std::vector<float> &array, size_t offset, std::vector<char> &compressionBuffer,
    size_t &compressedSize, uint32_t nx, uint32_t ny, uint32_t precision) const {

    int status = 0;    /* return value: 0 = success */
    zfp_type type;     /* array scalar type */
    zfp_field* field;  /* array meta data */
    zfp_stream* zfp;   /* compressed stream */
    size_t bufsize;    /* byte size of compressed buffer */
    bitstream* stream; /* bit stream to write to or read from */

    type = zfp_type_float;
    field = zfp_field_2d(array.data() + offset, type, nx, ny);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(nullptr);

    /* set compression mode and parameters via one of three functions */
    zfp_stream_set_precision(zfp, precision);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp, field);
    if (compressionBuffer.size() < bufsize) {
        compressionBuffer.resize(bufsize);
    }
    stream = stream_open(compressionBuffer.data(), bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    compressedSize = zfp_compress(zfp, field);
    if (!compressedSize) {
        status = 1;
    }

    /* clean up */
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    return status;
}

// This function is provided by Angus
std::vector<int32_t> DataSource::_getNanEncodingsBlock(std::vector<float>& array, int offset, int w, int h) const {
    // Generate RLE NaN list
    int length = w * h;
    int32_t prevIndex = offset;
    bool prev = false;
    std::vector<int32_t> encodedArray;

    for (auto i = offset; i < offset + length; i++) {
        bool current = std::isnan(array[i]);
        if (current != prev) {
            encodedArray.push_back(i - prevIndex);
            prevIndex = i;
            prev = current;
        }
    }
    encodedArray.push_back(offset + length - prevIndex);

    // Skip all-NaN images and NaN-free images
    if (encodedArray.size() > 1) {
        // Calculate average of 4x4 blocks (matching blocks used in ZFP), and replace NaNs with block average
        for (auto i = 0; i < w; i += 4) {
            for (auto j = 0; j < h; j += 4) {
                int blockStart = offset + j * w + i;
                int validCount = 0;
                float sum = 0;
                // Limit the block size when at the edges of the image
                int blockWidth = std::min(4, w - i);
                int blockHeight = std::min(4, h - j);
                for (int x = 0; x < blockWidth; x++) {
                    for (int y = 0; y < blockHeight; y++) {
                        float v = array[blockStart + (y * w) + x];
                        if (!std::isnan(v)) {
                            validCount++;
                            sum += v;
                        }
                    } // end of blockHeight loop
                } // end of blockWidth loop

                // Only process blocks which have at least one valid value AND at least one NaN. All-NaN blocks won't affect ZFP compression
                if (validCount && validCount != blockWidth * blockHeight) {
                    float average = sum / validCount;
                    for (int x = 0; x < blockWidth; x++) {
                        for (int y = 0; y < blockHeight; y++) {
                            float v = array[blockStart + (y * w) + x];
                            if (std::isnan(v)) {
                                array[blockStart + (y * w) + x] = average;
                            }
                        } // end of blockHeight loop
                    } // end of blockWidth loop
                } // check whether if validCount > 0 && validCount != blockWidth * blockHeight
            } // end of j loop
        } // end of i loop
    }
    return encodedArray;
}

QPointF DataSource::_getPixelCoordinates( double ra, double dec, bool* valid ) const{
    QPointF result;
    casa_mutex.lock();
    CoordinateFormatterInterface::SharedPtr cf( m_image-> metaData()-> coordinateFormatter()-> clone() );
    const CoordinateFormatterInterface::VD world { ra, dec };
    CoordinateFormatterInterface::VD pixel;
    *valid = cf->toPixel( world, pixel );
    if ( *valid ){
        result = QPointF( pixel[0], pixel[1]);
    }
    casa_mutex.unlock();

    return result;
}

std::pair<double,QString> DataSource::_getRestFrequency() const {
	std::pair<double,QString> restFreq( -1, "");
	if ( m_image ){
		restFreq = m_image->metaData()->getRestFrequency();
	}
	return restFreq;
}

QPointF DataSource::_getWorldCoordinates( double pixelX, double pixelY,
        Carta::Lib::KnownSkyCS coordSys, bool* valid ) const{
    QPointF result;
    casa_mutex.lock();

    CoordinateFormatterInterface::SharedPtr cf( m_image-> metaData()-> coordinateFormatter()-> clone() );
    cf->setSkyCS( coordSys );
    int imageDims = _getDimensions();
    std::vector<double> pixel( imageDims );
    pixel[0] = pixelX;
    pixel[1] = pixelY;
    CoordinateFormatterInterface::VD world;
    *valid = cf->toWorld( pixel, world );
    if ( *valid ){
        result = QPointF( world[0], world[1]);
    }
    casa_mutex.unlock();

    return result;
}

QString DataSource::_getPixelUnits() const {
    QString units = m_image->getPixelUnit().toStr();
    return units;
}

Carta::Lib::NdArray::RawViewInterface* DataSource::_getRawData( int frameStart, int frameEnd, int axisIndex ) const {
    Carta::Lib::NdArray::RawViewInterface* rawData = nullptr;
    if ( m_image ){
        // get the image dimension:
        // if the image dimension=3, then dim[0]: x-axis, dim[1]: y-axis, and dim[2]: channel-axis
        // if the image dimension=4, then dim[0]: x-axis, dim[1]: y-axis, dim[2]: stoke-axis,   and dim[3]: channel-axis
        //                                                            or  dim[2]: channel-axis, and dim[3]: stoke-axis
        int imageDim =m_image->dims().size();

        SliceND frameSlice = SliceND().next();

        for ( int i = 0; i < imageDim; i++ ){

            // only deal with the extra dimensions other than x-axis and y-axis
            if ( i != m_axisIndexX && i != m_axisIndexY ){

                // declare and set the variable "sliceSize" as the total number of channel or stoke
                int sliceSize = m_image->dims()[i];
                qDebug() << "++++++++ For the image dimension: dim[" << i << "], the total number of slices is " << sliceSize;

                SliceND& slice = frameSlice.next();

                //If it is the target axis,
                if ( i == axisIndex ){
                   //Use the passed in frame range
                   if (0 <= frameStart && frameStart < sliceSize &&
                        0 <= frameEnd && frameEnd < sliceSize ){
                        slice.start( frameStart );
                        slice.end( frameEnd + 1);
                   }
                   else {
                       slice.start(0);
                       slice.end( sliceSize);
                   }
                }
                //Or the entire range
                else {
                   slice.start( 0 );
                   slice.end( sliceSize );
                }
                slice.step( 1 );
            }
        }
        rawData = m_image->getDataSlice( frameSlice );
    }
    return rawData;
}

int DataSource::_getStokeIndicator() {
    int result = Util::getAxisIndex(m_image, AxisInfo::KnownType::STOKES);
    return result;
}

int DataSource::_getSpectralIndicator() {
    int result = Util::getAxisIndex(m_image, AxisInfo::KnownType::SPECTRAL);
    return result;
}

Carta::Lib::NdArray::RawViewInterface* DataSource::_getRawDataForStoke( int frameStart, int frameEnd, int stokeFrame ) const {

    Carta::Lib::NdArray::RawViewInterface* rawData = nullptr;
    int spectralIndex = Util::getAxisIndex( m_image, AxisInfo::KnownType::SPECTRAL );
    int stokeIndex = Util::getAxisIndex( m_image, AxisInfo::KnownType::STOKES );

    if ( m_image ){
        // get the image dimension:
        // if the image dimension=3, then dim[0]: x-axis, dim[1]: y-axis, and dim[2]: channel-axis
        // if the image dimension=4, then dim[0]: x-axis, dim[1]: y-axis, dim[2]: stoke-axis, and dim[3]: channel-axis
        //                                                            or  dim[2]: channel-axis, and dim[3]: stoke-axis
        int imageDim =m_image->dims().size();
        //qDebug() << "++++++++ Dimension of image raw data=" << imageDim;

        SliceND frameSlice = SliceND().next();

        for ( int i = 0; i < imageDim; i++ ){

            // only deal with the extra dimensions other than x-axis and y-axis
            if ( i != m_axisIndexX && i != m_axisIndexY ){

                // get the number of slice (e.q. channel) in this dimension
                int sliceSize = m_image->dims()[i];
                SliceND& slice = frameSlice.next();

                // If it is the target axis..
                if (i == spectralIndex) {
                   // Use the passed in frame range
                   if (0 <= frameStart && frameStart < sliceSize &&
                       0 <= frameEnd && frameEnd < sliceSize) {
                       slice.start(frameStart);
                       slice.end(frameEnd + 1);
                       qDebug() << "++++++++ Spectral axis index=" << i
                                << ", get the channel range= [" << frameStart << "," << frameEnd << "]";
                   } else {
                       qDebug() << "++++++++ Spectral axis index=" << i
                                << ", get the channel range= [0 ," << sliceSize << "]";
                       slice.start(0);
                       slice.end(sliceSize);
                   }
                } else if (i == stokeIndex) {
                    if (stokeFrame >= 0 && stokeFrame <= 3) {
                        // we only consider one stoke (stokeSliceIndex) for percentile calculation
                        qDebug() << "++++++++ Stoke axis index=" << i << ", get the channel" << stokeFrame <<
                                    "(-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)" ;
                        slice.start(stokeFrame);
                        slice.end(stokeFrame + 1);
                    } else {
                        // if the stoke-axis is exist (axisStokeIndex != -1),
                        qDebug() << "++++++++ Stoke axis index=" << i
                                 << ", get the channel range= [0 ," << sliceSize << "]";
                        slice.start(0);
                        slice.end(sliceSize);
                    }
                } else {
                    // for the other axis, assume it is a spectral frame
                    if (0 <= frameStart && frameStart < sliceSize &&
                        0 <= frameEnd && frameEnd < sliceSize) {
                        slice.start(frameStart);
                        slice.end(frameEnd + 1);
                        qDebug() << "++++++++ find the other axis index=" << i
                                 << ", get the channel range= [" << frameStart << "," << frameEnd << "]";
                    } else {
                        qDebug() << "++++++++ find the other axis index=" << i
                                 << ", get the channel range= [0 ," << sliceSize << "]";
                        slice.start(0);
                        slice.end(sliceSize);
                    }
                }

                slice.step( 1 );
            }
        }
        rawData = m_image->getDataSlice( frameSlice );
    }
    return rawData;
}

std::shared_ptr<Carta::Lib::Image::ImageInterface> DataSource::_getPermutedImage() const {
    std::shared_ptr<Carta::Lib::Image::ImageInterface> permuteImage(nullptr);
    if ( m_image ){
        //Build a vector showing the permute order.
        std::vector<int> indices = _getPermOrder();
        permuteImage = m_image->getPermuted( indices );
    }
    return permuteImage;
}

Carta::Lib::NdArray::RawViewInterface* DataSource::_getRawData( const std::vector<int> frames ) const {

    Carta::Lib::NdArray::RawViewInterface* rawData = nullptr;
    std::vector<int> mFrames = _fitFramesToImage( frames );

    if ( m_permuteImage ){
        int imageDim =m_permuteImage->dims().size();

        //Build a vector showing the permute order.
        std::vector<int> indices = _getPermOrder();

        SliceND nextSlice = SliceND();
        SliceND& slice = nextSlice;

        for ( int i = 0; i < imageDim; i++ ){

            //Since the image has been permuted the first two indices represent the display axes.
            if ( i != 0 && i != 1 ){

                //Take a slice at the indicated frame.
                int frameIndex = 0;
                int thisAxis = indices[i];
                AxisInfo::KnownType type = _getAxisType( thisAxis );

                // check the type of axis and its indix of slices
                int axisIndex = -1;
                if ( type == AxisInfo::KnownType::SPECTRAL ) {
                    axisIndex = static_cast<int>( type );
                    frameIndex = mFrames[axisIndex];
                    // qDebug() << "++++++++ SPECTRAL axis Index with permutation is" << axisIndex << ", the current frame Index is" << frameIndex;
                } else if ( type == AxisInfo::KnownType::STOKES ) {
                    axisIndex = static_cast<int>( type );
                    frameIndex = mFrames[axisIndex];
                    // qDebug() << "++++++++ STOKE axis Index with permutation is" << axisIndex << ", the current frame Index is" << frameIndex;
                } else if ( type != AxisInfo::KnownType::OTHER ) {
                    axisIndex = static_cast<int>( type );
                    frameIndex = mFrames[axisIndex];
                    // qDebug() << "++++++++ axis Index with permutation is" << axisIndex << ", the current frame Index is" << frameIndex;
                }

                slice.start( frameIndex );
                slice.end( frameIndex + 1);
            }

            if ( i < imageDim - 1 ){
                slice.next();
            }
        }
        rawData = m_permuteImage->getDataSlice( nextSlice );
    }
    return rawData;
}

QString DataSource::_getViewIdCurrent( const std::vector<int>& frames ) const {
   // We create an identifier consisting of the file name and -1 for the two display axes
   // and frame indices for the other axes.
   QString renderId = m_fileName;
   if ( m_image ){
       int imageSize = m_image->dims().size();
       for ( int i = 0; i < imageSize; i++ ){
           AxisInfo::KnownType axisType = _getAxisType( i );
           int axisFrame = frames[static_cast<int>(axisType)];
           QString prefix;
           //Hidden axis identified with an "f" and the index of the frame.
           if ( i != m_axisIndexX && i != m_axisIndexY ){
               prefix = "h";
           }
           //Display axis identified by a "d" plus the actual axis in the image.
           else {
               if ( i == m_axisIndexX ){
                   prefix = "dX";
               }
               else {
                   prefix = "dY";
               }
               axisFrame = i;
           }
           renderId = renderId + "//"+prefix + QString::number(axisFrame );
       }
   }
   return renderId;
}

bool DataSource::_isLoadable( std::vector<int> frames ) const {
        int imageDim =m_image->dims().size();
	bool loadable = true;
	for ( int i = 0; i < imageDim; i++ ){
		AxisInfo::KnownType type = _getAxisType( i );
		if ( AxisInfo::KnownType::OTHER != type ){
			int axisIndex = static_cast<int>( type );
			int frameIndex = frames[axisIndex];
                        int frameCount = m_image->dims()[i];
			if ( frameIndex >= frameCount ){
				loadable = false;
				break;
			}
		}
		else {
			loadable = false;
			break;
		}
	}
	return loadable;
}

bool DataSource::_isSpectralAxis() const {
	bool spectralAxis = false;
	int imageSize = m_image->dims().size();
	for ( int i = 0; i < imageSize; i++ ){
		AxisInfo::KnownType axisType = _getAxisType( i );
		if ( axisType == AxisInfo::KnownType::SPECTRAL ){
			spectralAxis = true;
			break;
		}
	}
	return spectralAxis;
}

QString DataSource::_setFileName( const QString& fileName, bool* success ){
    QString file = fileName.trimmed();
    *success = true;
    QString result;
    if (file.length() > 0) {
        if ( file != m_fileName ){
            try {
                auto res = Globals::instance()-> pluginManager()
                                      -> prepare <Carta::Lib::Hooks::LoadAstroImage>( file )
                                      .first();
                if (!res.isNull()){
                    m_image = res.val();
                    m_permuteImage = m_image;
                    std::shared_ptr<CoordinateFormatterInterface> cf(
                        m_image->metaData()->coordinateFormatter()->clone() );
                    m_coordinateFormatter = cf;
                    m_fileName = file;
                    //qDebug() << "[DataSource] m_fileName=" << m_fileName;
                }
                else {
                    result = "Could not find any plugin to load image";
                    qWarning() << result;
                    *success = false;
                }

            }
            catch( std::logic_error& err ){
                result = "Failed to load image "+fileName;
                qDebug() << result;
                *success = false;
            }
        }
    }
    else {
        result = "Could not load empty file.";
        *success = false;
    }
    return result;
}

bool DataSource::_setDisplayAxis( AxisInfo::KnownType axisType, int* axisIndex ){
    bool displayAxisChanged = false;
    if ( m_image ){
        int newXAxisIndex = Util::getAxisIndex(m_image, axisType);

        // invalid and let caller handle this case
        if (newXAxisIndex<0) {
            *axisIndex = newXAxisIndex;
             displayAxisChanged = true;
        }

        int imageSize = m_image->dims().size();
        if ( newXAxisIndex >= 0 && newXAxisIndex < imageSize ){
            if ( newXAxisIndex != *axisIndex ){
                *axisIndex = newXAxisIndex;
                displayAxisChanged = true;
            }
        }
    }
    return displayAxisChanged;
}

bool DataSource::_setSpatialRequirements(int fileId, int regionId,
    google::protobuf::RepeatedPtrField<std::string> spatialProfiles) {

    // TODO: need to store spatial profile to m_profileInfo &
    // get corresponding spatial data by checking the spatial profiles

    return true;
}

bool DataSource::_setSpectralRequirements(int fileId, int regionId, int stokeFrame,
    google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles) {

    // TODO: need to modify the way to get corresponding spectral data by checking the spatial profiles

    m_profileInfo.setStokesFrame(stokeFrame);

    return true;
}

PBMSharedPtr DataSource::_getSpectralProfile(int fileId, int x, int y, int stoke) {

    qDebug() << "[DataSource] Get spectral profile...................................>";

    // start timer for computing Z profile
    QElapsedTimer timer;
    timer.start();

    // TODO: need to check the spectral profile to get the corresponding spectral data
    // now only get 'z' no matter what the spectral profile is specified

    auto result = Globals::instance()->pluginManager()
        -> prepare <Carta::Lib::Hooks::ProfileHook>(m_image, nullptr/*region info (nullptr is for all region)*/,
                                                    x, y, m_profileInfo);
    auto lam = [=] (const Carta::Lib::Hooks::ProfileResult &data) {
        m_profileResult = data;
    };

    try {
        result.forEach(lam);
    }
    catch (char*& error) {
        qDebug() << "[DataSource] ProfileRenderWorker::run: caught error: " << error;
        m_profileResult.setError( QString(error) );
    }

    // pair(first, second): first for channel_vals[](skipped), second for spectral profile
    std::vector< std::pair<double,double> > profileData = m_profileResult.getData();

    // create spectral profile data & generate protobuf message
    std::shared_ptr<CARTA::SpectralProfileData> spectralProfileData(new CARTA::SpectralProfileData());
    spectralProfileData->set_file_id(fileId);
    spectralProfileData->set_region_id(0);
    spectralProfileData->set_stokes(m_profileInfo.getStokesFrame());
    spectralProfileData->set_progress(SPECTRAL_PROGRESS_COMPLETE);

    CARTA::SpectralProfile* spectralProfile = spectralProfileData->add_profiles();
    if (nullptr == spectralProfile) {
        qDebug() << "Add spectral profile to spectral profile data error.";
        return nullptr;
    }

    spectralProfile->set_coordinate("z");
    for(auto iter = profileData.begin(); iter != profileData.end(); iter++) {
        spectralProfile->add_vals(iter->second);
    }

    // end of timer for computing Z profile
    int elapsedTime = timer.elapsed();
    if (CARTA_RUNTIME_CHECKS) {
        qCritical() << "<> Time to get spectral profile:" << elapsedTime << "ms";
    }

    qDebug() << "[DataSource] .......................................................................Done";

    return spectralProfileData;
}

DataSource::~DataSource() {
}

}
}
