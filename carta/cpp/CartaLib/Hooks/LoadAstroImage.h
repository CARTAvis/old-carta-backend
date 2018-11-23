/**
 * Hook loading astronomical images.
 *
 **/

#pragma once

#include "CartaLib/CartaLib.h"
#include "CartaLib/IPlugin.h"
#include <vector>

namespace Carta
{
namespace Lib
{
namespace Hooks
{
/// load an (astronomical) image and convert to an instance of Image::ImageInterface
class LoadAstroImage : public BaseHook
{
    CARTA_HOOK_BOILER1( LoadAstroImage );

public:

    typedef Image::ImageInterface::SharedPtr ResultType;
    struct Params {
        Params( QString p_fileName,  QString p_hdu)
        {
            fileName = p_fileName;
            hdu = p_hdu;
        }

        QString fileName;
        QString hdu;
    };

    LoadAstroImage( Params * pptr ) : BaseHook( staticId ), paramsPtr( pptr )
    {
        // force instantiation of templates
        CARTA_ASSERT( is < Me > () );
    }

    ResultType result;
    Params * paramsPtr;
};
}
}
}
