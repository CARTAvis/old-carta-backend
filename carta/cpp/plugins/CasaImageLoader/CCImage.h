/**
 *
 **/

#pragma once

#include "CartaLib/CartaLib.h"
#include "CartaLib/IImage.h"
#include "CartaLib/AxisInfo.h"
#include "CCRawView.h"
#include "CCMetaDataInterface.h"
#include "casacore/images/Images/ImageInterface.h"
#include "casacore/images/Images/ImageUtilities.h"
#include "casacore/images/Images/TempImage.h"

#include <QDebug>
#include <memory>
#include <set>

#include "CartaLib/UtilCASA.h"

/// helper base class so that we can easily determine if this is a an image
/// interface created by this plugin if we ever want to down-cast it...
class CCImageBase
    : public Carta::Lib::Image::ImageInterface
{
    CLASS_BOILERPLATE( CCImageBase );

public:
//    /// \todo not sure if this is necessary for RTTI to work, if it's not it
//    /// can be removed...
//    virtual bool
//    isCasaImage() const final
//    {
//        return true;
//    }

    /**
     * Returns a pointer to the underlying casacore::LatticeBase* or null if there is no underlying
     * casacore image.
     * @return casacore::LatticeBase *
     */
    virtual casacore::LatticeBase * getCasaImage() = 0;

    virtual casacore::ImageInfo getImageInfo() const = 0;

//    virtual casacore::ImageInterface<casacore::Float> * getCasaIIfloat() = 0;


};

