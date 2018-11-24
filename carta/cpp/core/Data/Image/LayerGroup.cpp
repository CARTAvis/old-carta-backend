#include "LayerGroup.h"
#include "LayerData.h"
#include "DataSource.h"
#include "Data/Util.h"
#include "CartaLib/IRemoteVGView.h"
#include "State/UtilState.h"

#include <QDebug>
#include <QDir>
#include <cmath>

using Carta::Lib::AxisInfo;

namespace Carta {

namespace Data {

const QString LayerGroup::CLASS_NAME = "LayerGroup";
const QString LayerGroup::COMPOSITION_MODE="mode";
const QString LayerGroup::LAYERS = "layers";

class LayerGroup::Factory : public Carta::State::CartaObjectFactory {

public:

    Carta::State::CartaObject * create (const QString & path, const QString & id)
    {
        return new LayerGroup(path, id);
    }
};

bool LayerGroup::m_registered =
        Carta::State::ObjectManager::objectManager()->registerClass (CLASS_NAME,
                                                   new LayerGroup::Factory());

LayerGroup::LayerGroup( const QString& path, const QString& id ):
    LayerGroup( CLASS_NAME, path, id ) {
}

LayerGroup::LayerGroup(const QString& className, const QString& path, const QString& id) :
    Layer( className, path, id) {
}

QString LayerGroup::_addData(const QString& fileName, bool* success, int* stackIndex, int fileId) {
    QString result;
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    LayerData* targetSource = objMan->createObject<LayerData>();
    result = targetSource->_setFileName(fileName, success );
    //If we are making a new layer, see if there is a selected group.  If so,
    //add to the group.  If not, add to this group.
    if ( *success ) {
        if (fileId >= 0 && m_children.size() - 1 >= fileId) {
            // there is a children exists, replace it
            qDebug() << "[LayerGroup] There is an image object exists, replace it, fileId=" << fileId;
            m_children.replace(fileId, std::shared_ptr<Layer>(targetSource));
            *stackIndex = fileId;
        } else if (fileId >= 0 && m_children.size() == fileId) {
            // append a new children in the list
            qDebug() << "[LayerGroup] Append a new image object in the list, fileId=" << fileId;
            m_children.append(std::shared_ptr<Layer>(targetSource));
            //qDebug() << "[LayerGroup] Insert a new image object in the list, fileId=" << fileId;
            //m_children.insert(fileId, std::shared_ptr<Layer>(targetSource));
            *stackIndex = fileId;
        } else {
            qCritical() << "No image object is cached! check fileId=" << fileId << ", m_children.size()=" << m_children.size();
        }
    }
    else {
        delete targetSource;
    }
    return result;
}

bool LayerGroup::_addGroup(){
    Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
    LayerGroup* targetSource = objMan->createObject<LayerGroup>();
    m_children.append( std::shared_ptr<Layer>(targetSource));
    return true;
}

void LayerGroup::_addLayer( std::shared_ptr<Layer> layer, int targetIndex ){
    int tIndex = targetIndex;
    int childCount = m_children.size();
    if ( tIndex == -1 || tIndex == childCount){
        m_children.append( layer );
        tIndex = m_children.size() - 1;
    }
    else if (tIndex >= 0 && tIndex < childCount){
        m_children.insert( tIndex, layer );
    }
}

void LayerGroup::_clearData(){
    int childCount = m_children.size();
    for ( int i = childCount - 1; i>= 0; i-- ){
        _removeData( i );
    }
}

bool LayerGroup::_closeData( const QString& id ){
    bool dataClosed = false;
    int dataCount = m_children.size();
    int closeIndex = id.toInt();
    if (closeIndex >= 0 && closeIndex < dataCount) {
        _removeData(closeIndex);
        dataClosed = true;
    }
    return dataClosed;
}

int LayerGroup::_getFrameCount( AxisInfo::KnownType type ) const {
    int frameCount = 1;
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        frameCount = m_children[dataIndex]->_getFrameCount( type );
    }
    return frameCount;
}

std::shared_ptr<Carta::Lib::Image::ImageInterface> LayerGroup::_getImage(){
    std::shared_ptr<Carta::Lib::Image::ImageInterface> image(nullptr);
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        image = m_children[dataIndex]->_getImage();
    }
    return image;
}

std::shared_ptr<DataSource> LayerGroup::_getDataSource(){
    std::shared_ptr<DataSource> dSource( nullptr );
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        dSource = m_children[dataIndex]->_getDataSource();
    }
    return dSource;
}

std::vector<int> LayerGroup::_getImageDimensions( ) const {
    std::vector<int> result;
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        result = m_children[dataIndex]->_getImageDimensions();
    }
    return result;
}

