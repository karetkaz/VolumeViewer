#include "volume_quick.h"

static const float ZOOM = sqrtf(3);

VolumeRenderer::~VolumeRenderer() {
	if (vol3dTexId != 0) {
		this->glDeleteTextures(1, &vol3dTexId);
	}
	if (vol3dListId != 0) {
		this->glDeleteLists(vol3dListId, 1);
	}
	delete []this->vol3dData;
}

void VolumeRenderer::initializeOpenGL() {

	this->glEnable(GL_ALPHA_TEST);
	this->glAlphaFunc(GL_GREATER, 0.05f);

	this->glEnable(GL_BLEND);
	this->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	this->glEnable(GL_TEXTURE_3D);
}

void VolumeRenderer::renderModel(QSurface *surface, const QRect* roi) {
	if (_resetModel) {
		_resetView = true;
		_resetModel = false;
		if (vol3dTexId != 0) {
			this->glDeleteTextures(1, &vol3dTexId);
		}

		if (vol3dData != nullptr) {
			this->glGenTextures(1, &vol3dTexId);
			this->glBindTexture(GL_TEXTURE_3D, vol3dTexId);
			this->glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			this->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			this->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			this->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
			this->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			this->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			this->glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, vol3dSizeX, vol3dSizeY, vol3dSizeZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, vol3dData);
			this->glBindTexture(GL_TEXTURE_3D, 0);
		}
	}

	if (_resetView) {
		_resetView = false;

		this->glClearColor(qRed(this->backgroundColor) / 255.f,
						   qGreen(this->backgroundColor) / 255.f,
						   qBlue(this->backgroundColor) / 255.f, 1.f);

		int x = 0, y = 0;
		int width = surface->size().width();
		int height = surface->size().height();

		if (roi != nullptr) {
			//x = roi->x();
			//y = roi->y();
			width = roi->width();
			height = roi->height();
		}
		this->glViewport(x, y, width, height);

		this->glMatrixMode(GL_PROJECTION);
		this->glLoadIdentity();

		//Set the orthographic projection.
		/*if (width <= height) {
			this->glOrtho(-1, 1, -1 / aspect, 1 / aspect, -3, 3);
		} else {
			this->glOrtho(-aspect, aspect, -1, 1, -3, 3);
		}*/

		GLdouble aspect = width / (GLdouble)height;
		this->glOrtho(-aspect, aspect, 1, -1, 1, -1);
		this->glMatrixMode(GL_MODELVIEW);
		this->glLoadIdentity();

		if (vol3dListId != 0) {
			this->glDeleteLists(vol3dListId, 1);
		}
		vol3dListId = this->glGenLists(1);
		this->glNewList(vol3dListId, GL_COMPILE);
		this->glBindTexture(GL_TEXTURE_3D, vol3dTexId);
		for (size_t i = 0; i < vol3dSizeZ; ++i) {
			float texIdx = i / (float)vol3dSizeZ;
			float vtxIdx = 2 * texIdx - 1;
			this->glBegin(GL_QUADS);
			this->glTexCoord3f(0, 0, texIdx);
			this->glVertex3f(-vol3dZoom, -vol3dZoom, vtxIdx);
			this->glTexCoord3f(1, 0, texIdx);
			this->glVertex3f(vol3dZoom, -vol3dZoom, vtxIdx);
			this->glTexCoord3f(1, 1, texIdx);
			this->glVertex3f(vol3dZoom, vol3dZoom, vtxIdx);
			this->glTexCoord3f(0, 1, texIdx);
			this->glVertex3f(-vol3dZoom, vol3dZoom, vtxIdx);
			this->glEnd();
		}
		this->glEndList();
	}

	if (roi != nullptr) {
		this->glEnable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);

		this->glScissor(roi->x(), roi->y(), roi->width(), roi->height());
	}

	this->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	this->glMatrixMode(GL_TEXTURE);
	this->glLoadIdentity();

	// Translate and make 0.5f as the center
	// (texture coordinate is from 0 to 1. so center of rotation has to be 0.5f)
	this->glTranslatef(+.5f, +.5f, +.5f);

	matrix3d transform = this->vol3dTransform;

	transform *= translate(vol3dTranslate, vector3d(0, 0, ZOOM));
	float matrix[16];
	for (int i = 0; i < 16; ++i) {
		matrix[i] = transform[i % 4][i / 4];
	}
	this->glMultMatrixf(matrix);

	// make sure that volume fits on screen however we rotate it
	this->glScalef(ZOOM, ZOOM, ZOOM);

	this->glTranslatef(-.5f, -.5f, -.5f);

	// render the volume
	this->glCallList(vol3dListId);
	if (roi != nullptr) {
		this->glDisable(GL_SCISSOR_TEST);
	}
}

void VolumeRenderer::reset() {
	this->vol3dTransform = matrix3d(1);
	this->vol3dZoom = 1;
	this->vol3dTranslate = 0;
	for (unsigned i = 0; i < sizeof(lut); ++i) {
		lut[i] = static_cast<byte>(i);
	}
	requestRender(SizeChanged);
}

void VolumeRenderer::adjust(float brightness, float contrast, float gamma) {
	for (int idx = 0; idx < 256; idx += 1) {
		lut[idx] = toByte((pow(idx / 255.f, 1 / gamma) - .5f) * (contrast + 1) + .5f + brightness);
	}
}

void VolumeRenderer::renderGl(const QRect *roi, float readPixels[4]) {
	QSurface *surface = getSurface();
	bool needsInitialize = false;

	if (glContext == nullptr) {
		glContext = new QOpenGLContext();
		glContext->setFormat(surface->format());
		glContext->create();
		needsInitialize = true;
	}

	glContext->makeCurrent(surface);

	if (needsInitialize) {
		initializeOpenGLFunctions();
		initializeOpenGL();
	}

	VolumeRenderer::renderModel(surface, roi);

	if (readPixels != nullptr) {
		int x = readPixels[0];
		int y = surface->size().height() - readPixels[1];
		this->glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, readPixels);
	}
	else {
		glContext->swapBuffers(surface);
	}
}