/// implementation of the ImageInterface that the casacore image loader plugin
/// returns to the core
///
/// Note: Because I could not figure out whether CasaCore supports storing the
/// image in a non-templated class while maintaining fast data access, I templated
/// this class as well...
template < typename PType >
class CCImage
    : public CCImageBase
      , public std::enable_shared_from_this < CCImage < PType > >
{
    CLASS_BOILERPLATE( CCImage );

public:
    virtual const Carta::Lib::Unit &
    getPixelUnit() const override
    {
        return m_unit;
    }

    virtual const QString & getType() const override {
        return m_type;
    }

    virtual std::shared_ptr<Carta::Lib::Image::ImageInterface>
    getPermuted(const std::vector<int> & indices ) override{

        //Make sure the passed in indices make sense for this image.
        std::vector<int> imageVect = dims();
        int axisCount = imageVect.size();
        int indexCount = indices.size();
        CARTA_ASSERT( axisCount == indexCount );
        std::set<int> usedIndices;
        for ( int i = 0; i < indexCount; i++ ){
            if ( 0 <= indices[i] && indices[i] < indexCount ){
                int usedCount = usedIndices.count( indices[i] );
                CARTA_ASSERT( usedCount == 0 );
                usedIndices.insert( indices[i]);
            }
        }

        //Convert to a CASA data type.
        casacore::Vector<int> newOrder( indexCount );
        for ( int i = 0; i < indexCount; i++ ){
            newOrder[i] = indices[i];
        }
        //Change the order of the axes in the coordinate system
        casacore::CoordinateSystem coordSys = m_casaII->coordinates();
        coordSys.transpose( newOrder, newOrder );
        casacore::IPosition oldShape = m_casaII->shape();
        casacore::IPosition newShape( indexCount );
        for ( int i = 0; i < indexCount; i++ ){
            newShape[i] = oldShape[newOrder[i]];
        }

        //Make a new image and copy the data into it.
        casacore::ImageInterface<PType>* newImage = new casacore::TempImage<PType>(casacore::TiledShape( newShape), coordSys);
        casacore::Array<PType> dataCopy = m_casaII->get();
        newImage->put( reorderArray( dataCopy, newOrder ));
        if ( m_casaII->hasPixelMask()){
            std::unique_ptr<casacore::Lattice<casacore::Bool> > maskLattice( m_casaII->pixelMask().clone());
            casacore::Array<casacore::Bool> maskCopy = maskLattice->get();
            dynamic_cast< casacore::TempImage<PType> *>(newImage)->attachMask( casacore::ArrayLattice<casacore::Bool>(reorderArray( maskCopy, newOrder )));
        }

        //Finish the copy and create a CARTA image with permuted axes.
        casacore::ImageUtilities::copyMiscellaneous( *newImage, *m_casaII );
        std::shared_ptr<Carta::Lib::Image::ImageInterface> permuteImage =  create( newImage);
        return permuteImage;
    }

    virtual const std::vector < int > &
    dims() const override
    {
        return m_dims;
    }

    virtual bool
    hasMask() const override
    {
        return false;
    }

    virtual bool
    hasBeam() const override
    {
        casacore::ImageInfo imagef = m_casaII->imageInfo();
        return imagef.hasBeam();
    }

    virtual bool
    hasErrorsInfo() const override
    {
        return false;
    }

    /// it is safe to static_cast the instance of CCImageBase to CCImage<T>
    /// based on this type
    virtual Carta::Lib::Image::PixelType
    pixelType() const override
    {
        return m_pixelType;
    }

    virtual Carta::Lib::Image::PixelType
    errorType() const override
    {
        qFatal( "not implemented" );
    }

    virtual Carta::Lib::NdArray::RawViewInterface *
    getDataSlice( const SliceND & sliceInfo ) override
    {
        return new CCRawView < PType > ( this, sliceInfo );
    }

    /// \todo implement this
    virtual Carta::Lib::NdArray::Byte *
    getMaskSlice( const SliceND & sliceInfo) override
    {
        Q_UNUSED( sliceInfo );
        qFatal( "not implemented" );
    }

    /// \todo implement this
    virtual Carta::Lib::NdArray::RawViewInterface *
    getErrorSlice( const SliceND & sliceInfo) override
    {
        Q_UNUSED( sliceInfo );
        qFatal( "not implemented" );
    }

    virtual Carta::Lib::Image::MetaDataInterface::SharedPtr
    metaData() override
    {
        return m_meta;
    }



    /// call this to create an instance of this class, do not use constructor
    static CCImage::SharedPtr
    create( casacore::ImageInterface < PType > * casaImage )
    {

        // create an image interface instance and populate it with various
        // values from casacore::ImageInterface
        CCImage::SharedPtr img = std::make_shared < CCImage < PType > > ();
        //casa_mute.lock();

        img-> m_pixelType = Carta::Lib::Image::CType2PixelType < PType >::type;
        img-> m_dims      = casaImage-> shape().asStdVector();
        img-> m_casaII    = casaImage;
        img-> m_unit      = Carta::Lib::Unit( casaImage-> units().getName().c_str() );
        img-> m_type      = casaImage->imageType().c_str();

        // get title and escape html characters in case there are any
        QString htmlTitle = casaImage->imageInfo().objectName().c_str();
        htmlTitle = htmlTitle.toHtmlEscaped();

        // make our own copy of the coordinate system using 'clone'
        std::shared_ptr<casacore::CoordinateSystem> casaCS(
                    static_cast<casacore::CoordinateSystem *> (casaImage->coordinates().clone()));
        //casa_mute.unlock();

        // construct a meta data instance
        img-> m_meta = std::make_shared < CCMetaDataInterface > ( htmlTitle, casaCS );

        /// \todo remove this test code
       /* casacore::Record rec;
        if( ! casaCS-> save( rec, "")) {
            std::string err = casaCS-> errorMessage();
            qWarning() << "Could not serialize coordinate system";
        }
        else {
            rec.print( std::cerr);
            casacore::AipsIO os("/tmp/file.name", casacore::ByteIO::New);
            rec.putRecord( os);
        }*/

        return img;
    } // create

    virtual casacore::LatticeBase *
    getCasaImage() override
    {
        return m_casaII;
    }

    casacore::ImageInfo getImageInfo() const override{
               return m_casaII->imageInfo();
           }

    virtual
    ~CCImage() {
        if(m_casaII != nullptr)
        {
            delete m_casaII;
            m_casaII = nullptr;
        }

        //qDebug() << "~CCImage is getting called";
    }

    /// do not use this!
    /// \todo constructor should be protected... but I don't have time to fix the
    /// compiler errors (Pavol)
    CCImage()
    {
        m_casaII = nullptr;
    }

protected:
    /// type of the image data
    Carta::Lib::Image::PixelType m_pixelType;

    /// cached dimensions of the image
    std::vector < int > m_dims;

    /// pointer to the actual casacore::ImageInterface
    casacore::ImageInterface < PType > * m_casaII;

    /// cached unit
    Carta::Lib::Unit m_unit;

    /// meta data pointer
    CCMetaDataInterface::SharedPtr m_meta;

    QString m_type;

    /// we want CCRawView to access our internals...
    /// \todo maybe we just need a public accessor, no? I don't like friends :) (Pavol)
    friend class CCRawView < PType >;
};

/// helper to convert carta's image to casacore image interface
casacore::ImageInterface<casacore::Float> *
cartaII2casaII_float( std::shared_ptr<Carta::Lib::Image::ImageInterface> ii) ;
