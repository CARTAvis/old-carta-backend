/***
 * Manages and loads a single source of data.
 */

#pragma once

#include "CartaLib/Nullable.h"
#include "CartaLib/AxisDisplayInfo.h"
#include "CartaLib/CartaLib.h"
#include "CartaLib/AxisInfo.h"
#include "CartaLib/IntensityUnitConverter.h"
#include "CartaLib/IntensityCacheHelper.h"
#include "CartaLib/IPercentileCalculator.h"
#include <memory>

#include "CartaLib/Proto/region_histogram.pb.h"
#include "CartaLib/Proto/raster_image.pb.h"
#include "CartaLib/Proto/spatial_profile.pb.h"
#include "CartaLib/Proto/spectral_profile.pb.h"
#include "CartaLib/Proto/region_requirements.pb.h"

#include "CartaLib/ProfileInfo.h"
#include "CartaLib/Hooks/ProfileHook.h"

typedef Carta::Lib::RegionHistogramData RegionHistogramData;
typedef std::shared_ptr<google::protobuf::MessageLite> PBMSharedPtr;

class CoordinateFormatterInterface;
class SliceND;

namespace Carta {
namespace Lib {

namespace Image {
class ImageInterface;
}

namespace NdArray {
class RawViewInterface;
}

class IPCache;
}

namespace Data {

class DataSource : public QObject {

    friend class LayerData;
    friend class DataFactory;
    friend class Controller;

    Q_OBJECT

public:

    static const QString CLASS_NAME;
    static const QString DATA_PATH;

    virtual ~DataSource();

private:

    /**
     * Resizes the frame indices to fit the current image.
     * @param sourceFrames - a list of current image frames.
     * @return a list of frames that will fit the current image.
     */
    std::vector<int> _fitFramesToImage( const std::vector<int>& sourceFrames ) const;

    /**
     * Return a list of information about the axes to display.
     * @return a list showing information about which axes should be displayed and how
     *  they should be displayed.
     */
    std::vector<Carta::Lib::AxisDisplayInfo> _getAxisDisplayInfo() const;

    /**
     * Return a list of axes in the image.
     * @return - a list of axes that are present in the image.
     */
    std::vector<Carta::Lib::AxisInfo::KnownType> _getAxisTypes() const;

    /**
     * Return a list of axes in the image, containing the type and name of axes.
     * @return - a list of axes that are present in the image.
     */
    std::vector<Carta::Lib::AxisInfo> _getAxisInfos() const;

    /**
     * Returns the type of the axis with the given index in the image.
     * @param index - the index of the axis in the coordinate system.
     * @return the type of the corresponding axis or AxisInfo::KnownType::OTHER
     *      if the index does not correspond to a coordinate axis in the image.
     */
    Carta::Lib::AxisInfo::KnownType _getAxisType( int index ) const;

    /**
     * Return the x display axis.
     * @return the x display axis.
     */
    Carta::Lib::AxisInfo::KnownType _getAxisXType() const;

    /**
     * Return the y display axis.
     * @return the y display axis.
     */
    Carta::Lib::AxisInfo::KnownType _getAxisYType() const;

    /**
     * Return the hidden axes.
     * @return the hidden axes.
     */
    std::vector<Carta::Lib::AxisInfo::KnownType> _getAxisZTypes() const;

    /**
     * Return the coordinates at pixel (x, y) in the given coordinate system.
     * @param x the x-coordinate of the desired pixel.
     * @param y the y-coordinate of the desired pixel.
     * @param system the desired coordinate system.
     * @param frames - a list of current image frames.
     * @return a list formatted coordinates.
     */
    QStringList _getCoordinates( double x, double y, Carta::Lib::KnownSkyCS system,
            const std::vector<int>& frames) const;

    /**
     * Return the image size for the given coordinate index.
     * @param coordIndex an index of a coordinate of the image.
     * @return the corresponding dimension for that coordinate or -1 if none exists.
     */
    int _getDimension( int coordIndex ) const;

    /**
     * Return the number of dimensions in the image.
     * @return the number of image dimensions.
     */
    int _getDimensions() const;

    /**
     * Returns the number of frames in the horizontal and vertical display directions,
     * respectively.
     * @return - a pair consisting of frame counts on the horizontal and vertical axis.
     */
    std::pair<int,int> _getDisplayDims() const;

    /**
     * Returns the image's file name.
     * @return the path to the image.
     */
    QString _getFileName() const;


    /**
     * Return the number of frames for a particular axis in the image.
     * @param type - the axis for which a frame count is needed.
     * @return the number of frames for the given axis in the image.
     */
    int _getFrameCount( Carta::Lib::AxisInfo::KnownType type ) const;

