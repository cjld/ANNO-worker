#-------------------------------------------------
#
# Project created by QtCreator 2016-04-30T15:37:52
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = anno_worker
TEMPLATE = app


SOURCES += main.cpp\
    ./GMM/CmCv.cpp \
    ./GMM/CmDefinition.cpp \
    ./GMM/CmGMM_.cpp \
    ./GMM/CmLog.cpp \
    ./GMM/CmSetting.cpp \
    ./GMM/CmShow.cpp \
    ./GMM/CmUFset.cpp \
    ./graphCut/adjacency_list/graph.cpp \
    ./graphCut/adjacency_list/maxflow.cpp \
    ./graphCut/graph.cpp \
    ./graphCut/maxflow.cpp \
    ./GMM/CmFile.cpp \
    mywindow.cpp \
    form.cpp \
    multilevel.cpp

HEADERS  += \
    ./GMM/CmCv.h \
    ./GMM/CmDefinition.h \
    ./GMM/CmGaussian.h \
    ./GMM/CmGMM_.h \
    ./GMM/CmLog.h \
    ./GMM/CmSetting.h \
    ./GMM/CmShow.h \
    ./GMM/CmUFSet.h \
    ./graphCut/adjacency_list/block.h \
    ./graphCut/adjacency_list/graph.h \
    ./graphCut/block.h \
	./graphCut/instances.inc \
    ./graphCut/graph.h \
    ./GMM/CmFile.h \
    jsoncons/json.hpp \
    jsoncons/json_deserializer.hpp \
    jsoncons/json_error_category.hpp \
    jsoncons/json_filter.hpp \
    jsoncons/json_input_handler.hpp \
    jsoncons/json_output_handler.hpp \
    jsoncons/json_parser.hpp \
    jsoncons/json_reader.hpp \
    jsoncons/json_serializer.hpp \
    jsoncons/json_structures.hpp \
    jsoncons/json_type_traits.hpp \
    jsoncons/jsoncons.hpp \
    jsoncons/jsoncons_config.hpp \
    jsoncons/jsoncons_io.hpp \
    jsoncons/output_format.hpp \
    jsoncons/ovectorstream.hpp \
    jsoncons/parse_error_handler.hpp \
    mywindow.h \
    form.h \
    multilevel.h

FORMS    += \
    mywindow.ui \
    form.ui
QMAKE_CXXFLAGS += -std=c++14
linux-g++ {
	QMAKE_CXX = g++-5
	LIBS += -L/usr/local/lib/ -lopencv_stitching -lopencv_objdetect -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_video -lopencv_photo -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core -lopencv_imgcodecs

}

win32 {
        LIBS += -LD:\OpenCV2.4\opencv\build\x64\vc12\lib -lopencv_core2410 -lopencv_imgproc2410 -lopencv_highgui2410
        INCLUDEPATH += D:\OpenCV2.4\opencv\build\include
}

#CONFIG += console debug
