#include "volume_quick.h"
#include <QKeyEvent>

#include <iostream>

VolumeWindow::VolumeWindow(QQuickWindow *parent)
	: QQuickWindow(parent), VolumeRenderer() {
	setSurfaceType(QWindow::OpenGLSurface);
	reset();
}

VolumeWindow::~VolumeWindow() {
}

bool VolumeWindow::event(QEvent *event) {
	if (isExposed()) {
		switch (event->type()) {
			default:
				break;

			case QEvent::Resize:
				_resetView = true;
				// fall through

			case QEvent::Expose:
			case QEvent::UpdateRequest:
				renderGl(nullptr);
				return true;
		}
	}
	return isActive() && QQuickWindow::event(event);
}

void VolumeWindow::mousePressEvent(QMouseEvent *event) {
	mouse = event->globalPos();
}

void VolumeWindow::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() == Qt::NoButton) {
		return;
	}
	QPoint pos = event->globalPos();
	int dx = mouse.x() - pos.x();
	int dy = mouse.y() - pos.y();
	mouse = pos;

	emit mouseDrag(event->buttons(), dx, dy);
}

void VolumeWindow::wheelEvent(QWheelEvent *event) {
	emit mouseScroll(event->angleDelta().y() / (qreal)abs(event->delta()));
}

void VolumeWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	float pos[4] = {-1, -1, -1, -1};
	if (this->model != nullptr) {
		if (event->button() == Qt::LeftButton) {
			this->model->updateVolume(this, VolumeData::Positions, nullptr);
		}
		pos[0] = event->pos().x();
		pos[1] = event->pos().y();
		renderGl(nullptr, pos);
	}
	emit mouseSelect(event->button(), pos[0], pos[1], pos[2]);
}

void VolumeWindow::keyPressEvent(QKeyEvent *event) {
	switch (event->key()) {
		default:
			break;

		case Qt::Key_Escape:
			this->close();
			break;
	}
}