    /**
     * Get the index of the current frame of the axis specified by the sourceFrameIndex.
     * @param sourceFrameIndex - an index referring to a specific element in sourceFrames.
     * @param sourceFrames - a list for each axis type, indicating the current frame of the axis.
     * @return - the current frame for the axis.
     */
    int _getFrameIndex( int sourceFrameIndex, const std::vector<int>& sourceFrames ) const;

    /**
     * Get the permutation order of the datacube.
     * @return - the permutation order of the datacube.
     */
    std::vector<int> _getPermOrder() const;

    /**
     * Returns the underlying image.
     */
    std::shared_ptr<Carta::Lib::Image::ImageInterface> _getImage();
    std::shared_ptr<Carta::Lib::Image::ImageInterface> _getPermImage();

    /**
     * Returns the intensity and error corresponding to a percentile value.
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param percentiles - a list of numbers in [0,1] for which an intensity is desired.
     * @param stokeFrame - the index number of stoke slice
     * @param transformationLabel - a string identifier for the per-frame unit conversion used in the calculation
     * @return - a pointer to an IntensityValue object,
     * or a null pointer if the percentile is not in the cache
     */
    std::shared_ptr<Carta::Lib::IntensityValue> _readIntensityCache(int frameLow, int frameHigh, double percentile, int stokeFrame, QString transformationLabel) const;

    void _setIntensityCache(double intensity, double error, int frameLow, int frameHigh, double percentile, int stokeFrame, QString transformationLabel) const;

    /**
     * Returns the intensities corresponding to a given percentiles.
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param percentiles - a list of numbers in [0,1] for which an intensity is desired.
     * @param stokeFrame - the index number of stoke slice
     * @param transformationLabel - a string identifier for the per-frame unit conversion used in the calculation
     * @return - a list of intensity values.
     */
    std::vector<double> _getMinMaxIntensity( int frameLow, int frameHigh,
            const std::vector<double>& percentiles, int stokeFrame,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter) const;

    /**
     * Returns the histogram of pixels.
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param numberOfBins - the number of histogram bins between minimum and maximum of pixel values.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @param converter - used to convert the pixel values for different unit.
     * @return - a struct RegionHistogramData.
     */
    PBMSharedPtr _getPixels2Histogram(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
            int numberOfBins,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter) const;

    RegionHistogramData _getPixels2HistogramData(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
            int numberOfBins,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter) const;

    int _getStokeIndicator();
    int _getSpectralIndicator();

    /**
     * Returns a vector of pixels.
     * @param xMin - lower bound of the x-pixel-coordinate.
     * @param xMax - upper bound 0f the x-pixel-coordinate.
     * @param yMin - lower bound of the y-pixel-coordinate.
     * @param yMax - upper bound 0f the y-pixel-coordinate.
     * @param mip - down sampling factor.
     * @param minIntensity - use the minimum of intensity value to replace the NaN type of pixel value.
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @return - vector of pixels.
     */
    PBMSharedPtr _getRasterImageData(int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
        int frameLow, int frameHigh, int stokeFrame,
        bool isZFP, int precision, int numSubsets,
        bool &changeFrame, int regionId, int numberOfBins,
        Carta::Lib::IntensityUnitConverter::SharedPtr converter) const;

    int _compress(std::vector<float>& array, size_t offset, std::vector<char>& compressionBuffer,
            size_t& compressedSize, uint32_t nx, uint32_t ny, uint32_t precision) const;

    std::vector<int32_t> _getNanEncodingsBlock(std::vector<float>& array, int offset, int w, int h) const;

    /**
     * Returns a spatial profile data
     * @param x - x coordinate of cursor
     * @param y - y coordinate of cursor
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @return - vectors of x profile/y profile.
     */
    PBMSharedPtr _getXYProfiles(int fileId, int x, int y,
        int frameLow, int frameHigh, int stokeFrame,
        Carta::Lib::IntensityUnitConverter::SharedPtr converter) const;

    void _getXYProfiles(Carta::Lib::NdArray::Double doubleView, const int imgWidth, const int imgHeight,
    const int x, const int y, std::vector<float> & xProfile, std::vector<float> & yProfile) const;

    bool _addProfile(std::shared_ptr<CARTA::SpatialProfileData> spatialProfileData,
        const std::vector<float> & profile, const std::string coordinate) const;

    std::shared_ptr<Carta::Lib::Image::ImageInterface> _getPermutedImage() const;

    /**
     * Return the pixel coordinates corresponding to the given world coordinates.
     * @param ra - the right ascension (in radians) of the world coordinates.
     * @param dec - the declination (in radians) of the world coordinates.
     * @param valid - true if the pixel coordinates are valid; false otherwise.
     * @return - a point containing the pixel coordinates.
     */
    QPointF _getPixelCoordinates( double ra, double dec, bool* valid ) const;

    /**
     * Return the rest frequency and units for the image.
     * @return - the image rest frequency and units; a blank string and a negative
     * 		value are returned with the rest frequency can not be found.
     */
    std::pair<double,QString> _getRestFrequency() const;

