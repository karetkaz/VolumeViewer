#ifndef VOLUME_RENDERER_H
#define VOLUME_RENDERER_H

#include "voxel.h"
#include "volume.h"
#include "math3d.h"

#include <QRgb>
#include <QOpenGLFunctions_2_0>

class VolumeRenderer: protected QOpenGLFunctions_2_0 {

	QOpenGLContext *glContext;
	GLuint vol3dTexId;
	GLuint vol3dListId;

	unsigned char *vol3dData;
	size_t vol3dSizeX;
	size_t vol3dSizeY;
	size_t vol3dSizeZ;

	// brightness, contrast, gamma, threshold
	unsigned char lut[256];

protected:
	enum RenderRequestCause {
		ModelChanged,   // update volume texture
		SizeChanged,    // update geometry
		ViewChanged     // update view
	};

	QRgb backgroundColor = qRgba(0, 0, 0, 0);
	QRgb highlightColor = qRgba(0, 255, 255, 255 / 4);
	int highlightBorder = 1;

	matrix3d vol3dTransform;
	float vol3dTranslate;
	float vol3dZoom;

	bool _resetModel;
	bool _resetView;

	int threshold = 0;
	int alpha = 256;

	virtual void renderModel(QSurface *surface, const QRect *roi);
	virtual void requestRender(RenderRequestCause cause) = 0;
	virtual QSurface* getSurface() = 0;

public:
	void initializeOpenGL();
	void renderGl(const QRect *roi, float readPixels[4] = nullptr);

	void reset();
	void adjust(float brightness, float contrast, float gamma);

	VolumeRenderer() {
		glContext = nullptr;
		vol3dTexId = 0;
		vol3dListId = 0;
		vol3dData = nullptr;
		vol3dSizeX = 0;
		vol3dSizeY = 0;
		vol3dSizeZ = 0;
		_resetModel = true;
		_resetView = true;

		this->vol3dTransform = matrix3d(1);
		this->vol3dZoom = 1;
		this->vol3dTranslate = 0;
		for (unsigned i = 0; i < sizeof(lut); ++i) {
			lut[i] = static_cast<byte>(i);
		}
	}
	~VolumeRenderer() override;

	template<typename voxel>
	void setVolume(const Volume<voxel> &volume, float sphere[4]) {
		const Volume<voxel> *vol = &volume;
		if (volume.depth() == 1) {
			Volume<voxel> *temp = new Volume<voxel>(volume.width(), volume.height(), 2);
			volume.resize(*temp, 0);
			vol = temp;
		}
		unsigned char *buffer = vol3dData;

		if (vol->width() != vol3dSizeX || vol->height() != vol3dSizeY || vol->depth() != vol3dSizeZ) {
			delete []vol3dData;
			vol3dSizeX = vol->width();
			vol3dSizeY = vol->height();
			vol3dSizeZ = vol->depth();
			vol3dData = buffer = new unsigned char[vol3dSizeX * vol3dSizeY * vol3dSizeZ * 4];
		}

		int threshold = this->threshold;
		int alpha = this->alpha;
		bool clr = false;

		if (threshold < 0) {
			threshold = -threshold;
			clr = true;
		}
		if (threshold > 255) {
			threshold = 255;
		}

		if (alpha < 0) {
			alpha = -alpha;
			clr = true;
		}
		if (alpha > 255) {
			alpha = 255;
		}

		if (sphere != nullptr) {
			float r = sphere[3] * vol->depth();
			vector3d cut = vector3d(
				sphere[0] * vol->width(),
				sphere[1] * vol->height(),
				sphere[2] * vol->depth(), 0
			);

			for (unsigned z = 0; z < vol3dSizeZ; ++z) {
				for (unsigned y = 0; y < vol3dSizeY; ++y) {
					for (unsigned x = 0; x < vol3dSizeX; ++x) {
						vector3d pos(x, y, z, 0);
						float d = length(pos - cut);
						if (d > r && d < r + highlightBorder) {
							buffer[0] = qRed(highlightColor);
							buffer[1] = qGreen(highlightColor);
							buffer[2] = qBlue(highlightColor);
							buffer[3] = qAlpha(highlightColor);
						}
						else {
							int vox = vol->get(x, y, z).toRGBA(buffer);
							if (vox > threshold) {
								buffer[0] = lut[buffer[0]];
								buffer[1] = lut[buffer[1]];
								buffer[2] = lut[buffer[2]];
								buffer[3] = buffer[3] * alpha >> 8;
							}
							else if (!clr) {
								*(int32_t*)buffer = 0;
							}
							else {
								buffer[3] = 0;
							}
						}
						buffer += 4;
					}
				}
			}
		}
		else {
			for (unsigned z = 0; z < vol3dSizeZ; ++z) {
				for (unsigned y = 0; y < vol3dSizeY; ++y) {
					for (unsigned x = 0; x < vol3dSizeX; ++x) {
						int vox = vol->get(x, y, z).toRGBA(buffer);
						if (vox > threshold) {
							buffer[0] = lut[buffer[0]];
							buffer[1] = lut[buffer[1]];
							buffer[2] = lut[buffer[2]];
							buffer[3] = buffer[3] * alpha >> 8;
						}
						else if (!clr) {
							*(int32_t*)buffer = 0;
						}
						else {
							buffer[3] = 0;
						}
						buffer += 4;
					}
				}
			}
		}

		if (vol != &volume) {
			delete vol;
		}
		requestRender(ModelChanged);
	}

	template<typename voxel>
	void setPositions(const Volume<voxel> &volume) {
		int threshold = this->threshold;
		if (threshold < 0) {
			threshold = -threshold;
		}
		if (threshold > 255) {
			threshold = 255;
		}

		unsigned char *buffer = vol3dData;
		if (volume.width() != vol3dSizeX || volume.height() != vol3dSizeY || volume.depth() != vol3dSizeZ) {
			delete []vol3dData;
			vol3dSizeX = volume.width();
			vol3dSizeY = volume.height();
			vol3dSizeZ = volume.depth();
			vol3dData = buffer = new unsigned char[vol3dSizeX * vol3dSizeY * vol3dSizeZ * 4];
		}

		for (unsigned z = 0; z < vol3dSizeZ; ++z) {
			for (unsigned y = 0; y < vol3dSizeY; ++y) {
				for (unsigned x = 0; x < vol3dSizeX; ++x) {
					voxel vox = volume.get(x, y, z);
					if (vox.toRGBA(buffer) > threshold) {
						buffer[0] = toByte(x / static_cast<float>(vol3dSizeX));
						buffer[1] = toByte(y / static_cast<float>(vol3dSizeY));
						buffer[2] = toByte(z / static_cast<float>(vol3dSizeZ));
						buffer[3] = 255;
					}
					else {
						buffer[0] = toByte(x / static_cast<float>(vol3dSizeX));
						buffer[1] = toByte(y / static_cast<float>(vol3dSizeY));
						buffer[2] = toByte(z / static_cast<float>(vol3dSizeZ));
						buffer[3] = 0;
					}
					if (x == 0 || y == 0 || z == 0 || x == vol3dSizeX - 1 || y == vol3dSizeY - 1 || z == vol3dSizeZ - 1) {
						buffer[3] = 0;
					}
					buffer += 4;
				}
			}
		}
		requestRender(ModelChanged);
	}
};

#endif
