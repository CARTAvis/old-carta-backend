/***
 * Meshes together data, selection(s), and view(s).
 */

#pragma once

#include <State/StateInterface.h>
#include <State/ObjectManager.h>
#include "CartaLib/CartaLib.h"
#include "CartaLib/AxisInfo.h"
#include "CartaLib/InputEvents.h"
#include "CartaLib/IntensityUnitConverter.h"
#include "CartaLib/IPercentileCalculator.h"
#include "CartaLib/Proto/region_requirements.pb.h"
#include <QString>
#include <QList>
#include <QObject>
#include <set>

typedef std::shared_ptr<google::protobuf::MessageLite> PBMSharedPtr;

class CoordinateFormatterInterface;

namespace Carta {
    namespace Lib {
        namespace Image {
            class ImageInterface;
        }
        namespace NdArray {
            class RawViewInterface;
        }
    }
}

namespace Carta {
namespace Data {
//class ColorState;
class Layer;
class LayerData;
class Stack;
class DataSource;
class DisplayControls;
//class ContourControls;
class Settings;
//class Region;
//class RegionControls;

typedef Carta::Lib::InputEvents::JsonEvent InputEvent;

class Controller: public QObject, public Carta::State::CartaObject {

    friend class Colormap;
    friend class DataFactory;

    Q_OBJECT

public:

    /**
     * Clear the view.
     */
    void clear();

    /**
     * Add data to this controller.
     * @param fileName the location of the data;
     *        this could represent a url or an absolute path on a local filesystem.
     * @param success - set to true if the data was successfully added.
     * @return - the identifier for the data that was added, if it was added successfully;
     *      otherwise, an error message.
     */
    QString addData(const QString& fileName, bool* success, int fileId);

    // set the file id as the private parameter in the Stack object,
    // which represents the current image shown on the frontend image viewer
    void setFileId(int fileId);

    /**
     * Apply the indicated clips to managed images.
     * @param minIntensityPercentile the minimum clip percentile [0,1].
     * @param maxIntensityPercentile the maximum clip percentile [0,1].
     * @return a QString indicating if there was an error applying the clips or an empty
     *      string if there was not an error.
     */
    QString applyClips( double minIntensityPercentile, double maxIntensityPercentile );

    /**
     * Close the given image.
     * @param id - a stack id for the image to close.
     * @return - an error message if the image was not successfully closed.
     */
    QString closeImage( const QString& id );

    /**
     * Return the auto clip.
     * @return the auto clip.
     */
    bool getAutoClip() const;

    /**
     * Return the minimum clip percentile.
     * @return the minimum clip percentile.
     */
    double getClipPercentileMin() const;

    /**
     * Return the maximum clip percentile.
     * @return the maximum clip percentile.
     */
    double getClipPercentileMax() const;

    /**
     * Return the data source of the selected image.
     * @return - the data source of the selected image.
     */
    std::shared_ptr<DataSource> getDataSource() const;

    /**
     * Return a list of images that have been loaded.
     * @return - a list of loaded images.
     */
    std::vector<std::shared_ptr<Carta::Lib::Image::ImageInterface> > getImages();

    std::shared_ptr<Carta::Lib::Image::ImageInterface> getImage();

    /**
     * Get the image dimensions.
     */
    std::vector<int> getImageDimensions( ) const;

    /**
     * Returns the histogram of pixels.
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param numberOfBins - the number of histogram bins between minimum and maximum of pixel values.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @param converter - used to convert the pixel values for different unit
     * @return - a struct RegionHistogramData
     */
    PBMSharedPtr getPixels2Histogram(int fileId, int regionId, int frameLow, int frameHigh, int stokeFrame,
            int numberOfBins,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter=nullptr) const;

    int getStokeIndicator() const;
    int getSpectralIndicator() const;

    /**
     * Returns a spatial profile data
     * @param x - x coordinate of cursor
     * @param y - y coordinate of cursor
     * @param frameLow - a lower bound for the image channels or -1 if there is no lower bound.
     * @param frameHigh - an upper bound for the image channels or -1 if there is no upper bound.
     * @param stokeFrame - a stoke frame (-1: no stoke, 0: stoke I, 1: stoke Q, 2: stoke U, 3: stoke V)
     * @return - vectors of x profile/y profile.
     */
    PBMSharedPtr getXYProfiles(int fileId, int x, int y,
            int frameLow, int frameHigh, int stokeFrame,
            Carta::Lib::IntensityUnitConverter::SharedPtr converter=nullptr) const;

