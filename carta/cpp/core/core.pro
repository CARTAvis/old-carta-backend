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
    stable.h \
    CmdLine.h \
    MainConfig.h \
    State/ObjectManager.h \
    State/StateInterface.h \
    State/UtilState.h \
    ImageView.h \
    Data/DataLoader.h \
    Data/Error/ErrorReport.h \
    Data/Error/ErrorManager.h \
    Data/ILinkable.h \
    Data/Settings.h \
    Data/Image/Controller.h \
    Data/Image/DataFactory.h \
    Data/Image/LayerGroup.h \
    Data/Image/Stack.h \
    Data/Image/Layer.h \
    Data/Image/LayerData.h \
    Data/Image/Contour/Contour.h \
    Data/Image/Contour/ContourControls.h \
    Data/Image/Contour/ContourGenerateModes.h \
    Data/Image/Contour/ContourTypes.h \
    Data/Image/Contour/ContourSpacingModes.h \
    Data/Image/Contour/ContourStyles.h \
    Data/Image/Contour/DataContours.h \
    Data/Image/Contour/GeneratorState.h \
    Data/Image/CoordinateSystems.h \
    Data/Image/DataSource.h \
    Data/Image/LayerCompositionModes.h \
    Data/Image/Render/RenderRequest.h \
    Data/Image/Render/RenderResponse.h \
    Data/Selection.h \
    Data/LinkableImpl.h \
    Data/Region/Region.h \
    Data/Region/RegionControls.h \
    Data/Region/RegionPolygon.h \
    Data/Region/RegionEllipse.h \
    Data/Region/RegionPoint.h \
    Data/Region/RegionRectangle.h \
    Data/Region/RegionFactory.h \
    Data/Region/RegionTypes.h \
    Data/Statistics/Statistics.h \
    Data/Units/UnitsFrequency.h \
    Data/Units/UnitsIntensity.h \
    Data/Units/UnitsSpectral.h \
    Data/Units/UnitsWavelength.h \
    Data/Util.h \
    Data/ViewManager.h \
    Data/ViewPlugins.h \
    Data/FitsHeaderExtractor.h \
    ImageRenderService.h \
    Shape/ControlPoint.h \
    Shape/ControlPointEditable.h \
    Shape/ShapeBase.h \
    Shape/ShapeEllipse.h \
    Shape/ShapePoint.h \
    Shape/ShapePolygon.h \
    Shape/ShapeRectangle.h \
    Algorithms/percentileAlgorithms.h \
    Algorithms/percentileManku99.h \
    DefaultContourGeneratorService.h \
    coreMain.h \
    SimpleRemoteVGView.h

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
    Data/Settings.cpp \
    Data/Image/Controller.cpp \
    Data/Image/DataFactory.cpp \
    Data/Image/LayerData.cpp \
    Data/Image/Layer.cpp \
    Data/Image/LayerGroup.cpp \
    Data/Image/Stack.cpp \
    Data/Image/Contour/Contour.cpp \
    Data/Image/Contour/ContourControls.cpp \
    Data/Image/Contour/ContourGenerateModes.cpp \
    Data/Image/Contour/ContourTypes.cpp \
    Data/Image/Contour/ContourSpacingModes.cpp \
    Data/Image/Contour/ContourStyles.cpp \
    Data/Image/Contour/DataContours.cpp \
    Data/Image/Contour/GeneratorState.cpp \
    Data/Image/CoordinateSystems.cpp \
    Data/Image/DataSource.cpp \
    Data/Image/LayerCompositionModes.cpp \
    Data/Image/Render/RenderRequest.cpp \
    Data/Image/Render/RenderResponse.cpp \
    Data/DataLoader.cpp \
    Data/Error/ErrorReport.cpp \
    Data/Error/ErrorManager.cpp \
    Data/LinkableImpl.cpp \
    Data/Selection.cpp \
    Data/Region/Region.cpp \
    Data/Region/RegionControls.cpp \
    Data/Region/RegionPoint.cpp \
    Data/Region/RegionPolygon.cpp \
    Data/Region/RegionEllipse.cpp \
    Data/Region/RegionFactory.cpp \
    Data/Region/RegionRectangle.cpp \
    Data/Region/RegionTypes.cpp \
    Data/Statistics/Statistics.cpp \
    Data/Units/UnitsFrequency.cpp \
    Data/Units/UnitsIntensity.cpp \
    Data/Units/UnitsSpectral.cpp \
    Data/Units/UnitsWavelength.cpp \
    Data/Util.cpp \
    Data/ViewManager.cpp \
    Data/ViewPlugins.cpp \
    Data/FitsHeaderExtractor.cpp \
    Shape/ControlPoint.cpp \
    Shape/ControlPointEditable.cpp \
    Shape/ShapeBase.cpp \
    Shape/ShapeEllipse.cpp \
    Shape/ShapePoint.cpp \
    Shape/ShapePolygon.cpp \
    Shape/ShapeRectangle.cpp \
    ImageRenderService.cpp \
    Algorithms/percentileAlgorithms.cpp \
    DefaultContourGeneratorService.cpp \
    coreMain.cpp \
    SimpleRemoteVGView.cpp

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
