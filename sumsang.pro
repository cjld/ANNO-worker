#-------------------------------------------------
#
# Project created by QtCreator 2016-04-30T15:37:52
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sumsang
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ./zz/GMM/CmCv.cpp \
    ./zz/GMM/CmDefinition.cpp \
    ./zz/GMM/CmGMM_.cpp \
    ./zz/GMM/CmLog.cpp \
    ./zz/GMM/CmSetting.cpp \
    ./zz/GMM/CmShow.cpp \
    ./zz/GMM/CmUFset.cpp \
    ./zz/graphCut/adjacency_list/graph.cpp \
    ./zz/graphCut/adjacency_list/maxflow.cpp \
    ./zz/graphCut/graph.cpp \
    ./zz/graphCut/maxflow.cpp \
    ./zz/abscutout.cpp \
    ./zz/imagecut.cpp \
    ./zz/GMM/CmFile.cpp \
    ./zz/meanshiftSegmentation/allsegs.cpp \
    ./zz/meanshiftSegmentation/ms.cpp \
    ./zz/meanshiftSegmentation/msImageProcessor.cpp \
    ./zz/meanshiftSegmentation/msSys.cpp \
    ./zz/meanshiftSegmentation/RAList.cpp \
    ./zz/meanshiftSegmentation/rlist.cpp \
    cutoutsettings.cpp \
    alphamatting.cpp \
    mywindow.cpp \
    form.cpp \
    multilevel.cpp

HEADERS  += mainwindow.h \
    ./zz/GMM/CmCv.h \
    ./zz/GMM/CmDefinition.h \
    ./zz/GMM/CmGaussian.h \
    ./zz/GMM/CmGMM_.h \
    ./zz/GMM/CmLog.h \
    ./zz/GMM/CmSetting.h \
    ./zz/GMM/CmShow.h \
    ./zz/GMM/CmUFSet.h \
    ./zz/graphCut/adjacency_list/block.h \
    ./zz/graphCut/adjacency_list/graph.h \
    ./zz/graphCut/block.h \
    ./zz/graphCut/graph.h \
    ./zz/abscutout.h \
    ./zz/imagecut.h \
    ./zz/GMM/CmFile.h \
    ./zz/meanshiftSegmentation/allsegs.h \
    ./zz/meanshiftSegmentation/cvHelper.h \
    ./zz/meanshiftSegmentation/ms.h \
    ./zz/meanshiftSegmentation/msImageProcessor.h \
    ./zz/meanshiftSegmentation/msSys.h \
    ./zz/meanshiftSegmentation/RAList.h \
    ./zz/meanshiftSegmentation/rlist.h \
    ./zz/meanshiftSegmentation/tdef.h \
    cutoutsettings.h \
    alphamatting.h \
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

LIBS += -L/usr/local/lib/ -lopencv_stitching -lopencv_objdetect -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_video -lopencv_photo -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core -lopencv_imgcodecs

FORMS    += mainwindow.ui \
    mywindow.ui \
    form.ui
QMAKE_CXXFLAGS += -std=c++14
#CONFIG += console debug