    /**
     * Return the world coordinates corresponding to the given pixel coordinates.
     * @param pixelX - the first pixel coordinate.
     * @param pixelY - the second pixel coordinate.
     * @param valid - true if the pixel coordinates are valid; false otherwise.
     * @return - a point containing the pixel coordinates.
     */
    QPointF _getWorldCoordinates( double pixelX, double pixelY,
            Carta::Lib::KnownSkyCS coordSys, bool* valid ) const;

    /**
     * Return the units of the pixels.
     * @return the units of the pixels, or blank if units could not be obtained.
     */
    QString _getPixelUnits() const;

    /**
     * Returns the raw data as an array.
     * @param axisIndex - an index of an image axis.
     * @param frameLow the lower bound for the frames or -1 for the whole image.
     * @param frameHigh the upper bound for the frames or -1 for the whole image.
     * @param axisIndex - the axis for the frames or -1 for all axes.
     * @return the raw data or nullptr if there is none.
     */
    Carta::Lib::NdArray::RawViewInterface* _getRawData( int frameLow, int frameHigh, int axisIndex ) const;

    /**
     * Returns the raw data as an array.
     * @param axisIndex - an index of an image axis.
     * @param frameLow the lower bound for the frames or -1 for the whole image.
     * @param frameHigh the upper bound for the frames or -1 for the whole image.
     * @param axisIndex - the axis for the frames or -1 for all axes.
     * @param axisStokeIndex - the axis for the stoke frame.
     * @param stokeSliceIndex - the index of the stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V).
     * @return the raw data or nullptr if there is none.
     */
    Carta::Lib::NdArray::RawViewInterface* _getRawDataForStoke(int frameLow, int frameHigh, int stokeFrame) const;

    /**
     * Returns the raw data for the current view.
     * @param frames - a list of current image frames.
     * @return the raw data for the current view or nullptr if there is none.
     */
    Carta::Lib::NdArray::RawViewInterface* _getRawData( const std::vector<int> frames ) const;

    //Returns an identifier for the current image slice being rendered.
    QString _getViewIdCurrent( const std::vector<int>& frames ) const;

    ///Returns whether the frames actually exist in the image.
    bool _isLoadable( std::vector<int> frames ) const;

    //Returns whether or not there is a spectral axis in the image.
    bool _isSpectralAxis() const;

    /**
     * Set the coordinate system in the duplicated coordinateformatter,
     * i.e. m_coordinateFormatter.
     * @param cs - the target coordinate system (defined in CartaLib::KnownSkyCS)
     */
    void _setCoordinateSystem( Carta::Lib::KnownSkyCS cs );

    /**
     * Set a particular axis type to be one of the display axes.
     * @param axisType - the type of axis.
     * @param axisIndex - a pointer to the display axis index.
     * @return true if the specified display axis changes; false otherwise.
     */
    bool _setDisplayAxis( Carta::Lib::AxisInfo::KnownType axisType, int* axisIndex );

    /**
     * Attempts to load an image file.
     * @param fileName an identifier for the location of a data source.
     * @param success - set to true if the image file is successfully loaded; otherwise,
     *      set to false.
     * @return - an error message if the image was not successfully loaded; otherwise,
     *      an empty string.
     */
    QString _setFileName( const QString& fileName, bool* success );

    // set spatial requirements
    bool _setSpatialRequirements(int fileId, int regionId,
            google::protobuf::RepeatedPtrField<std::string> spatialProfiles);

    // set spectral requirements
    bool _setSpectralRequirements(int fileId, int regionId, int stokeFrame,
            google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles);

    // calculate spectral profile (z-profile)
    PBMSharedPtr _getSpectralProfile(int fileId, int x, int y, int stoke);

    /**
     *  Constructor.
     */
    DataSource();

    QString m_fileName;
    int m_cmapCacheSize;

    //Pointer to image interface.
    std::shared_ptr<Carta::Lib::Image::ImageInterface> m_image;
    std::shared_ptr<Carta::Lib::Image::ImageInterface> m_permuteImage;

    // profile info
    Carta::Lib::ProfileInfo m_profileInfo;

    // profile calculation result
    Carta::Lib::Hooks::ProfileResult m_profileResult;

    /// coordinate formatter
    std::shared_ptr<CoordinateFormatterInterface> m_coordinateFormatter;

    // disk cache
    std::shared_ptr<Carta::Lib::IPCache> m_diskCache;
    // wrapper class
    std::shared_ptr<Carta::Lib::IntensityCacheHelper> m_diskCacheHelper;

    //Indices of the display axes.
    int m_axisIndexX;
    int m_axisIndexY;

    const static bool APPROXIMATION_GET_LOCATION;
    const static bool IS_MULTITHREAD_ZFP;
    const static int MAX_SUBSETS;

    DataSource(const DataSource& other);
    DataSource& operator=(const DataSource& other);
};
}
}
