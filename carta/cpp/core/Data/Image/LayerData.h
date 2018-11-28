/***
 * Represents a single image layer in the stack.
 */

#pragma once
#include "Layer.h"
#include "State/ObjectManager.h"
#include "CartaLib/IImage.h"
#include "CartaLib/AxisInfo.h"
#include "CartaLib/AxisLabelInfo.h"
#include "CartaLib/VectorGraphics/VGList.h"
#include "CartaLib/IntensityUnitConverter.h"
#include <QImage>
#include <memory>
#include <set>

namespace Carta {
namespace Lib {
    namespace NdArray {
        class RawViewInterface;
    }
}

namespace Data {

class DataSource;

class LayerData : public Layer {

friend class Controller;
friend class LayerGroup;

Q_OBJECT

public:

    static const QString CLASS_NAME;

    virtual ~LayerData();

protected:

    /**
     * Return the data source of the image.
     * @return - the data source of the image.
     */
    virtual std::shared_ptr<DataSource> _getDataSource() Q_DECL_OVERRIDE;

    /**
     * Return the number of frames for the given axis in the image.
     * @param type  - the axis for which a frame count is needed.
     * @return the number of frames for the given axis in the image.
     */
    virtual int _getFrameCount( Carta::Lib::AxisInfo::KnownType type ) const Q_DECL_OVERRIDE;

     /**
      * Returns the underlying image.
      */
     virtual std::shared_ptr<Carta::Lib::Image::ImageInterface> _getImage() Q_DECL_OVERRIDE;

     /**
      * Get the image dimensions.
      * @return - a list containing frame counts for each dimension of the image.
      */
     virtual std::vector<int> _getImageDimensions( ) const Q_DECL_OVERRIDE;

     /**
     * Return percentiles corresponding to the given intensities.
     * @param frameLow a lower bound for the channel range or -1 if there is no lower bound.
     * @param frameHigh an upper bound for the channel range or -1 if there is no upper bound.
     * @param intensities values for which percentiles are needed.
     * @return the percentiles corresponding to the intensities.
     */
     virtual std::vector<double> _getPercentiles( int frameLow, int frameHigh, std::vector<double> intensities, Carta::Lib::IntensityUnitConverter::SharedPtr converter ) const Q_DECL_OVERRIDE;

    /**
     * Return the value of the pixel at (x, y).
     * @param x the x-coordinate of the desired pixel
     * @param y the y-coordinate of the desired pixel.
     * @param frames - list of image frames.
     * @return the value of the pixel at (x, y), or blank if it could not be obtained.
     *
     * Note the xy coordinates are expected to be in casa pixel coordinates, i.e.
     * the CENTER of the left-bottom-most pixel is 0.0,0.0.
     */
    virtual QString _getPixelValue( double x, double y, const std::vector<int>& frames ) const Q_DECL_OVERRIDE;

    virtual QString _getFileName() Q_DECL_OVERRIDE;

    /**
     * Attempts to load an image file.
     * @param fileName - an identifier for the location of the image file.
     * @param success - set to true if the file is successfully loaded.
     * @return - an error message if the file could not be loaded or the id of
     *      the layer if it is successfully loaded.
     */
    virtual QString _setFileName( const QString& fileName, bool* success ) Q_DECL_OVERRIDE;

    /**
     * Returns the histogram of pixels.
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param numberOfBins - the number of histogram bins between minimum and maximum of pixel values.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @param converter - used to convert the pixel values for different unit
     * @return - a struct RegionHistogramData
     */
    virtual PBMSharedPtr _getPixels2Histogram(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
            int numberOfBins,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter) const Q_DECL_OVERRIDE;

    virtual int _getStokeIndicator() const Q_DECL_OVERRIDE;
    virtual int _getSpectralIndicator() const Q_DECL_OVERRIDE;

     /**
     * Returns a spatial profile data
     * @param x - x coordinate of cursor
     * @param y - y coordinate of cursor
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @return - vectors of x profile/y profile.
     */
    virtual PBMSharedPtr _getXYProfiles(int fileId, int x, int y,
            int frameLow, int frameHigh, int stokeFrame,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter) const Q_DECL_OVERRIDE;

    /**
     * Returns success or failed when setting spatial requirements
     * @param region id
     * @param spatial profiles - contains the information of required spatial profiles
     * @return - true or false
     */
    virtual bool _setSpatialRequirements(int fileId, int regionId,
            google::protobuf::RepeatedPtrField<std::string> spatialProfiles) const Q_DECL_OVERRIDE;

    /**
     * Returns success or failed when setting spectral requirements
     * @param region id
     * @param stoke frame
     * @param spectral profiles - contains the information of required spectral profiles
     * @return - true or false
     */
    virtual bool _setSpectralRequirements(int fileId, int regionId, int stokeFrame,
            google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles) const Q_DECL_OVERRIDE;

    /**
     * Returns a spectral profile of a point region
     * @param x - x coordinate of cursor
     * @param y - y coordinate of cursor
     * @param stoke frame
     * @return - a spectral profile
     */
    virtual PBMSharedPtr _getSpectralProfile(int fileId, int x, int y, int stokeFrame) const Q_DECL_OVERRIDE;

    /**
     * Returns a vector of pixels.
     * @param xMin - lower bound of the x-pixel-coordinate.
     * @param xMax - upper bound 0f the x-pixel-coordinate.
     * @param yMin - lower bound of the y-pixel-coordinate.
     * @param yMax - upper bound 0f the y-pixel-coordinate.
     * @param mip - down sampling factor.
     * @param minIntensity - use the minimum of intensity value to replace the NaN type of pixel value.
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @return - vector of pixels.
     */
    virtual PBMSharedPtr _getRasterImageData(int fileId, int xMin, int xMax, int yMin, int yMax, int mip,
        int frameLow, int frameHigh, int stokeFrame,
        bool isZFP, int precision, int numSubsets,
        bool &changeFrame, int regionId, int numberOfBins,
        Lib::IntensityUnitConverter::SharedPtr converter) const Q_DECL_OVERRIDE;

protected slots:

private slots:

private:

    /**
     *  Constructor.
     */
    LayerData( const QString& path, const QString& id );

    class Factory;
    static bool m_registered;

    //Pointer to image interface.
    std::shared_ptr<DataSource> m_dataSource;

    Carta::Lib::VectorGraphics::VGList m_regionGraphics;

    LayerData(const LayerData& other);
    LayerData& operator=(const LayerData& other);
};
}
}
