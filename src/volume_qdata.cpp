#include "volume_quick.h"
#include "volume_filter.h"

#include <QRunnable>
#include <QCollator>
#include <QDirIterator>
#include <QFileInfo>
#include <utility>

struct Task : public QRunnable {

	VolumeExecutor &runner;
	const QString operation;
	const function<void()> action;

	Task(VolumeExecutor &runner, QString operation, function<void()> action)
		: runner(runner), operation(std::move(operation)), action(std::move(action)) {
		this->setAutoDelete(true);
	}

public:
	void run() override {
		QElapsedTimer timer;
		try {
			timer.start();
			action();
			emit runner.operationComplete(runner.time(), timer.elapsed(), operation);
		} catch (const std::overflow_error& e) {
			// this executes if f() throws std::overflow_error (same type rule)
			cerr << "overflow_error: " << e.what() << endl;
			emit runner.operationFailed(runner.time(), e.what());
		} catch (const std::runtime_error& e) {
			// this executes if f() throws std::underflow_error (base class rule)
			cerr << "runtime_error: " << e.what() << endl;
			emit runner.operationFailed(runner.time(), e.what());
		} catch (const std::exception& e) {
			// this executes if f() throws std::logic_error (base class rule)
			cerr << "exception: " << e.what() << endl;
			emit runner.operationFailed(runner.time(), e.what());
		} catch (...) {
			// this executes if f() throws std::string or int or any other unrelated type
			cerr << "error: unknown" << endl;
			emit runner.operationFailed(runner.time(), operation);
		}
	}
};

void VolumeExecutor::start(const QString &operation, const function<void()> &action) {
	executor.clear();
	executor.start(new Task(*this, operation, action));
}
void VolumeExecutor::push(const QString &operation, const function<void()> &action) {
	executor.start(new Task(*this, operation, action));
}
bool VolumeExecutor::join(int timeout) {
	return executor.waitForDone(timeout);
}

static bool ends_with(const string& str, const string& end) {
	size_t slen = str.size(), elen = end.size();
	if (slen < elen) return false;
	while (elen) {
		if (str[--slen] != end[--elen]) return false;
	}
	return true;
}

static const float inf = 1e30f;

static void normalize(Volume<float1> &volume, bool useAbs = false) {
	float1 min(+inf);
	float1 max(-inf);
	volume.forEach([&min, &max](float1 &value) {
		if (min.value > value.value) {
			min.value = value.value;
		}
		if (max.value < value.value) {
			max.value = value.value;
		}
	});

	if (useAbs) {
		volume.forEach([&min, &max](float1 &value) {
			value.value = abs(value.value / (max.value - min.value));
		});
	} else {
		volume.forEach([&min, &max](float1 &value) {
			value.value = (value.value - min.value) / (max.value - min.value);
		});
	}
}

void VolumeData::readSlices(const string &path, int width, int height, unsigned blurSize, int slices, const function<void(const string &path, Volume<float1> &volume, int z)> &readSlice) {
	Volume<float1> *resize = &input;

	QFileInfo file(path.c_str());
	vector<QString> files;
	QDirIterator filesIt(file.dir().absolutePath(), QStringList("*." + file.suffix()));
	while (filesIt.hasNext()) {
		files.push_back(filesIt.next());
	}

	if (files.empty()) {
		throw runtime_error("Folder is not a multiple image directory");
	}
	QCollator collator;
	collator.setNumericMode(true);
	sort(files.begin(), files.end(), [&collator](const QString &a, const QString &b) {
		return collator.compare(a, b) < 0;
	});

	if (slices > 0) {
		if (slices < files.size()) {
			slices = files.size();
		}
		if (input.width() != width || input.height() != height || input.depth() != slices) {
			resize = new Volume<float1>(width, height, slices);
		}
	} else {
		slices = input.depth();
	}

	int spacing = (slices - files.size()) / 2;

	input.fill(float1::zero);
	for (unsigned z = 0; z < files.size(); z++) {
		//log() << "reading slice[" << z << " / " << resize->depth() << "]: " << files[z].toStdString();
		readSlice(files[z].toStdString(), *resize, z + spacing);
	}
	log() << "images loaded: " << files.size();

	normalize(*resize);
	log() << "volume normalized: [" << 2 << ", " << 3 << "]";

	if (blurSize > 1) {
		Volume<float1> *blured = new Volume<float1>(resize->width(), resize->height(), resize->depth());
		Kernel<float1>(blurSize).fillGauss(blurSize / 4.).filter(*resize, *blured);
		delete resize;
		resize = blured;
		log() << "volume blured";
	}

	if (resize != &input) {
		resize->resize(input, 1);
		delete resize;
		log() << "volume resized";
	}
}
void VolumeData::onInputChanged() {
	thumbDirty = true;
	emit volumeChanged();
}

