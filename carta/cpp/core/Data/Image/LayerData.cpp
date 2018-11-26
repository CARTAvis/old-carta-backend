#include "DataSource.h"
#include "Data/DataLoader.h"
#include "Data/Util.h"
#include "State/UtilState.h"
#include "CartaLib/AxisDisplayInfo.h"

#include <QDebug>
#include <QTime>
#include "LayerData.h"

using Carta::Lib::AxisInfo;
using Carta::Lib::AxisDisplayInfo;

namespace Carta {
namespace Data {

const QString LayerData::CLASS_NAME = "LayerData";

class LayerData::Factory : public Carta::State::CartaObjectFactory {

public:

    Carta::State::CartaObject * create (const QString & path, const QString & id)
    {
        return new LayerData(path, id);
    }
};

bool LayerData::m_registered =
        Carta::State::ObjectManager::objectManager()->registerClass (CLASS_NAME,
                                                   new LayerData::Factory());

LayerData::LayerData(const QString& path, const QString& id) :
    Layer( CLASS_NAME, path, id),
    m_dataSource( new DataSource()) {
}

std::shared_ptr<DataSource> LayerData::_getDataSource(){
    return m_dataSource;
}

std::vector<int> LayerData::_getImageDimensions( ) const {
    std::vector<int> result;
    if ( m_dataSource ){
        int dimensions = m_dataSource->_getDimensions();
        for ( int i = 0; i < dimensions; i++ ) {
            int d = m_dataSource->_getDimension( i );
            result.push_back( d );
        }
    }
    return result;
}

int LayerData::_getFrameCount( AxisInfo::KnownType type ) const {
    int frameCount = 1;
    if ( m_dataSource ){
        frameCount = m_dataSource->_getFrameCount( type );
    }
    return frameCount;
}

std::shared_ptr<Carta::Lib::Image::ImageInterface> LayerData::_getImage(){
    std::shared_ptr<Carta::Lib::Image::ImageInterface> image;
    if ( m_dataSource ){
        image = m_dataSource->_getImage();
    }
    return image;
}

int LayerData::_getStokeIndicator() const {
    int result;
    if ( m_dataSource ){
        result = m_dataSource->_getStokeIndicator();
    }
    return result;
}

int LayerData::_getSpectralIndicator() const {
    int result;
    if ( m_dataSource ){
        result = m_dataSource->_getSpectralIndicator();
    }
    return result;
}

PBMSharedPtr LayerData::_getPixels2Histogram(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
    int numberOfBins,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr results;
    if ( m_dataSource ){
        results = m_dataSource->_getPixels2Histogram(fileId, regionId, frameLow, frameHigh, stokeFrame, numberOfBins, converter);
    }
    return results;
}

PBMSharedPtr LayerData::_getXYProfiles(int fileId, int x, int y,
    int frameLow, int frameHigh, int stokeFrame,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr results;
    if ( m_dataSource ){
        results = m_dataSource->_getXYProfiles(fileId, x, y, frameLow, frameHigh, stokeFrame, converter);
    }
    return results;
}

bool LayerData::_setSpatialRequirements(int fileId, int regionId,
    google::protobuf::RepeatedPtrField<std::string> spatialProfiles) const {
    if ( !m_dataSource ){
        return false;
    }

    return m_dataSource->_setSpatialRequirements(fileId, regionId, spatialProfiles);
}

bool LayerData::_setSpectralRequirements(int fileId, int regionId, int stokeFrame,
    google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles) const {
    if ( !m_dataSource ){
        return false;
    }

    return m_dataSource->_setSpectralRequirements(fileId, regionId, stokeFrame, spectralProfiles);
}

PBMSharedPtr LayerData::_getSpectralProfile(int fileId, int x, int y, int stokeFrame) const {
    if ( !m_dataSource ){
        return nullptr;
    }

    return m_dataSource->_getSpectralProfile(fileId, x, y, stokeFrame);
}

PBMSharedPtr LayerData::_getRasterImageData(int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
    int frameLow, int frameHigh, int stokeFrame,
    bool isZFP, int precision, int numSubsets,
    bool &changeFrame, int regionId, int numberOfBins,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr results;
    if (m_dataSource) {
        results = m_dataSource->_getRasterImageData(fileId, xMin, xMax, yMin, yMax, mip,
                                                    frameLow, frameHigh, stokeFrame,
                                                    isZFP, precision, numSubsets,
                                                    changeFrame, regionId, numberOfBins, converter);
    }
    return results;
}

std::vector<double> LayerData::_getPercentiles( int frameLow, int frameHigh, std::vector<double> intensities, Carta::Lib::IntensityUnitConverter::SharedPtr converter ) const {
    std::vector<double> percentiles(intensities.size());
    if ( m_dataSource ){
        percentiles = m_dataSource->_getPercentiles( frameLow, frameHigh, intensities, converter );
    }
    return percentiles;
}

QString LayerData::_getPixelValue( double x, double y, const std::vector<int>& frames ) const {
    QString pixelValue( "" );
    if ( m_dataSource ){
        pixelValue = m_dataSource->_getPixelValue( x, y, frames );
    }
    return pixelValue;
}

QString LayerData::_getFileName() {
    return m_dataSource->_getFileName();
}

QString LayerData::_setFileName( const QString& fileName, bool * success ){
    QString result = m_dataSource->_setFileName( fileName, success );
    return result;
}

LayerData::~LayerData() {
}
}
}
