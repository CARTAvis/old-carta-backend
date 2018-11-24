#include "Layer.h"
#include "Data/Util.h"
#include "Data/DataLoader.h"
#include "State/UtilState.h"

#include <QDebug>

using Carta::Lib::AxisInfo;

namespace Carta {

namespace Data {

const QString Layer::CLASS_NAME = "Layer";
const QString Layer::GROUP = "group";
const QString Layer::LAYER = "layer";

Layer::Layer( const QString& className, const QString& path, const QString& id) :
    CartaObject( className, path, id) {
}

void Layer::_addLayer( std::shared_ptr<Layer> /*layer*/, int /*targetIndex*/ ){

}

bool Layer::_closeData( const QString& /*id*/ ){
    return false;
}

QString Layer::_getFileName() {
    return "";
}

QString Layer::_setFileName( const QString& /*fileName*/, bool* success ){
    QString result = "Incorrect layer for loading image files.";
    *success = false;
    return result;
}

Layer::~Layer() {
}
}
}
