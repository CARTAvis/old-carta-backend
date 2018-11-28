CARTA backend
======

## Packaged releases
The easiest way to run CARTA (with a backend and frontend) is through our packaged releases available here: https://cartavis.github.io/

## Setting up a CARTA backend development environment

### Dockerfile
The easiest way to prepare a development environment that can build and run the CARTA backend is to use one of our Dockerfiles at this repository: https://github.com/CARTAvis/carta-backend-docker-env

**OR**

### Manually
Here are step by step instructions if you wish to prepare a CARTA backend development environment natively on your Mac or Linux system:

1. We usually build everything inside a directory called "cartawork" `mkdir ~/cartawork && cd ~/cartawork`
2. Clone the carta-backend repository `git clone https://github.com/CARTAvis/carta-backend.git`
2. Install the required 3rd party packages. This process is automated through a script supporting CentOS, Ubuntu, and Mac. Run it from outside the carta-backend directory. `sudo ./carta-backend/carta/scripts/install3party.sh` It will create a `~/cartawork/CARTAvis-external/Thirdparty` directory. Note it requires sudo privalege.
3. Install casacore and code. This process is also automated with a script and requires sudo privalege `sudo ./carta-backend/carta/scripts/buildcasa.sh`
4. Download Qt [here](https://download.qt.io/archive/qt/) and install using the GUI installer. Any version starting from 5.7 is acceptable as that is when QtWebEngine was included as standard. Ensure that you select the QtWebEngine module in order to install it.

The CARTA backend development environment should now be ready.

## To build the CARTA backend

1. `cd  ~/cartawork/carta-backend`
2. Set up the protobuffer submodule `git submodule init && git submodule update`
3. Make the build directory `mkdir build && cd build`
4. Add qmake to the system path, e.g. `export PATH=/qt/5.9.4/gcc_64/bin:$PATH` (Not necessary if using Dockerfile method).
5. `qmake NOSERVER=1 CARTA_BUILD_TYPE=dev ../carta -r` (or to suppress all terminal output, use `qmake NOSERVER=1 CARTA_BUILD_TYPE=release ../carta -r`)
6. `make -j 4`

The CARTA backend should now be ready to run.

## To run the CARTA backend

To run CARTA in this non-packaged state, we need to manually define some library paths (Not necessary if using the Dockerfile method).

For Linux:

`export LD_LIBRARY_PATH=/cartawork/CARTAvis-externals/ThirdParty/casacore/lib:$LD_LIBRARY_PATH`
`export LD_LIBRARY_PATH=/cartawork/CARTAvis-externals/ThirdParty/protobuf/lib:$LD_LIBRARY_PATH`
`export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH`

`cd cpp/desktop/`

Run CARTA `./CARTA`

By default, CARTA uses port 3002, this can be changed by specifying the port flag, e.g. `./CARTA --port=4000`

By default, CARTA looks for images in `~/CARTA/Images`

### Extra: Install ephemerides and geodetic data

In order to open some FITS images, casacore requires the ephemerides and geodetic data. By default, the non-packaged version of CARTA looks for this data in `~/data`.
Paste the following content in your terminal to install:
```
mkdir data ; \
mkdir data/ephemerides ;\
mkdir data/geodetic ; \
svn co https://svn.cv.nrao.edu/svn/casa-data/distro/ephemerides/ data/ephemerides ;\
svn co https://svn.cv.nrao.edu/svn/casa-data/distro/geodetic/ data/geodetic ; \
mv data ~/
```

## CARTA frontend

CARTA needs to connect to a frontend. Instructions to prepare a CARTA frontend can be found at this repository: https://github.com/idia-astro/carta-frontend


## Deployment

To prepare a distributable and packaged version of the carta-backend use information from this repository; https://github.com/CARTAvis/deploytask2018


Explanation of the branches
=======
`master`: Mainstream branch, no development.

`develop`: Development branch. Usually we will merge a feature branch to it and include hot fixes for those features. For each phase of release, we wil merge `develop` to `master`.  

`feature branches`: People develop each feature in a branch whose name can be `peter/drawImage`, or `issue-131` if we use tickets. When it is finished, use `pull request` to proceed code review and then merge to develop. After merging, evaluate those added features on `develop`.

`Fix Bug`: Except for some special cases, such as modifying documents, changing build scripts, or low/no-risk fixes, you will need to commit your bug fixes in Hotfix branch or the original feature branch, then make a pull request to do code review.

Supported systems
=======

Current development platforms:
1. CentOS 7
2. Ubuntu 18.04
3. Mac 10.13

Supported deployment platforms:
1. CentOS 6, 7
2. Ubuntu 16.04, 18.04
3. Mac: OS X El Capitan (10.11), macOS Sierra (10.12), macOS High Sierra (10.13), macOS Mojave (10.14)

Tested c++ compiler: gcc 4.8.5, 8.2 (used by Ubuntu 18.04) & clang on macOS.

CARTA can be built by Qt 5.7 or greater as it includes QtWebEngine

## Third Party Libraries

| Third-party Libraries | Version | license |
| :--- | :--- | :--- |
| casacore | 2.3+ | GPLv2 |
| casa | 5.0.0+ | GPLv2 |
| gfortran (needed by casacore) |  4.8+ | GPLv3 |
| WCSLIB (needed by casacore) | 5.15 | LGPLv3 |
| CFITSIO (need by casacore) | 3.39 | [link \(NASA license\)](https://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/f_user/node9.html) |
| GSL (need by casacore) | 2.3 | GPLv3 |
| flex (need by casacore) | 2.5.37 | [link \(flex license\)](https://raw.githubusercontent.com/westes/flex/master/COPYING) |
| Qt | 5.7+ | LGPLv3 |
| libstdc++ \(Included for CentOS 6\) | 4.8.1+ | [GCC Runtime Exception](https://www.gnu.org/licenses/gcc-exception-3.1-faq.html) |

## CI/CD
Note: These CI scripts require updating

CircleCI (docker): https://circleci.com/gh/CARTAvis/carta

Travis CI (Mac): https://travis-ci.org/CARTAvis/carta/.

Mac auto build repo: https://goo.gl/3pRsjs.

