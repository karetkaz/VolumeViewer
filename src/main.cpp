#include "settings.h"
#include "volume_filter.h"
#include "volume_quick.h"

#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QDirIterator>
#include <QQmlContext>
#include <QCollator>
#include <QStack>

const float1 float1::zero(0);
const float4 float4::zero(0, 0, 0, 0);

static int thumbnailSize = 192;
static int volumeResolution = 512;

VolumeData::VolumeData(): VolumeData(volumeResolution, thumbnailSize) {
	this->start("init", [this]() {
		constexpr int size = 25;
		constexpr double sigma = size / 5.;
		Kernel<float1>(size).fillGauss(sigma, 2, 0, -1).resize(this->input, 0);
		Kernel<float1>(size).fillGauss(sigma, 0, 2, -1).resize(this->saved, 0);
		onInputChanged();
	});
}

int main(int argc, char *argv[]) {
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	QGuiApplication app(argc, argv);
	app.setApplicationName("Volume Viewer");
	app.setOrganizationName("volume_view");
	Settings settings(app.organizationName() + "/config");

	// set default values
	volumeResolution = settings.getValue(nullptr, "volume.resolution", volumeResolution).toInt();
	thumbnailSize = settings.getValue(nullptr, "thumbnail.resolution", thumbnailSize).toInt();

	qmlRegisterType<VolumeWindow>("VolumeRenderer", 1, 0, "Volume3dWindow");
	qmlRegisterType<VolumeData>("VolumeRenderer", 1, 0, "Volume3dData");

	QQmlApplicationEngine engine;
	engine.rootContext()->setContextProperty("settings", &settings);
	engine.rootContext()->setContextProperty("volumeResolution", volumeResolution);
	engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

	if (engine.rootObjects().isEmpty())
		return -1;

	return app.exec();
}