void VolumeData::open(const QUrl &qPath, int slices) {
	string path = qPath.toLocalFile().toStdString();
	log() << "open(file: " << path << ", slices: " << slices << ")";
	this->start("open", [this, path, slices]() {
		if (ends_with(path, ".vol")) {
			input.open(path);
			onInputChanged();
			return;
		}

		/*if (ends_with(path, ".nii.gz")) {
			readNIFTI(path, input);
			normalize(input);
			onInputChanged();
			return;
		}

		if (ends_with(path, ".nii")) {
			readNIFTI(path, input);
			normalize(input);
			onInputChanged();
			return;
		}*/

		QImage image;
		if (!image.load(QString(path.c_str()))) {
			throw runtime_error("Failed to open file: " + path);
		}
		int width = image.width();
		int height = image.height();

		log() << "image: (width: " << width << ", height: " << height << ")";

		/*if (ends_with(path, ".png") && image.depth() == 16) {
			try {
				readSlices(path, width, height, 0, slices, readPng);
				onInputChanged();
				return;
			} catch (...) {
				this->operationFailed(time(), "Invalid 16bit grayscale png, reading as an image");
			}
		}// */

		readSlices(path, width, height, 0, slices, [&image, width, height](const string &file, Volume<float1> &volume, int z) {
			if (!image.load(QString(file.c_str()))) {
				throw runtime_error("Failed to open file: " + file);
			}
			if (image.width() != width || image.height() != height) {
				throw runtime_error("Invalid image size: " + file);
			}
			int cx = (volume.width() - width) / 2;
			int cy = (volume.height() - height) / 2;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					QRgb rgb = image.pixel(x, y);
					float1 value = float1(qGray(rgb) / 255.f);
					volume.set(x + cx, y + cy, z, value);
				}
			}
		});
		onInputChanged();
	});
}
void VolumeData::save(const QUrl &qPath) {
	string path = qPath.toLocalFile().toStdString();
	log() << "save(file: " << path << ")";
	this->start("save", [this, path]() {
		if (ends_with(path, ".sparse.vol")) {
			input.save(path, [](float1 voxel) {
				return voxel != float1::zero;
			});
		}
		else if (ends_with(path, ".png")) {
			QImage image(input.width(), input.height(), QImage::Format_Grayscale8);
			string nameNoExt = path.substr(0, path.length() - 4);
			for (int z = 0; z < input.depth(); ++z) {
				for (int y = 0; y < input.height(); ++y) {
					for (int x = 0; x < input.width(); ++x) {
						int value = toByte(input.get(x, y, z).value);
						image.setPixel(x, y, qRgb(value, value, value));
					}
				}
				image.save(QString::asprintf("%s.%04d.png", nameNoExt.c_str(), z));
			}
		}
		else {
			input.save(path);
		}
	});
}

void VolumeData::backup() {
	log() << "backup";
	this->push("backup", [this]() {
		input.resize(saved, 0);
	});
}
void VolumeData::restore() {
	log() << "restore";
	this->start("restore", [this]() {
		saved.resize(input, 0);
		onInputChanged();
	});
}

void VolumeData::threshold(float min, float max, bool normalize) {
	log() << "threshold(min: " << min << ", max: " << max << ", normalize" << normalize << ")";
	this->push("threshold", [this, min, max, normalize]() {
		if (min < max) {
			input.forEach([&](float1 &voxel) {
				if (voxel.value < min || voxel.value > max) {
					voxel = float1::zero;
				}
				else if (normalize) {
					voxel.value = (voxel.value - min) / (max - min);
				}
			});
		}
		else {
			input.forEach([&](float1 &voxel) {
				if (voxel.value < min && voxel.value > max) {
					voxel = float1::zero;
				}
				else if (normalize) {
					if (voxel.value > min) {
						voxel.value -= min - max;
					}
					voxel.value /= 1 - (min - max);
				}
			});
		}
		onInputChanged();
	});
}
void VolumeData::cutCropSphere(float x, float y, float z, float r, bool crop) {
	log() << "cutCropSphere(crop: " << crop << ", radius: " << r <<")";
	this->push("cropSphere", [this, x, y, z, r, crop]() {
		float sx = input.width();
		float sy = input.height();
		float sz = input.depth();
		float R = r * input.depth();
		vector3d cut(x * sx, y * sy, z * sz, 0);
		input.forEach([this, &cut, R, crop](int x, int y, int z) {
			if (crop == (length(vector3d(x, y, z, 0) - cut) >= R)) {
				input.set(x, y, z, float1::zero);
			}
		});
		onInputChanged();
	});
}

void VolumeData::fill(KernelType type, int size, float value) {
	log() << "fill(size: " << size << ", value: " << value << ")";
	this->push("fill", [this, type, size, value]() {
		Kernel<float1> kernel(size);
		switch (type) {

			case Box:
				kernel.fill(float1(value));
				break;

			case Disk:
				kernel.fillDisk(float1(value));
				break;

			case Cross:
				kernel.fillCross(float1(value));
				break;

			case Diamond:
				kernel.fillDiamond(float1(value));
				break;

			case Gauss:
				kernel.fillGauss(value);
				break;
		}
		kernel.resize(input, 0);
		onInputChanged();
	});
}

