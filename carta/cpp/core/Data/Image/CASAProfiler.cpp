#include "CASAProfiler.h"
#include "FileLoader/FileLoader.h"
#include <QDebug>

namespace Carta
{
namespace Lib
{

CASAProfiler::CASAProfiler()
{
}

void CASAProfiler::getProfileSlicer(casacore::Slicer& latticeSlicer, int x, int y, int channel, int stokes) 
{
    // to slice image data along x, y, or channel axis (indicated with -1)
    casacore::IPosition start, count;
    if (x<0) { // get x profile
        start = casacore::IPosition(1,0);
        count = casacore::IPosition(1,imageShape(0));
    } else {
        start = casacore::IPosition(1,x);
        count = casacore::IPosition(1,1);
    }

    if (y<0) { // get y profile
        start.append(casacore::IPosition(1,0));
        count.append(casacore::IPosition(1,imageShape(1)));
    } else {
        start.append(casacore::IPosition(1,y));
        count.append(casacore::IPosition(1,1));
    }

/*
    if(ndims == 3) {
        if (channel<0) { // get spectral profile
            start.append(casacore::IPosition(1,0));
            count.append(casacore::IPosition(1,imageShape(2)));
        } else {
            start.append(casacore::IPosition(1,channel));
            count.append(casacore::IPosition(1,1));
        }
    } else if(ndims==4) {
        if (channel<0) { // get spectral profile with stokes
            if (stokesAxis==2) {
                start.append(casacore::IPosition(2,stokes,0));
                count.append(casacore::IPosition(2,1,imageShape(3)));
            } else {
                start.append(casacore::IPosition(2,0,stokes));
                count.append(casacore::IPosition(2,imageShape(2),1));
            }
        } else {
            if (stokesAxis==2)
                start.append(casacore::IPosition(2,stokes,channel));
            else
                start.append(casacore::IPosition(2,channel,stokes));
            count.append(casacore::IPosition(2,1,1));
        }
    }
    */
    
    latticeSlicer = casacore::Slicer(start, count);
}

bool CASAProfiler::getSpatialProfileData(const std::string& filename, const int x, const int y,
    const int channel, const int stoke, std::vector<std::vector<float>>& spatialProfiles)
{
    Carta::Lib::FileLoader* loader = Carta::Lib::FileLoader::getLoader(filename);
    auto &dataSet = loader->loadData(FileInfo::Data::XYZW);
    imageShape = dataSet.shape();

    // slice image data
    casacore::Slicer section;

    casacore::Array<float> tmpx, tmpy;
    
    // x
    getProfileSlicer(section, -1, y, channel, stoke);
    dataSet.getSlice(tmpx, section, true);
    spatialProfiles.push_back(tmpx.tovector());
  
    // y
    getProfileSlicer(section, x, -1, channel, stoke);
    dataSet.getSlice(tmpy, section, true);
    spatialProfiles.push_back(tmpy.tovector());

    return true;
}

CASAProfiler::~CASAProfiler()
{
}

} // namespace Lib
} // namespace Carta