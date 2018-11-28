/***
 * Represents an indexed group of layers in a stack.
 */

#pragma once

#include "LayerGroup.h"
#include "State/ObjectManager.h"
#include "CartaLib/IImage.h"
#include "CartaLib/AxisInfo.h"
#include "CartaLib/InputEvents.h"

namespace Carta {
namespace Data {

typedef Carta::Lib::InputEvents::JsonEvent InputEvent;

class Stack : public LayerGroup {

friend class Controller;

Q_OBJECT

public:

    /**
     * Returns a json string representing data selections.
     * @return a Json string representing data selections.
     */
    QString getStateString() const;
        static const QString CLASS_NAME;
        virtual ~Stack();

signals:

	  void inputEvent( InputEvent ev );

    /// Return the result of SaveFullImage() after the image has been rendered
    /// and a save attempt made.
    void saveImageResult( bool result );

protected:

    virtual bool _addGroup( ) Q_DECL_OVERRIDE;
    virtual bool _closeData( const QString& id ) Q_DECL_OVERRIDE;
    virtual int _getIndexCurrent( ) const Q_DECL_OVERRIDE;

private slots:

private:

    QStringList _getOpenedFileList();

    QString _addDataImage(const QString& fileName, bool* success , int fileId);

    QString _reorderImages( const std::vector<int> & indices );

    /**
     *  Constructor.
     */
    Stack( const QString& path, const QString& id );

    class Factory;
    static bool m_registered;

    // save the current file id on the frontend image viewer
    int m_fileId = -1;
    // set the current file id on the frontend image viewer
    void _setFileId(int fileId);

    Stack(const Stack& other);
    Stack& operator=(const Stack& other);
};
}
}
