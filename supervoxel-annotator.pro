#-------------------------------------------------
#
# Project created by QtCreator 2012-01-12T17:46:21
#
#-------------------------------------------------

QT       += core gui

TARGET = supervoxel-annotator
TEMPLATE = app


SOURCES += main.cpp\
        annotatorwnd.cpp \
    qlabelimage.cpp \
    textinfodialog.cpp \
    regionlistframe.cpp \
    PluginServices.cpp

HEADERS  += annotatorwnd.h \
    qlabelimage.h \
    SuperVoxeler.h \
    Matrix3D.h \
    ColorLists.h \
    FijiHelper.h \
    Region3D.h \
    ShapeStatistics.h \
    textinfodialog.h \
    regionlistframe.h \
    RegionGrowing.h \
    PluginBase.h \
    CommonTypes.h \
    PluginServices.h \
    MiscUtils.h

FORMS    += annotatorwnd.ui \
    textinfodialog.ui \
    regionlistframe.ui

# -- This is a neat trick to export symbols from the executable
#  so the loaded plugin can link to them
QMAKE_CXXFLAGS += -fvisibility=hidden
QMAKE_LFLAGS += -fvisibility=hidden -Wl,--export-dynamic

QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp

# Replace to point to ITK
ITKPATH = /data/phd/software/itk/compile/InsightToolkit-3.20.0
ITKPATH_BUILD = $$ITKPATH/build

# Replace to point to SLIC path
SLICPATH = $$_PRO_FILE_PWD_/../slic

INCLUDEPATH += $$SLICPATH/../

SOURCES += $$SLICPATH/LKM.cpp $$SLICPATH/utils.cpp


#### ITK STUFF

INCLUDEPATH += $$ITKPATH/Code/Review
INCLUDEPATH += $$ITKPATH_BUILD/Code/Review

INCLUDEPATH += $$ITKPATH/Utilities/gdcm/src
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/gdcm/src

INCLUDEPATH += $$ITKPATH/Utilities/gdcm
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/gdcm

INCLUDEPATH += $$ITKPATH/Utilities/vxl/core
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/vxl/core

INCLUDEPATH += $$ITKPATH/Utilities/vxl/vcl
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/vxl/vcl

INCLUDEPATH += $$ITKPATH/Utilities/vxl/v3p/netlib
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/vxl/v3p/netlib

INCLUDEPATH += $$ITKPATH/Utilities/vxl/core
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/vxl/core

INCLUDEPATH += $$ITKPATH/Utilities/vxl/vcl
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/vxl/vcl

INCLUDEPATH += $$ITKPATH/Utilities/vxl/v3p/netlib
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/vxl/v3p/netlib

INCLUDEPATH += $$ITKPATH/Code/Numerics/Statistics
INCLUDEPATH += $$ITKPATH_BUILD/Code/Numerics/Statistics

INCLUDEPATH += $$ITKPATH/Utilities
INCLUDEPATH += $$ITKPATH_BUILD/Utilities

INCLUDEPATH += $$ITKPATH/Utilities/itkExtHdrs
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/itkExtHdrs

INCLUDEPATH += $$ITKPATH/Utilities/nifti/znzlib
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/nifti/znzlib

INCLUDEPATH += $$ITKPATH/Utilities/nifti/niftilib
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/nifti/niftilib

INCLUDEPATH += $$ITKPATH/Utilities/expat
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/expat

INCLUDEPATH += $$ITKPATH/Utilities/DICOMParser
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/DICOMParser

INCLUDEPATH += $$ITKPATH/Utilities/NrrdIO
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/NrrdIO

INCLUDEPATH += $$ITKPATH/Utilities/MetaIO
INCLUDEPATH += $$ITKPATH_BUILD/Utilities/MetaIO

INCLUDEPATH += $$ITKPATH/Code/SpatialObject
INCLUDEPATH += $$ITKPATH_BUILD/Code/SpatialObject

INCLUDEPATH += $$ITKPATH/Code/Numerics/NeuralNetworks
INCLUDEPATH += $$ITKPATH_BUILD/Code/Numerics/NeuralNetworks

INCLUDEPATH += $$ITKPATH/Code/Numerics/FEM
INCLUDEPATH += $$ITKPATH_BUILD/Code/Numerics/FEM

INCLUDEPATH += $$ITKPATH/Code/IO
INCLUDEPATH += $$ITKPATH_BUILD/Code/IO

INCLUDEPATH += $$ITKPATH/Code/Numerics
INCLUDEPATH += $$ITKPATH_BUILD/Code/Numerics

INCLUDEPATH += $$ITKPATH/Code/Common
INCLUDEPATH += $$ITKPATH_BUILD/Code/Common

INCLUDEPATH += $$ITKPATH/Code/BasicFilters
INCLUDEPATH += $$ITKPATH_BUILD/Code/BasicFilters

INCLUDEPATH += $$ITKPATH/Code/Algorithms
INCLUDEPATH += $$ITKPATH_BUILD/Code/Algorithms

INCLUDEPATH += $$ITKPATH/
INCLUDEPATH += $$ITKPATH_BUILD/

LIBS += -L$$ITKPATH_BUILD/bin -lITKIO -lITKStatistics -lITKNrrdIO -litkgdcm -litkjpeg12 -litkjpeg16 -litkopenjpeg -litkpng -litktiff -litkjpeg8 -lITKSpatialObject -lITKMetaIO -lITKDICOMParser -lITKEXPAT -lITKniftiio -lITKznz -litkzlib -lITKCommon -litksys -litkvnl_inst -litkvnl_algo -litkvnl -litkvcl -litkv3p_lsqr -lpthread -lm -ldl -litkNetlibSlatec -litkv3p_netlib
LIBS += -luuid




