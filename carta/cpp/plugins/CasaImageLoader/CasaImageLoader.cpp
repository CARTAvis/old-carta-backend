#include "CasaImageLoader.h"
#include "CCImage.h"
#include "CartaLib/Hooks/Initialize.h"
#include "CartaLib/Hooks/LoadAstroImage.h"
#include <QDebug>
#include <QTime>
#include <casacore/casa/Exceptions/Error.h>
#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/MIRIADImage.h>
#include <casacore/images/Images/HDF5Image.h>
#include <casacore/images/Images/ImageExpr.h>
#include <casacore/images/Images/ImageExprParse.h>
#include <casacore/images/Images/ImageOpener.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/casa/Quanta.h>
#include <memory>
#include <algorithm>
#include <cstdint>
#include "CartaLib/UtilCASA.h"

CasaImageLoader::CasaImageLoader(QObject *parent) :
    QObject(parent)
{
}

bool CasaImageLoader::handleHook(BaseHook & hookData)
{
    qDebug() << "CasaImageLoader plugin is handling hook #" << hookData.hookId();

    if (hookData.is<Carta::Lib::Hooks::Initialize>()) {
        // Register FITS and Miriad image types
        casacore::FITSImage::registerOpenFunction();
        casacore::MIRIADImage::registerOpenFunction();
        return true;
    } else if (hookData.is<Carta::Lib::Hooks::LoadAstroImage>()) {
        Carta::Lib::Hooks::LoadAstroImage & hook
                = static_cast<Carta::Lib::Hooks::LoadAstroImage &>(hookData);
        auto fname = hook.paramsPtr->fileName;
        hook.result = loadImage(fname);
        return hook.result != nullptr;
    }

    qWarning() << "Sorry, dont' know how to handle this hook";
    return false;
}

std::vector<HookId> CasaImageLoader::getInitialHookList()
{
    return {
        Carta::Lib::Hooks::Initialize::staticId,
        Carta::Lib::Hooks::LoadAstroImage::staticId
    };
}

template <typename T>
static CCImageBase::SharedPtr tryCast( casacore::LatticeBase * lat)
{
    typedef casacore::ImageInterface<T> CCIT;
    CCIT * cii = dynamic_cast<CCIT *>(lat);
    typename CCImage<T>::SharedPtr res = nullptr;
    if(cii) {
        res = CCImage<T>::create(cii);
    }
    return res;
}

///
/// \brief Attempts to load an image using casacore library, namely the very first
/// frame of it. Then converts the frame to a QImage using 100% histogram clip values.
/// \param fname file name with the image
/// \param result where to store the result
/// \return true if successful, false otherwise
///
Carta::Lib::Image::ImageInterface::SharedPtr CasaImageLoader::loadImage(const QString & fname)
{
    qDebug() << "CasaImageLoader plugin trying to load image: " << fname;

    // load image: casacore::ImageOpener::openPagedImage will emit exception if failed,
    // so need to catch the exception for error handling.
    casacore::LatticeBase * lat = nullptr;
    casacore::ImageOpener::ImageTypes filetype = casacore::ImageOpener::imageType(fname.toStdString());
    try {
        if (filetype != casacore::ImageOpener::ImageTypes::UNKNOWN) {
            if (filetype == casacore::ImageOpener::ImageTypes::AIPSPP) {
                casa_mutex.lock();
                lat = casacore::ImageOpener::openPagedImage(fname.toStdString());
                casa_mutex.unlock();
            } else if (filetype == casacore::ImageOpener::ImageTypes::HDF5) {
                casa_mutex.lock();
                lat = casacore::ImageOpener::openHDF5Image(fname.toStdString());
                casa_mutex.unlock();
            } else {
                casa_mutex.lock();
                lat = casacore::ImageOpener::openImage(fname.toStdString());
                casa_mutex.unlock();
            }
        } else {
            qWarning() << "unknow format \t-out of ideas, bailing out";
            return nullptr;
        }
    } catch (std::exception& e) {
        qWarning() << "Open image failed: exception " << e.what();
        return nullptr;
    }

    if (nullptr == lat) {
        qWarning() << "Open image failed: unknown image type to casacore.";
        return nullptr;
    }

    lat->reopen();
    qDebug() << "lat.shape = " << std::string(lat->shape().toString()).c_str() << "lat.dataType = " << lat->dataType();

    CCImageBase::SharedPtr res = tryCast<float>(lat);
    // Please note that the following code will not be reached
    // even if the FITS file is defined in 64 bit
    // and FitsHeaderExtractor::_CasaFitsConverter assumes that
    // image is load in casacore::ImageInterface < casacore::Float > format
    if(!res) res = tryCast<double>(lat);
    if(!res) res = tryCast<u_int8_t>(lat);
    if(!res) res = tryCast<int16_t>(lat);
    if(!res) res = tryCast<int32_t>(lat);
    if(!res) res = tryCast<casacore::Int>(lat);

    if(!res) {
        qWarning() << "Unsupported lattice type:" << lat-> dataType();
        delete lat;
        return nullptr;
    }

    return res;
}

CasaImageLoader::~CasaImageLoader()
{
}
