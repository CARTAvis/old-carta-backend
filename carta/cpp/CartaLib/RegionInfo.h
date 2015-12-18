/**
 * Information for defining a region.
 **/

#pragma once

#include <QString>
#include <vector>

namespace Carta {
namespace Lib {
class RegionInfo {
public:
    /// supported region types in images
    enum class RegionType : int {
        Polygon = 0,
        Ellipse,
        Unknown
    };

    RegionInfo();

    /**
     * Add a point to the region.  For a polygonal region, this may be an additional corner
     * point, for an elliptical region, it may be a point that describes the blc or trc of the
     * bounding box.
     * @corner - an (x,y) pair in radians describing a point in the images.
     */
    void addCorner( std::pair<double,double> corner );

    /**
     * Clear the list of region corners.
     */
    void clearCorners();

    /**
     * Return the corner points that describe the boundaries of the
     * region.
     * @return - the corner points that mark region boundaries.
     */
    std::vector<std::pair<double,double> > getCorners() const;


    /**
     * Return the region type.
     * @return - the type of region (elliptical, polygonal, etc).
     */
    RegionType getRegionType() const;

    /**
     * Set the region type.
     * @param type - the region type.
     */
    void setRegionType( RegionType type );

    virtual ~RegionInfo();
private:
    RegionType m_regionType;
    std::vector<std::pair<double,double> > corners;

};
}
}

