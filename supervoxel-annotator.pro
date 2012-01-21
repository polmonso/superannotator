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
    regionlistframe.cpp

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
    RegionGrowing.h

FORMS    += annotatorwnd.ui \
    textinfodialog.ui \
    regionlistframe.ui

QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp

# Replace to point to ITK
ITKPATH = /data/phd/software/itk/compile/InsightToolkit-3.20.0

# Replace to point to SLIC path
SLICPATH = $$_PRO_FILE_PWD_/../slic

INCLUDEPATH += $$SLICPATH/../

SOURCES += $$SLICPATH/LKM.cpp $$SLICPATH/utils.cpp

#### ITK STUFF

INCLUDEPATH += $$ITKPATH/Code/Review
INCLUDEPATH += $$ITKPATH/Utilities/gdcm/src
INCLUDEPATH += $$ITKPATH/Utilities/gdcm
INCLUDEPATH += $$ITKPATH/Utilities/vxl/core
INCLUDEPATH += $$ITKPATH/Utilities/vxl/vcl
INCLUDEPATH += $$ITKPATH/Utilities/vxl/v3p/netlib
INCLUDEPATH += $$ITKPATH/Utilities/vxl/core
INCLUDEPATH += $$ITKPATH/Utilities/vxl/vcl
INCLUDEPATH += $$ITKPATH/Utilities/vxl/v3p/netlib
INCLUDEPATH += $$ITKPATH/Code/Numerics/Statistics
INCLUDEPATH += $$ITKPATH/Utilities
INCLUDEPATH += $$ITKPATH/Utilities
INCLUDEPATH += $$ITKPATH/Utilities/itkExtHdrs
INCLUDEPATH += $$ITKPATH/Utilities/nifti/znzlib
INCLUDEPATH += $$ITKPATH/Utilities/nifti/niftilib
INCLUDEPATH += $$ITKPATH/Utilities/expat
INCLUDEPATH += $$ITKPATH/Utilities/expat
INCLUDEPATH += $$ITKPATH/Utilities/DICOMParser
INCLUDEPATH += $$ITKPATH/Utilities/DICOMParser
INCLUDEPATH += $$ITKPATH/Utilities/NrrdIO
INCLUDEPATH += $$ITKPATH/Utilities/NrrdIO
INCLUDEPATH += $$ITKPATH/Utilities/MetaIO
INCLUDEPATH += $$ITKPATH/Code/SpatialObject
INCLUDEPATH += $$ITKPATH/Code/Numerics/NeuralNetworks
INCLUDEPATH += $$ITKPATH/Code/Numerics/FEM
INCLUDEPATH += $$ITKPATH/Code/IO
INCLUDEPATH += $$ITKPATH/Code/Numerics
INCLUDEPATH += $$ITKPATH/Code/Common
INCLUDEPATH += $$ITKPATH/Code/BasicFilters
INCLUDEPATH += $$ITKPATH/Code/Algorithms
INCLUDEPATH += $$ITKPATH/

LIBS += -L$$ITKPATH/bin -lITKIO -lITKStatistics -lITKNrrdIO -litkgdcm -litkjpeg12 -litkjpeg16 -litkopenjpeg -litkpng -litktiff -litkjpeg8 -lITKSpatialObject -lITKMetaIO -lITKDICOMParser -lITKEXPAT -lITKniftiio -lITKznz -litkzlib -lITKCommon -litksys -litkvnl_inst -litkvnl_algo -litkvnl -litkvcl -litkv3p_lsqr -lpthread -lm -ldl -litkNetlibSlatec -litkv3p_netlib
LIBS += -luuid





















