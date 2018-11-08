#include "CASACubeInterface.h"
#include <QDebug>

namespace Carta
{
namespace Lib
{

CASACubeInterface::CASACubeInterface(const std::string filename)
    : m_filename(filename),
      m_loader(Carta::Lib::FileLoader::getLoader(filename))
{
}

void CASACubeInterface::getProfileSlicer(casacore::Slicer& latticeSlicer, int x, int y, int channel, int stoke,
    casacore::IPosition imageShape) const
{
    size_t ndims = imageShape.size();

    // to slice image data along x, y, or channel axis (indicated with -1)
    casacore::IPosition start, count;
    if (x < 0) { // get x profile
        start = casacore::IPosition(1, 0);
        count = casacore::IPosition(1, imageShape(0));
    } else {
        start = casacore::IPosition(1, x);
        count = casacore::IPosition(1, 1);
    }

    if (y < 0) { // get y profile
        start.append(casacore::IPosition(1, 0));
        count.append(casacore::IPosition(1, imageShape(1)));
    } else {
        start.append(casacore::IPosition(1, y));
        count.append(casacore::IPosition(1, 1));
    }

    if(ndims == 3) {
        if (channel < 0) { // get spectral profile
            start.append(casacore::IPosition(1, 0));
            count.append(casacore::IPosition(1, imageShape(2)));
        } else {
            start.append(casacore::IPosition(1, channel));
            count.append(casacore::IPosition(1, 1));
        }
    } else if(ndims == 4) {
        if (channel < 0) { // get spectral profile with stoke
            if (m_loader->stokesAxis() == 2) {
                start.append(casacore::IPosition(2, stoke, 0));
                count.append(casacore::IPosition(2, 1, imageShape(3)));
            } else {
                start.append(casacore::IPosition(2, 0, stoke));
                count.append(casacore::IPosition(2, imageShape(2), 1));
            }
        } else {
            if (m_loader->stokesAxis() == 2)
                start.append(casacore::IPosition(2, stoke, channel));
            else
                start.append(casacore::IPosition(2, channel, stoke));
            count.append(casacore::IPosition(2, 1, 1));
        }
    }
    
    latticeSlicer = casacore::Slicer(start, count);
}

bool CASACubeInterface::getSpatialProfileData(const int x, const int y, const int channel, const int stoke,
    std::vector<std::vector<float>>& spatialProfiles) const
{
    auto & dataSet = m_loader->loadData(FileInfo::Data::XYZW);
    auto imageShape = dataSet.shape();

    // slice image data
    casacore::Slicer section;
    casacore::Array<float> tmpx, tmpy;
    
    // x
    getProfileSlicer(section, -1, y, channel, stoke, imageShape);
    dataSet.getSlice(tmpx, section, true);
    spatialProfiles.push_back(tmpx.tovector());
  
    // y
    getProfileSlicer(section, x, -1, channel, stoke, imageShape);
    dataSet.getSlice(tmpy, section, true);
    spatialProfiles.push_back(tmpy.tovector());

    return true;
}

CASACubeInterface::~CASACubeInterface()
{
}

} // namespace Lib
} // namespace Carta