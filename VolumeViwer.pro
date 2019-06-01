QT += qml quick 3drender
!no_desktop: QT += widgets

CONFIG += c++11

HEADERS += \
	src/math3d.h \
	src/settings.h \
	src/volume.h \
	src/volume_filter.h \
	src/volume_renderer.h \
	src/volume_quick.h \
	src/voxel.h \
	src/voxel_float1.h \
	src/voxel_float4.h

SOURCES += \
	#src/volume_image.cpp \
	src/volume_renderer.cpp \
	src/volume_qdata.cpp \
	src/volume_qwindow.cpp \
	src/main.cpp

OTHER_FILES += \
	qml/main.qml \
	qml/ColorRow.qml \
	qml/SliderRow.qml \
	qml/OperationList.qml \
	qml/OperationItem.qml \
	qml/OperationItemSphere.qml \
	qml/OperationItemValue.qml \
	#data/*.json


RESOURCES += qml.qrc

unix: OTHER_FILES += $$(HOME)/.config/volume_view/config.ini
