#ifndef Q3DVOLUME_H
#define Q3DVOLUME_H

#include "math3d.h"

#include "voxel.h"
#include "voxel_float1.h"
#include "voxel_float4.h"

#include "volume.h"
#include "volume_renderer.h"

#include <QThreadPool>
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QQuickItem>

#include <sstream>
#include <ostream>

class VolumeExecutor : public QObject {
	Q_OBJECT

protected:
	QThreadPool executor;
	QElapsedTimer timer;

public:
	VolumeExecutor() {
	}
	
	VolumeExecutor(int threads) {
		executor.setMaxThreadCount(threads);
	}
	
	Q_INVOKABLE qint64 time() {
		return timer.elapsed();
	}
	Q_INVOKABLE void print(const QString &message) {
		cout << message.toStdString() << endl;
	}

	void start(const QString &operation, const function<void()> &action);
	void push(const QString &operation, const function<void()> &action);
	bool join(int timeout = -1);

	inline void push(const function<void()> &action) {
		push("", action);
	}
	
signals:
	void operationStart(qint64 time, QString message);
	void operationComplete(qint64 time, qint64 elapsed, QString message);
	void operationFailed(qint64 time, QString message);
	void volumeChanged();
};

class VolumeData : public VolumeExecutor {
	Q_OBJECT
	Q_PROPERTY(int maxThreads READ maxThreads WRITE maxThreads)

	Volume<float1> thumb;
	Volume<float1> input;
	Volume<float1> saved;
	Volume<float4> result;
	volatile bool thumbDirty = false;

	static constexpr qint64 SEC_MILLIS = 1000;
	static constexpr qint64 MIN_MILLIS = 60 * SEC_MILLIS;
	static constexpr qint64 HOUR_MILLIS = 60 * MIN_MILLIS;
	static constexpr qint64 DAY_MILLIS = 24 * HOUR_MILLIS;

	void readSlices(const string &path, int width, int height, unsigned blurSize, int slices, const function<void(const string &path, Volume<float1> &volume, int z)> &readSlice);
	void onInputChanged();
protected:
	class Logger {
		VolumeData *log;
		const qint64 elapsed;
	public:
		stringbuf *out;

		explicit Logger(VolumeData *self) : Logger(self, -1) {}
		Logger(VolumeData *self, qint64 elapsed)
			: log(self), elapsed(elapsed) {
			out = new stringbuf();
		}
		~Logger() {
			dump();
			delete out;
		}

		void dump() {
			string str = out->str();
			qint64 global = this->log->timer.elapsed();
			if (this->log != nullptr) {
				if (elapsed < 0) {
					emit this->log->operationStart(global, QString::fromStdString(str));
				}
				else {
					emit this->log->operationComplete(global, elapsed, QString::fromStdString(str));
				}
			}
			/*const char *unit = "ms";
			double time = global;
			if (time > DAY_MILLIS) {
				time /= DAY_MILLIS;
				unit = "d";
			}
			else if (time > HOUR_MILLIS) {
				time /= HOUR_MILLIS;
				unit = "h";
			}
			else if (time > MIN_MILLIS) {
				time /= MIN_MILLIS;
				unit = "m";
			}
			else {
				time /= SEC_MILLIS;
				unit = "s";
			}
			cout << "[" << QString::asprintf("%05.1f %s", time, unit).toStdString() << "] "<< str << endl;
			// */
		}

		Logger &operator <<(const string &value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const float &value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const double &value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const bool &value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const signed &value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const unsigned &value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const size_t &value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const char *value) {
			std::ostream(out) << value;
			return *this;
		}
		Logger &operator <<(const QColor &value) {
			std::ostream(out) << std::hex << value.rgb();
			return *this;
		}
	};

protected:
	explicit VolumeData();
	VolumeData(unsigned size, unsigned thumbSize)
		: thumb(thumbSize)
		, input(size, size, size)
		, saved(size, size, size)
		, result(size, size, size) {
		timer.start();
	}
	Logger log() {
		return Logger(this, -1);
	}

	Logger log(qint64 start) {
		return Logger(this, this->timer.elapsed() - start);
	}

public:
	int maxThreads() const { return executor.maxThreadCount(); }
	void maxThreads(int value) { executor.setMaxThreadCount(value); }