// this function is replaced in the Stack.cpp, so it is just a dummy function.
int LayerGroup::_getIndexCurrent( ) const {
    int dataIndex = -1;
    //The current index should be the first selected one.
    //int childCount = m_children.size();
    //for ( int i = 0; i < childCount; i++ ){
    //    if ( m_children[i]->_isSelected()){
    //        dataIndex = i;
    //        break;
    //    }
    //}
    //Just choose the top one if nothing is selected
    //if ( dataIndex == -1 && childCount > 0 ){
    //    dataIndex = 0;
    //}
    return dataIndex;
}

int LayerGroup::_getStokeIndicator() const {
    int result;
    int dataIndex = _getIndexCurrent();
    if (dataIndex >= 0) {
        result = m_children[dataIndex]->_getStokeIndicator();
    }
    return result;
}

int LayerGroup::_getSpectralIndicator() const {
    int result;
    int dataIndex = _getIndexCurrent();
    if (dataIndex >= 0) {
        result = m_children[dataIndex]->_getSpectralIndicator();
    }
    return result;
}

PBMSharedPtr LayerGroup::_getPixels2Histogram(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
    int numberOfBins, Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr results;
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        results = m_children[dataIndex]->_getPixels2Histogram(fileId, regionId, frameLow, frameHigh, stokeFrame, numberOfBins, converter);
    }
    return results;
}

PBMSharedPtr LayerGroup::_getXYProfiles(int fileId, int x, int y,
    int frameLow, int frameHigh, int stokeFrame,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr results;
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        results = m_children[dataIndex]->_getXYProfiles(fileId, x, y, frameLow, frameHigh, stokeFrame, converter);
    }
    return results;
}

bool LayerGroup::_setSpatialRequirements(int fileId, int regionId,
            google::protobuf::RepeatedPtrField<std::string> spatialProfiles) const {
    int dataIndex = _getIndexCurrent();
    if ( dataIndex < 0 ){
        return false;
    }

    return m_children[dataIndex]->_setSpatialRequirements(fileId, regionId, spatialProfiles);
}

bool LayerGroup::_setSpectralRequirements(int fileId, int regionId, int stokeFrame,
            google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles) const {
    int dataIndex = _getIndexCurrent();
    if ( dataIndex < 0 ){
        return false;
    }

    return m_children[dataIndex]->_setSpectralRequirements(fileId, regionId, stokeFrame, spectralProfiles);
}

PBMSharedPtr LayerGroup::_getSpectralProfile(int fileId, int x, int y, int stokeFrame) const {
    int dataIndex = _getIndexCurrent();
    if ( dataIndex < 0 ){
        return nullptr;
    }

    return m_children[dataIndex]->_getSpectralProfile(fileId, x, y, stokeFrame);
}

PBMSharedPtr LayerGroup::_getRasterImageData(int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
    int frameLow, int frameHigh, int stokeFrame,
    bool isZFP, int precision, int numSubsets,
    bool &changeFrame, int regionId, int numberOfBins,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr results;
    int dataIndex = _getIndexCurrent();
    if (dataIndex >= 0) {
        results = m_children[dataIndex]->_getRasterImageData(fileId, xMin, xMax, yMin, yMax, mip,
                                                             frameLow, frameHigh, stokeFrame,
                                                             isZFP, precision, numSubsets,
                                                             changeFrame, regionId, numberOfBins, converter);
    }
    return results;
}

std::vector<double> LayerGroup::_getPercentiles( int frameLow, int frameHigh, std::vector<double> intensities, Carta::Lib::IntensityUnitConverter::SharedPtr converter ) const {
    std::vector<double> percentiles(intensities.size());
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        percentiles = m_children[dataIndex]->_getPercentiles( frameLow, frameHigh, intensities, converter );
    }
    return percentiles;
}

QString LayerGroup::_getPixelValue( double x, double y, const std::vector<int>& frames ) const {
    QString pixelValue("");
    int dataIndex = _getIndexCurrent();
    if ( dataIndex >= 0 ){
        pixelValue = m_children[dataIndex]->_getPixelValue( x, y, frames );
    }
    return pixelValue;
}

void LayerGroup::_removeData( int index ){
    int childCount = m_children.size();
    if ( 0 <= index && index < childCount ){
        disconnect( m_children[index].get());
        QString id = m_children[index]->getId();
        Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();
        objMan->removeObject( id );
        m_children.removeAt( index );
        qDebug() << "[LayerGroup] remove the image index=" << index;
    }
}

LayerGroup::~LayerGroup() {
    _clearData();

}
}
}
