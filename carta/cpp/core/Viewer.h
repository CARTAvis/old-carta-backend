/**
 * The viewer application represented as an object...
 */

#pragma once

#include <QObject>
#include <memory>

namespace Carta {
namespace Data {
class ViewManager;
}
}

///
/// \brief The Viewer class is the main class of the viewer. It sets up all other
/// components.
///
class Viewer : public QObject
{
    Q_OBJECT

public:

    /// constructor
    /// should be called when platform is initialized, but connector isn't
    explicit Viewer();

    /// this should be called when connector is already initialized (i.e. it's
    /// safe to start setting/getting state)
    void start();

    /// Show areas under active development.
    void setDeveloperView( );

    std::shared_ptr<Carta::Data::ViewManager> m_viewManager;

signals:

public slots:

    /// Closing the database of cache before quit
    /// It had better be executed by other object (such as any unloaded plugin when exitting)
    void DBClose();

protected slots:

protected:

private:

    bool m_devView;

};


