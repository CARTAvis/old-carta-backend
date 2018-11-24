! include(../common.pri) {
  error( "Could not find the common.pri file!" )
}

TEMPLATE = lib

###CONFIG += staticlib
QT += network
QT += xml
QT += concurrent

HEADERS += \
    IConnector.h \
    IPlatform.h \
    Viewer.h \
    IView.h \
    MyQApp.h \
    CallbackList.h \
    PluginManager.h \
    Globals.h \
    Algorithms/Graphs/TopoSort.h \
    CmdLine.h \
    MainConfig.h \
    State/ObjectManager.h \
    State/StateInterface.h \
    State/UtilState.h \
    ImageView.h \
    Data/DataLoader.h \
    Data/Error/ErrorReport.h \
    Data/Error/ErrorManager.h \
    Data/Image/Controller.h \
    Data/Image/DataFactory.h \
    Data/Image/LayerGroup.h \
    Data/Image/Stack.h \
    Data/Image/Layer.h \
    Data/Image/LayerData.h \
    Data/Image/DataSource.h \
    Data/Util.h \
    Data/ViewManager.h \
    Data/ViewPlugins.h \
    Data/FitsHeaderExtractor.h \
    Algorithms/percentileAlgorithms.h \
    coreMain.h

SOURCES += \
    Viewer.cpp \
    MyQApp.cpp \
    CallbackList.cpp \
    PluginManager.cpp \
    Globals.cpp \
    Algorithms/Graphs/TopoSort.cpp \
    CmdLine.cpp \
    MainConfig.cpp \
    State/ObjectManager.cpp\
    State/StateInterface.cpp \
    State/UtilState.cpp \
    ImageView.cpp \
    Data/Image/Controller.cpp \
    Data/Image/DataFactory.cpp \
    Data/Image/LayerData.cpp \
    Data/Image/Layer.cpp \
    Data/Image/LayerGroup.cpp \
    Data/Image/Stack.cpp \
    Data/Image/DataSource.cpp \
    Data/DataLoader.cpp \
    Data/Error/ErrorReport.cpp \
    Data/Error/ErrorManager.cpp \
    Data/Util.cpp \
    Data/ViewManager.cpp \
    Data/ViewPlugins.cpp \
    Data/FitsHeaderExtractor.cpp \
    Algorithms/percentileAlgorithms.cpp \
    coreMain.cpp

#message( "common            PWD=$$PWD")
#message( "common         IN_PWD=$$IN_PWD")
#message( "common _PRO_FILE_PWD_=$$_PRO_FILE_PWD_")
#message( "common        OUT_PWD=$$OUT_PWD")

#CONFIG += precompile_header
#PRECOMPILED_HEADER = stable.h
#QMAKE_CXXFLAGS += -H

INCLUDEPATH += $$absolute_path(../../../ThirdParty/rapidjson/include)

#INCLUDEPATH += ../../../ThirdParty/qwt/include
#LIBS += -L../../../ThirdParty/qwt/lib -lqwt

INCLUDEPATH += ../../../ThirdParty/protobuf/include
LIBS += -L../../../ThirdParty/protobuf/lib -lprotobuf

INCLUDEPATH += ../../../ThirdParty/zfp/include
LIBS += -L../../../ThirdParty/zfp/lib -lzfp

QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/../CartaLib\''

#QWT_ROOT = $$absolute_path("../../../ThirdParty/qwt")
#INCLUDEPATH += $$QWT_ROOT/include

# Add Casacore Libs
casacoreLIBS += -L$${CASACOREDIR}/lib
casacoreLIBS += -lcasa_lattices -lcasa_tables -lcasa_scimath -lcasa_scimath_f -lcasa_mirlib
casacoreLIBS += -lcasa_casa -llapack -lblas -ldl
casacoreLIBS += -lcasa_images -lcasa_coordinates -lcasa_fits -lcasa_measures

LIBS += $${casacoreLIBS}
LIBS += -L$${WCSLIBDIR}/lib -lwcs
LIBS += -L$${CFITSIODIR}/lib -lcfitsio

INCLUDEPATH += $${CASACOREDIR}/include
INCLUDEPATH += $${WCSLIBDIR}/include
INCLUDEPATH += $${CFITSIODIR}/include

INCLUDEPATH += ../../../ThirdParty/wcslib/include
LIBS += -L../../../ThirdParty/wcslib/lib -lwcs

INCLUDEPATH += ../../../ThirdParty/cfitsio/include
LIBS += -L../../../ThirdParty/cfitsio/lib

unix:macx {
#	QMAKE_LFLAGS += '-F$$QWT_ROOT/lib'
#	LIBS +=-L../CartaLib -lCartaLib -framework qwt
        LIBS +=-L../CartaLib -lCartaLib
}
else {
#        QMAKE_LFLAGS += '-Wl,-rpath,\'$$QWT_ROOT/lib\''
#        LIBS +=-L../CartaLib -lCartaLib -L$$QWT_ROOT/lib -lqwt
        LIBS +=-L../CartaLib -lCartaLib
}

DEPENDPATH += $$PROJECT_ROOT/CartaLib