void VolumeData::filter(FilterType filterType, KernelType kernelType, int kernelSize, float value) {
	log() << "filter(size: " << kernelSize << ", value: " << value << ")";
	this->push("filter", [this, filterType, kernelType, kernelSize, value]() {
		Kernel<float1> kernel(kernelSize);
		switch (kernelType) {

			case Box:
				kernel.fill(float1(value));
				break;

			case Disk:
				kernel.fillDisk(float1(value));
				break;

			case Cross:
				kernel.fillCross(float1(value));
				break;

			case Diamond:
				kernel.fillDiamond(float1(value));
				break;

			case Gauss:
				kernel.fillGauss(value);
				break;
		}

		Volume<float1> temp = input;
		switch (filterType) {

			case Filter:
				kernel.filter(temp, input);
				break;

			case Median:
				kernel.median(temp, input);
				break;

			case Erode:
				kernel.erode(temp, input);
				break;

			case Dilate:
				kernel.dilate(temp, input);
				break;
		}
		onInputChanged();
	});
}
void VolumeData::filter(int size, QList<qreal> values) {
	log() << "filter(size: " << size << ", values: " << values.size() << ")";
	this->push("filter", [this, size, values]() {
		Kernel<float1> kernel(size);
		for (int z = 0; z < size; ++z) {
			for (int y = 0; y < size; ++y) {
				for (int x = 0; x < size; ++x) {
					kernel.set(x, y, z, float1(values[(z * size + y) * size + x]));
				}
			}
		}

		Volume<float1> temp = input;	// make a copy
		kernel.filter(temp, input);

		onInputChanged();
	});
}

// TODO: currently the algorithm is applied to each slice(2D), make it work in 3D
void VolumeData::clahe(int bins, int windowSize, float clipLimit) {
	log() << "clahe(bins: " << bins << ", windowSize: " << windowSize << ", clipLimit: " << clipLimit << ")";
	this->push("clahe", [this, bins, windowSize, clipLimit]() {
		// Setup.
		int *H = new int[bins + 1];
		int *SH = new int[bins + 1];
		int radius = (windowSize - 1) / 2;

		Volume<float1> src = input;	// make a copy
		Volume<float1> &out = input;	// link only

		int width = src.width();
		int height = src.height();
		int slices = src.depth();

		for (int z = 0; z < slices; ++z) {
			int area = 0;
			memset(H, 0, (bins + 1) * sizeof(int));

			for (int y = 0; y < height; ++y) {
				// Find height of addition/subtraction boxes.
				int yMin = max(0, y - radius);
				int yMax = min(height, y + radius + 1);

				// Find height of addition/subtraction boxes.
				for (int x = -radius; x < width + radius; ++x) {

					// Remove pixels on the left edge.
					int subi = x - radius;
					if (subi >= 0) {
						// Create histogram, don't scale.
						for (int jj = yMin; jj < yMax; ++jj) {
							int idx = bins * src.get(subi, jj, z).value;
							H[idx] -= 1;
						}
						// Modify histogram size (for later scaling).
						area -= yMax - yMin;
					}

					// Add pixels on the right edge.
					int addi = x + radius;
					if (addi < width) {
						// Create histogram, don't scale.
						for (int jj = yMin; jj < yMax; ++jj) {
							int idx = bins * src.get(addi, jj, z).value;
							H[idx] += 1;
						}
						// Modify histogram size (for later scaling).
						area += yMax - yMin;
					}

					if (x >= 0 && x < width) {
						// Update pixel value.
						int idx = bins * src.get(x, y, z).value;
						float val = 0;

						// Crop off the top
						float cropped = 0;
						memcpy(SH, H, (bins + 1) * sizeof(int));

						int limit = clipLimit * area / bins;
						for (int l = 0; l < bins; ++l) {
							int d = SH[l] - limit;
							if (d > 0) {
								cropped += d;
								SH[l] = limit;
							}
						}

						// Spread out the cropped area. Generate CDF.
						float spread = cropped / bins;
						for (int l = 0; l < idx; ++l) {
							val += SH[l] + spread;
						}

						// Convert to true CDF value;
						out.set(x, y, z, float1(val / area));
					}
				}
			}
		}
		delete []H;
		delete []SH;
		onInputChanged();
	});
}

void VolumeData::updateVolume(VolumeRenderer *renderer, ViewVolume view, float sphere[4]) {
	// FIXME: start computations on a new thread, try to use OpenGL render queue
	switch (view) {
		case Thumb:
			if (thumbDirty) {
				input.resize(thumb, 1);
				thumbDirty = false;
			}
			renderer->setVolume(this->thumb, sphere);
			break;

		case Input:
			renderer->setVolume(this->input, sphere);
			break;

		case Backup:
			renderer->setVolume(this->saved, sphere);
			break;

		case Output:
			renderer->setVolume(this->result, sphere);
			break;

		case Positions:
			renderer->setPositions(this->input);
			break;
	}
}
