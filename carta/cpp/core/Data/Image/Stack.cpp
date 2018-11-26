#include "LayerGroup.h"
#include "Data/Image/DataSource.h"
#include "Data/Util.h"
#include "State/UtilState.h"
#include "State/StateInterface.h"
#include "Globals.h"

#include <QDebug>
#include <QDir>
#include "Stack.h"

using Carta::Lib::AxisInfo;

namespace Carta {
namespace Data {

const QString Stack::CLASS_NAME = "Stack";

class Stack::Factory : public Carta::State::CartaObjectFactory {

public:

    Carta::State::CartaObject * create (const QString & path, const QString & id)
    {
        return new Stack(path, id);
    }
};

bool Stack::m_registered =
        Carta::State::ObjectManager::objectManager()->registerClass (CLASS_NAME,
                                                   new Stack::Factory());

Stack::Stack(const QString& path, const QString& id) :
    LayerGroup( CLASS_NAME, path, id) {
}

QString Stack::_addDataImage(const QString& fileName, bool* success, int fileId) {
    int stackIndex = -1;
    // add fileId here
    QString result = _addData( fileName, success, &stackIndex, fileId);
    if ( *success && stackIndex >= 0 ){
        // set the file id on the image viewer
        _setFileId(fileId);
    }
    return result;
}

bool Stack::_addGroup( ){
    bool groupAdded = LayerGroup::_addGroup();
    return groupAdded;
}

bool Stack::_closeData( const QString& id ){
    // remove the fileId
    bool dataClosed = LayerGroup::_closeData( id );
    return dataClosed;
}

QStringList Stack::_getOpenedFileList() {

    QStringList nameList;
    for ( std::shared_ptr<Layer> layer : m_children ){
        nameList.append( layer->_getFileName());
    }
    return nameList;
}

int Stack::_getIndexCurrent( ) const {
    int dataIndex = 0;
    // get the private parameter of fileId here
    if (m_fileId >= 0 && m_fileId < m_children.size()) {
        dataIndex = m_fileId;
    } else {
        qFatal("Image index requested by the frontend is not matched with the backend status!");
    }

    qDebug() << "[Stack] dataIndex (current index)=" << dataIndex;
    return dataIndex;
}

QString Stack::getStateString() const{
    QString result = m_state.toString();
    return result;
}

void Stack::_setFileId(int fileId) {
    m_fileId = fileId;
}

Stack::~Stack() {
}

}
}