    /**
     * Returns success or failed when setting spatial requirements
     * @param region id
     * @param spatial profiles - contains the information of required spatial profiles
     * @return - true or false
     */
    bool setSpatialRequirements(int fileId, int regionId,
            google::protobuf::RepeatedPtrField<std::string> spatialProfiles) const;

    /**
     * Returns success or failed when setting spectral requirements
     * @param region id
     * @param stoke frame
     * @param spectral profiles - contains the information of required spectral profiles
     * @return - true or false
     */
    bool setSpectralRequirements(int fileId, int regionId, int stokeFrame,
            google::protobuf::RepeatedPtrField<CARTA::SetSpectralRequirements_SpectralConfig> spectralProfiles) const;

    /**
     * Returns a spectral profile of a point region
     * @param x - x coordinate of cursor
     * @param y - y coordinate of cursor
     * @param stoke frame
     * @return - a spectral profile
     */
    PBMSharedPtr getSpectralProfile(int fileId, int x, int y, int stokeFrame) const;

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
    PBMSharedPtr getRasterImageData(int fileId, int x_min, int x_max, int y_min, int y_max, int mip,
        int frameLow, int frameHigh, int stokeFrame,
        bool isZFP, int precision, int numSubsets,
        bool &changeFrame, int regionId, int numberOfBins,
        Lib::IntensityUnitConverter::SharedPtr converter) const;

    /**
     * Return a list of indices indicating the current frames of the selected
     * image.
     * @return - a list consisting of the current frames of the current image.
     */
    std::vector<int> getImageSlice() const;

    /**
     * Return percentiles corresponding to the given intensities.
     * @param frameLow a lower bound for the channel range or -1 if there is no lower bound.
     * @param frameHigh an upper bound for the channel range or -1 if there is no upper bound.
     * @param intensities values for which percentiles are needed.
     * @return the percentiles corresponding to the intensities.
     */
    std::vector<double> getPercentiles( int frameLow, int frameHigh, std::vector<double> intensities, Carta::Lib::IntensityUnitConverter::SharedPtr converter=nullptr ) const;

    /**
     * Return the region controller.
     * @return - the region controller.
     */
//    std::shared_ptr<RegionControls> getRegionControls();

    /**
     * Return the units of the pixels.
     * @return the units of the pixels, or blank if units could not be obtained.
     */
    QString getPixelUnits() const;

    /**
     * Return the index of the image that is currently at the top of the stack.
     * @return - the index of the current image.
     */
    int getSelectImageIndex() const;

    /**
      * Returns a json string representing the state of this controller.
      * @param type - the type of snapshot to return.
      * @param sessionId - an identifier for the user's session.
      * @return a string representing the state of this controller.
      */
    virtual QString getStateString( const QString& sessionId, SnapshotType type ) const Q_DECL_OVERRIDE;

    /**
     * Force a state refresh.
     */
    virtual void refreshState() Q_DECL_OVERRIDE;

    /**
     * Restore the state from a string representation.
     * @param state- a json representation of state.
     */
    void resetState( const QString& state ) Q_DECL_OVERRIDE;

    /**
     * Reset the images that are loaded and other data associated state.
     * @param state - the data state.
     */
    virtual void resetStateData( const QString& state ) Q_DECL_OVERRIDE;

    virtual ~Controller();

    static const QString CLASS_NAME;
    static const QString PLUGIN_NAME;

signals:

protected:

    virtual QString getSnapType(CartaObject::SnapshotType snapType) const Q_DECL_OVERRIDE;

private:

	/**
	 *  Constructor.
	 */
	Controller( const QString& id, const QString& path );

	class Factory;

	/// Add an image to the stack from a file.
    QString _addDataImage(const QString& fileName, bool* success, int fileId);

	static bool m_registered;

	static const QString CLIP_VALUE_MIN;
	static const QString CLIP_VALUE_MAX;
	static const QString AUTO_CLIP;

//	std::shared_ptr<RegionControls> m_regionControls;

	//Data available to and managed by this controller.
	std::unique_ptr<Stack> m_stack;

	Controller(const Controller& other);
	Controller& operator=(const Controller& other);

};

}
}
