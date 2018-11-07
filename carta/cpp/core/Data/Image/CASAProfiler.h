/*
 * Get spatial/spectral profiles with casacore
 */

#pragma once

#include <casacore/images/Images/ImageInterface.h>

namespace Carta
{
namespace Lib
{

class CASAProfiler
{
public:
    CASAProfiler();
    ~CASAProfiler();

    bool getSpatialProfileData(const std::string& filename, const int x, const int y, 
        const int channel, const int stoke, std::vector<std::vector<float>>& spatialProfiles);

private:
    void getProfileSlicer(casacore::Slicer& latticeSlicer, int x, int y, int channel, int stokes);

    casacore::IPosition imageShape;
    size_t ndims;
    int stokesAxis, chanAxis;
};

} // namespace Lib
} // namespace Carta
