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

    bool getSpatialProfileData(const int x, const int y, const int channel, const int stoke,
        std::vector<std::vector<float>>& spatialProfiles) const;

private:
    void getProfileSlicer(casacore::Slicer& latticeSlicer, int x, int y, int channel, int stokes,
        casacore::IPosition imageShape) const;

    std::string m_filename;
    Carta::Lib::FileLoader* m_loader;
};

} // namespace Lib
} // namespace Carta
