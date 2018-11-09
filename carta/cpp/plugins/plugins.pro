TEMPLATE = subdirs
#CONFIG += ordered

SUBDIRS += casaCore
SUBDIRS += CasaImageLoader
SUBDIRS += Fitter1D
SUBDIRS += Histogram
SUBDIRS += ImageAnalysis
SUBDIRS += RegionCASA
SUBDIRS += ProfileCASA
SUBDIRS += CyberSKA
SUBDIRS += DevIntegration
SUBDIRS += PCacheSqlite3

unix:macx {
# Disable PCacheLevelDB plugin on Mac, since build carta error with LevelDB enabled.
}
else{
  SUBDIRS += PCacheLevelDB
}