	enum ViewVolume {
		Thumb, Input, Backup, Positions, Output
	};
	Q_ENUMS(ViewVolume)

	enum FilterType {
		Filter, Median, Erode, Dilate
	};
	Q_ENUMS(FilterType)

	enum KernelType {
		Box, Disk, Cross, Diamond, Gauss
	};
	Q_ENUMS(KernelType)

	Q_INVOKABLE void fill(KernelType type, int size, float value);
	Q_INVOKABLE void backup();
	Q_INVOKABLE void restore();

	Q_INVOKABLE void open(const QUrl &path, int slices = 0);
	Q_INVOKABLE void save(const QUrl &path);

	Q_INVOKABLE void cutCropSphere(float x, float y, float z, float r, bool crop = true);
	Q_INVOKABLE void threshold(float min, float max = 1.f, bool normalize = false);
	Q_INVOKABLE void filter(FilterType filterType, KernelType kernelType, int kernelSize, float value);
	Q_INVOKABLE void filter(int kernelSize, QList<qreal> values);
	Q_INVOKABLE void clahe(int bins, int windowSize, float clipLimit);

	Q_INVOKABLE void updateVolume(VolumeRenderer *renderer, ViewVolume view, float sphere[4]);
};

class VolumeWindow: public QQuickWindow, protected VolumeRenderer {
	Q_OBJECT
	Q_PROPERTY(VolumeData* model READ renderData WRITE renderData)

	Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE backgroundColor NOTIFY modelChanged)
	Q_PROPERTY(QColor highlightColor READ highlightColor WRITE highlightColor NOTIFY viewChanged)
	Q_PROPERTY(int highlightBorder READ highlightBorder WRITE highlightBorder NOTIFY viewChanged)

	Q_PROPERTY(qreal threshold READ getThreshold WRITE threshold NOTIFY thresholdChanged)
	Q_PROPERTY(qreal alpha READ getAlpha WRITE alpha NOTIFY alphaChanged)

	Q_PROPERTY(qreal plane READ getPlane WRITE plane NOTIFY viewChanged)
	Q_PROPERTY(qreal zoom READ getZoom WRITE zoom NOTIFY viewChanged)

public:
	explicit VolumeWindow(QQuickWindow *parent = nullptr);
	~VolumeWindow();

