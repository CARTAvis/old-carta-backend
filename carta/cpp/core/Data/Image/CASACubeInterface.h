/*
 * Get spatial/spectral profiles with casacore
 */

#pragma once

#include "FileLoader/FileLoader.h"
#include <casacore/images/Images/ImageInterface.h>

namespace Carta
{
namespace Lib
{

class CASACubeInterface
{
public:
    CASACubeInterface(const std::string filename);
    ~CASACubeInterface();

    void setImageChannels(const int channel, const int stokes);
    bool prepareCachedImage(const int channel, const int stokes);
    bool getSpatialProfileData(const int x, const int y, const int channel, const int stokes,
        std::vector<std::vector<float>>& spatialProfiles) const;

private:
    std::string m_filename;
    Carta::Lib::FileLoader* m_loader;

    casacore::IPosition m_imageShape; // m_imageShape = (width, height, depth, stokes)
    int m_ndims;

    // set image channel
    int m_channel, m_stokes;
    int m_stokesAxis, m_chanAxis;

    // cache which saves matrix for channelIndex, stokesIndex
    casacore::Matrix<float> m_channelCache;

    void _getChannelMatrix(casacore::Matrix<float>& chanMatrix, int channel, int stokes);
    casacore::Slicer _getChannelMatrixSlicer(int channel, int stokes);
    void _getProfileSlicer(casacore::Slicer& latticeSlicer, int x, int y, int channel, int stokes) const;
};

} // namespace Lib
} // namespace Carta
