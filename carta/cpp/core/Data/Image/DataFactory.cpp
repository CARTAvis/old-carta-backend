#include "DataFactory.h"
#include "Data/DataLoader.h"
#include "Data/Image/Controller.h"
#include "Data/Image/DataSource.h"
#include "Data/Util.h"
#include "Globals.h"

#include <QDebug>

namespace Carta {
namespace Data {

DataFactory::DataFactory(){
}

QString DataFactory::addData(Controller* controller, const QString& fileName, bool* success , int fileId){
    QString result;
    *success = false;
    if ( controller ){    
        result = controller->_addDataImage(fileName, success, fileId);
    }
    else {
        result = "The data in "+fileName +" could not be added because no controller was specified.";
    }
    return result;
}

DataFactory::~DataFactory(){

}
}
}
