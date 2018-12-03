/***
 * Represents a group of layers in the stack.
 */

#pragma once
#include "Layer.h"
#include "State/ObjectManager.h"
#include "CartaLib/IImage.h"
#include "CartaLib/IntensityUnitConverter.h"

namespace Carta {

namespace Data {

class LayerData;

class LayerGroup : public Layer {

Q_OBJECT

public:

    static const QString CLASS_NAME;
    static const QString COMPOSITION_MODE;

    virtual ~LayerGroup();

signals:
    void frameChanged(  Carta::Lib::AxisInfo::KnownType axis);

    void removeLayer( Layer* group);

    void viewLoad( );

protected:

    /**
     * Add a data layer to the group.
     * @param fileName - path to the image file.
     * @param success - set to true if the image file is successfully loaded.
     * @param stackIndex - set to the index of the image in this group if it is loaded
     *      in this group.
     */
    QString _addData(const QString& fileName, bool* success, int* stackIndex, int fileId);

    virtual bool _addGroup();

    /**
     * Add a layer to this one at the given index.
     * @param layer - the layer to add.
     * @param targetIndex - the index for the new layer.
     */
    virtual void _addLayer( std::shared_ptr<Layer> layer, int targetIndex = -1 ) Q_DECL_OVERRIDE;

    virtual bool _closeData( const QString& id ) Q_DECL_OVERRIDE;

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
     * @return - a list of frame counts for the current image in each dimension.
     */
    virtual std::vector<int> _getImageDimensions( ) const Q_DECL_OVERRIDE;

    virtual int _getIndexCurrent( ) const;

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
    virtual QString _getPixelValue( double x, double y,
            const std::vector<int>& frames ) const Q_DECL_OVERRIDE;

    /**
     *  Constructor.
     */
    LayerGroup( const QString& path, const QString& id );
    LayerGroup(const QString& className, const QString& path, const QString& id);

    static const QString LAYERS;

    QList<std::shared_ptr<Layer> > m_children;


protected slots:

private slots:

private:

    void _clearData();

    void _removeData( int index );

    class Factory;
    static bool m_registered;

    LayerGroup(const LayerGroup& other);
    LayerGroup& operator=(const LayerGroup& other);
};
}
}