protected:
	bool event(QEvent *event) override;

	void mousePressEvent(QMouseEvent *) override;
	void mouseMoveEvent(QMouseEvent *) override;
	void wheelEvent(QWheelEvent *) override;
	void mouseDoubleClickEvent(QMouseEvent *) override;

	//	void touchEvent(QTouchEvent *);

	void keyPressEvent(QKeyEvent *) override;
	//	void keyReleaseEvent(QKeyEvent *);

	//	void focusInEvent(QFocusEvent *);
	//	void focusOutEvent(QFocusEvent *);

	//	void showEvent(QShowEvent *);
	//	void hideEvent(QHideEvent *);

	VolumeData* renderData() {
		return this->model;
	}
	void renderData(VolumeData *data) {
		this->model = data;
		if (!this->isExposed()) {
			// no need to draw anything while window is not visible
			return;
		}
		this->model->updateVolume(this, VolumeData::Input, nullptr);
	}


	QColor backgroundColor() const {
		QRgb c = VolumeRenderer::backgroundColor;
		return QColor(qRed(c), qGreen(c), qBlue(c), qAlpha(c));
	}
	void backgroundColor(QColor value) {
		VolumeRenderer::backgroundColor = value.rgba();
		requestRender(SizeChanged);
	}
	
	QColor highlightColor() const {
		QRgb c = VolumeRenderer::highlightColor;
		return QColor(qRed(c), qGreen(c), qBlue(c), qAlpha(c));
	}
	void highlightColor(QColor value) {
		VolumeRenderer::highlightColor = value.rgba();
	}

	int highlightBorder() const { return VolumeRenderer::highlightBorder; }
	void highlightBorder(int value) {
		VolumeRenderer::highlightBorder = value;
	}

	Q_INVOKABLE qreal getThreshold() const { return VolumeRenderer::threshold / 255.; }
	void threshold(qreal value) {
		int intValue = static_cast<int>(value * 255);
		if (VolumeRenderer::threshold != intValue) {
			VolumeRenderer::threshold = intValue;
			emit thresholdChanged();
		}
	}

	Q_INVOKABLE qreal getAlpha() const { return VolumeRenderer::alpha / 255.; }
	void alpha(qreal value) {
		int intValue = static_cast<int>(value * 255);
		if (VolumeRenderer::alpha != intValue) {
			VolumeRenderer::alpha = intValue;
			emit alphaChanged();
		}
	}

	Q_INVOKABLE float getZoom() const { return vol3dZoom; }
	void zoom(float value) {
		if (this->vol3dZoom != value) {
			this->vol3dZoom = value;
			requestRender(SizeChanged);
		}
	}

	Q_INVOKABLE float getPlane() const { return vol3dTranslate; }
	void plane(float value) {
		if (this->vol3dTranslate != value) {
			this->vol3dTranslate = value;
			requestRender(ViewChanged);
		}
	}

	Q_INVOKABLE void rotate(float dx, float dy, float dz = 0) {
		this->vol3dTransform *= rotation(deg2rad(dy), vector3d(1, 0, 0));
		this->vol3dTransform *= rotation(deg2rad(dx), vector3d(0, 1, 0));
		this->vol3dTransform *= rotation(deg2rad(dz), vector3d(0, 0, 1));
		requestRender(ViewChanged);
	}

	Q_INVOKABLE void adjust(float brightness, float contrast, float gamma) {
		VolumeRenderer::adjust(brightness, contrast, gamma);
	}

	Q_INVOKABLE void render(int show) {
		if (!this->isExposed()) {
			// no need to draw anything while window is not visible
			return;
		}
		if (this->model == nullptr) {
			// the model is not yet set
			return;
		}
		this->model->updateVolume(this, (VolumeData::ViewVolume)show, nullptr);
	}

	Q_INVOKABLE void render(int show, qreal threshold) {
		if (!this->isExposed()) {
			// no need to draw anything while window is not visible
			return;
		}
		if (this->model == nullptr) {
			// the model is not yet set
			return;
		}
		int originalValue = VolumeRenderer::threshold;
		VolumeRenderer::threshold = static_cast<int>(threshold * 255);
		this->model->updateVolume(this, (VolumeData::ViewVolume)show, nullptr);
		VolumeRenderer::threshold = originalValue;
	}
	
	Q_INVOKABLE void render(int show, qreal x, qreal y, qreal z, qreal r) {
		if (!this->isExposed()) {
			// no need to draw anything while window is not visible
			return;
		}
		if (this->model == nullptr) {
			// the model is not yet set
			return;
		}
		float sphere[4] = {(float)x, (float)y, (float)z, (float)r};
		this->model->updateVolume(this, (VolumeData::ViewVolume)show, sphere);
	}

	Q_INVOKABLE QList<qreal> getTransform() {
		QList<qreal> result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.append(vol3dTransform[i][j]);
			}
		}
		return result;
	}

	Q_INVOKABLE void setTransform(QList<qreal> values) {
		int pos = 0;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				vol3dTransform[i][j] = values[pos++];
			}
		}
		requestRender(ViewChanged);
	}

protected:
	void requestRender(RenderRequestCause cause) override {
		switch (cause) {
			case ModelChanged:
				emit modelChanged();
				_resetModel = true;
				// fall through

			case SizeChanged:
				_resetView = true;
				// fall through

			case ViewChanged:
				emit viewChanged();
				break;
		}
		requestUpdate();
	}

	QSurface* getSurface() override { return this; }

	VolumeData *model = nullptr;
	QPoint mouse;

signals:
	void thresholdChanged();
	void alphaChanged();

	void viewChanged();		// rotaton/translation: just re-draw the display list
	void modelChanged();	// zoom/model: needs to reconstruct the display list
	void volumeChanged();	// threshold, brightness, etc: needs to recreate 3d texture from model

	void mouseDrag(int btn, int dx, int dy);
	void mouseScroll(qreal dx);
	void mouseSelect(int btn, float r, float g, float b);
	void mouseDblClick(int btn, int dx, int dy);
};

#endif
