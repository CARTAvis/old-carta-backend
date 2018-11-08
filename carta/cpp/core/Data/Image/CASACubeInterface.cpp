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
    auto &dataSet = m_loader->loadData(FileInfo::Data::XYZW);

    m_imageShape = dataSet.shape();
    m_ndims = m_imageShape.size();

    // determine chan and stokes axes
    m_stokesAxis = m_loader->stokesAxis();
    if (m_ndims < 3) m_chanAxis = -1;
    else m_chanAxis = 2;
    if (m_ndims == 4 && m_stokesAxis == 2) m_chanAxis=3;
}

void CASACubeInterface::setImageChannels(const int channel, const int stokes)
{
    if (m_channel != channel || m_stokes != stokes || m_channelCache.empty()) {
        m_channel = channel;
        m_stokes = stokes;

        // prepare cache image
        _getChannelMatrix(m_channelCache, channel, stokes);
    }
}

bool CASACubeInterface::getSpatialProfileData(const int x, const int y, const int channel, const int stokes,
    std::vector<std::vector<float>>& spatialProfiles) const
{
    std::vector<float> profile;
    if (m_stokes == stokes) { // use stored channel matrix
        // x
        spatialProfiles.push_back(m_channelCache.column(y).tovector());
        // y
        spatialProfiles.push_back(m_channelCache.row(x).tovector());
    } else {
        // slice image data
        casacore::Slicer section;
        casacore::Array<float> tmpx, tmpy;

        // x
        _getProfileSlicer(section, -1, y, channel, stokes);
        m_loader->loadData(FileInfo::Data::XYZW).getSlice(tmpx, section, true);
        spatialProfiles.push_back(tmpx.tovector());

        // y
        _getProfileSlicer(section, x, -1, channel, stokes);
        m_loader->loadData(FileInfo::Data::XYZW).getSlice(tmpy, section, true);
        spatialProfiles.push_back(tmpy.tovector());
    }

    return true;
}

void CASACubeInterface::_getChannelMatrix(casacore::Matrix<float>& chanMatrix, int channel, int stokes)
{
    // access already cached image
    if (!m_channelCache.empty() && channel == m_channel && stokes == m_stokes) {
        chanMatrix.reference(m_channelCache);
        return;
    }

    // get new slice image data
    casacore::Slicer section = _getChannelMatrixSlicer(channel, stokes);
    casacore::Array<float> tmp;
    m_loader->loadData(FileInfo::Data::XYZW).getSlice(tmp, section, true);
    chanMatrix.reference(tmp);
}

casacore::Slicer CASACubeInterface::_getChannelMatrixSlicer(int channel, int stokes)
{
    casacore::IPosition count(2, m_imageShape(0), m_imageShape(1));
    casacore::IPosition start(2, 0, 0);

    // slice image data
    if(m_ndims == 3) {
        count.append(casacore::IPosition(1, 1));
        start.append(casacore::IPosition(1, channel));
    } else if (m_ndims == 4) {
        count.append(casacore::IPosition(2, 1, 1));
        if (m_stokesAxis == 2)
            start.append(casacore::IPosition(2, stokes, channel));
        else
            start.append(casacore::IPosition(2, channel, stokes));
    }
    // slice image data
    casacore::Slicer section(start, count);
    return section;
}

void CASACubeInterface::_getProfileSlicer(casacore::Slicer& latticeSlicer, int x, int y, int channel, int stokes) const
{
    // to slice image data along x, y, or channel axis (indicated with -1)
    casacore::IPosition start, count;
    if (x < 0) { // get x profile
        start = casacore::IPosition(1, 0);
        count = casacore::IPosition(1, m_imageShape(0));
    } else {
        start = casacore::IPosition(1, x);
        count = casacore::IPosition(1, 1);
    }

    if (y < 0) { // get y profile
        start.append(casacore::IPosition(1, 0));
        count.append(casacore::IPosition(1, m_imageShape(1)));
    } else {
        start.append(casacore::IPosition(1, y));
        count.append(casacore::IPosition(1, 1));
    }

    if(m_ndims == 3) {
        if (channel < 0) { // get spectral profile
            start.append(casacore::IPosition(1, 0));
            count.append(casacore::IPosition(1, m_imageShape(2)));
        } else {
            start.append(casacore::IPosition(1, channel));
            count.append(casacore::IPosition(1, 1));
        }
    } else if(m_ndims == 4) {
        if (channel < 0) { // get spectral profile with stokes
            if (m_loader->stokesAxis() == 2) {
                start.append(casacore::IPosition(2, stokes, 0));
                count.append(casacore::IPosition(2, 1, m_imageShape(3)));
            } else {
                start.append(casacore::IPosition(2, 0, stokes));
                count.append(casacore::IPosition(2, m_imageShape(2), 1));
            }
        } else {
            if (m_loader->stokesAxis() == 2)
                start.append(casacore::IPosition(2, stokes, channel));
            else
                start.append(casacore::IPosition(2, channel, stokes));
            count.append(casacore::IPosition(2, 1, 1));
        }
    }
    
    latticeSlicer = casacore::Slicer(start, count);
}

CASACubeInterface::~CASACubeInterface()
{
}

} // namespace Lib
} // namespace Carta