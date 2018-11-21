#include "State/ObjectManager.h"
#include "State/UtilState.h"
#include "Data/Image/Controller.h"
#include "Data/Image/CoordinateSystems.h"
#include "Data/Image/DataFactory.h"
#include "Data/Image/Stack.h"
#include "Data/Image/DataSource.h"
//#include "Data/Image/Contour/ContourControls.h"
//#include "Data/Image/Contour/DataContours.h"
//#include "Data/Region/RegionControls.h"
//#include "Data/Region/Region.h"
#include "Data/Settings.h"
#include "Data/DataLoader.h"
#include "Data/Error/ErrorManager.h"
#include "../../ImageRenderService.h"
//#include "Data/Colormap/Colormaps.h"

#include "Data/Util.h"
#include "ImageView.h"
#include "CartaLib/IImage.h"
#include "Globals.h"

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QDir>
#include <memory>
#include <set>

using namespace std;

namespace Carta {

namespace Data {

class Controller::Factory : public Carta::State::CartaObjectFactory {

public:

    Carta::State::CartaObject * create (const QString & path, const QString & id)
    {
        return new Controller (path, id);
    }
};

const QString Controller::CLIP_VALUE_MIN = "clipValueMin";
const QString Controller::CLIP_VALUE_MAX = "clipValueMax";
const QString Controller::AUTO_CLIP = "autoClip";
const QString Controller::PLUGIN_NAME = "ImageViewer";

const QString Controller::CLASS_NAME = "Controller";
bool Controller::m_registered = Carta::State::ObjectManager::objectManager()->registerClass (CLASS_NAME, new Controller::Factory());

using Carta::State::UtilState;
using Carta::State::StateInterface;
using Carta::Lib::AxisInfo;

Controller::Controller( const QString& path, const QString& id ) :
                CartaObject( CLASS_NAME, path, id) {

	Carta::State::ObjectManager* objMan = Carta::State::ObjectManager::objectManager();

	//Stack
    Stack* layerGroupRoot = objMan->createObject<Stack>();
    m_stack.reset( layerGroupRoot ); // necessary !!
}

QString Controller::addData(const QString& fileName, bool* success, int fileId) {
	*success = false;
    QString result = DataFactory::addData( this, fileName, success, fileId);
    return result;
}

void Controller::setFileId(int fileId) {
    // set the file id as the private parameter in the Stack object
    m_stack->_setFileId(fileId);
}

QString Controller::_addDataImage(const QString& fileName, bool* success, int fileId) {
    // assign the fileId as a private parameter in the m_stack
    QString result = m_stack->_addDataImage(fileName, success, fileId);
    if ( *success ) {
        qDebug() << "[Controller] open file is successful!";
    } else {
        qDebug() << "[Controller] fail to open file!";
    }
    return result;
}

QString Controller::applyClips( double minIntensityPercentile, double maxIntensityPercentile ){
    QString result("");
    return result;
}

void Controller::clear(){
    unregisterView();
}

QString Controller::closeImage( const QString& id ){
    QString result;
    bool imageClosed = m_stack->_closeData( id );
    if ( !imageClosed ){
        result = "Could not find data to remove for id="+id;
    }
    return result;
}

bool Controller::getAutoClip() const {
    bool autoClip = m_state.getValue<bool>(AUTO_CLIP);
    return autoClip;
}

double Controller::getClipPercentileMax() const {
    double clipValueMax = m_state.getValue<double>(CLIP_VALUE_MAX);
    return clipValueMax;
}

double Controller::getClipPercentileMin() const {
    double clipValueMin = m_state.getValue<double>(CLIP_VALUE_MIN);
    return clipValueMin;
}

std::shared_ptr<DataSource> Controller::getDataSource() const {
    return m_stack->_getDataSource();
}

//std::shared_ptr<RegionControls> Controller::getRegionControls() {
//	return m_regionControls;
//}

std::vector< std::shared_ptr<Carta::Lib::Image::ImageInterface> > Controller::getImages() {
    return m_stack->_getImages();
}

std::shared_ptr<Carta::Lib::Image::ImageInterface> Controller::getImage() {
    return m_stack->_getImage();
}

std::vector<int> Controller::getImageDimensions( ) const {
    std::vector<int> result = m_stack->_getImageDimensions();
    return result;
}

std::vector<int> Controller::getImageSlice() const {
    std::vector<int> result = m_stack->_getImageSlice();
    return result;
}

int Controller::getStokeIndicator() const {
    int result = m_stack->_getStokeIndicator();
    return result;
}

int Controller::getSpectralIndicator() const {
    int result = m_stack->_getSpectralIndicator();
    return result;
}

PBMSharedPtr Controller::getPixels2Histogram(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame, int numberOfBins, Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr result = m_stack->_getPixels2Histogram(fileId, regionId, frameLow, frameHigh, stokeFrame, numberOfBins, converter);
    return result;
}

PBMSharedPtr Controller::getXYProfiles(int fileId, int x, int y,
            int frameLow, int frameHigh, int stokeFrame,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr result = m_stack->_getXYProfiles(fileId, x, y, frameLow, frameHigh, stokeFrame, converter);
    return result;
}

bool Controller::setSpatialRequirements(int fileId, int regionId,
            google::protobuf::RepeatedPtrField<std::string> spatialProfiles) const {
    return m_stack->_setSpatialRequirements(fileId, regionId, spatialProfiles);
}

bool Controller::setSpectralRequirements(int fileId, int regionId, int stokeFrame,
            google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles) const {
    return m_stack->_setSpectralRequirements(fileId, regionId, stokeFrame, spectralProfiles);
}

PBMSharedPtr Controller::getSpectralProfile(int fileId, int x, int y, int stokeFrame) const {
    return m_stack->_getSpectralProfile(fileId, x, y, stokeFrame);
}

PBMSharedPtr Controller::getRasterImageData(int fileId, int x_min, int x_max, int y_min, int y_max, int mip,
    int frameLow, int frameHigh, int stokeFrame,
    bool isZFP, int precision, int numSubsets,
    bool &changeFrame, int regionId, int numberOfBins,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter) const {
    PBMSharedPtr result = m_stack->_getRasterImageData(fileId, x_min, x_max, y_min, y_max, mip,
                                                       frameLow, frameHigh, stokeFrame,
                                                       isZFP, precision, numSubsets,
                                                       changeFrame, regionId, numberOfBins, converter);
    return result;
}

std::vector<double> Controller::getPercentiles( int frameLow, int frameHigh, std::vector<double> intensities, Carta::Lib::IntensityUnitConverter::SharedPtr converter ) const {
    return m_stack->_getPercentiles( frameLow, frameHigh, intensities, converter );
}

QString Controller::getPixelUnits() const {
    QString result = m_stack->_getPixelUnits();
    return result;
}

int Controller::getSelectImageIndex() const {
    return m_stack->_getSelectImageIndex();
}

QString Controller::getStateString( const QString& sessionId, SnapshotType type ) const{
    QString result("");
    return result;
}

QString Controller::getSnapType(CartaObject::SnapshotType snapType) const {
    QString objType("");
    return objType;
}

void Controller::refreshState() {
}

void Controller::resetState( const QString& state ) {
}

void Controller::resetStateData( const QString& state ) {
}

Controller::~Controller(){
    //unregisterView();
    clear();
}

}
}
