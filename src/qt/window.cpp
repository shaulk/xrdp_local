#include "common.h"
#include "info.h"
#include "window.h"
#include "state.h"

SyncChangeReference::SyncChangeReference(int width, int height, unsigned char *input_data, int num_rects, xrdp_rect_spec *rects) : release_latch(1)
{
 #ifdef USE_COPIES
	uint64_t size = width * height * 4;
	this->data = (unsigned char *)malloc(size);
	if (this->data == nullptr) {
		log(LOG_ERROR, "malloc failed: %s\n", strerror(errno));
		throw std::runtime_error("malloc failed");
	}
	this->rects = (xrdp_rect_spec *)malloc(num_rects * sizeof(xrdp_rect_spec));
	if (this->rects == nullptr) {
		log(LOG_ERROR, "malloc failed: %s\n", strerror(errno));
		free(this->data);
		throw std::runtime_error("malloc failed");
	}
	memcpy(this->data, input_data, size);
	memcpy(this->rects, rects, num_rects * sizeof(xrdp_rect_spec));
#endif
	this->width = width;
	this->height = height;
	this->num_rects = num_rects;
#ifdef USE_BORROWS
	this->data = input_data;
	this->rects = rects;
#endif
}

SyncChangeReference::~SyncChangeReference()
{
#ifdef USE_COPIES
	free(data);
	free(rects);
#endif
}

void SyncChangeReference::signal_data_is_not_used()
{
#ifdef USE_BORROWS
	release_latch.count_down();
#endif
}

void SyncChangeReference::block_until_data_is_not_used()
{
#ifdef USE_BORROWS
	release_latch.wait();
#endif
}

unsigned char *SyncChangeReference::get_data()
{
	return data;
}

xrdp_rect_spec *SyncChangeReference::get_rects()
{
	return rects;
}

int SyncChangeReference::get_num_rects()
{
	return num_rects;
}

int SyncChangeReference::get_width()
{
	return width;
}

int SyncChangeReference::get_height()
{
	return height;
}

QtWindow::QtWindow(QtState *QtState, int width, int height)
{
	this->qt = QtState;
	framebuffer = QImage(width, height, QImage::Format_RGB32);

	// Make sure the window manager doesn't try to resize us.
	// This is only revelant for debugging, in production there's no window
	// manager so a regular resize would work too.
	setFixedSize(width, height);

	// We want to get mouseMove events
	setMouseTracking(true);

	// We can't be full screen because then our window is just on one screen,
	// so we're frameless at 0x0.
	move(0, 0);
	setWindowFlags(Qt::FramelessWindowHint);

	// Finally, show the window.
	show();
}

QtWindow::~QtWindow() {
}

void QtWindow::paint_rects_slot(SyncChangeReference *change, int x, int y)
{
	try {
		QPainter painter(&this->framebuffer);
		QImage new_image(change->get_data(), change->get_width(), change->get_height(), QImage::Format_RGB32);
		int num_rects = change->get_num_rects();
		for (int i = 0; i < num_rects; i++) {
			xrdp_rect_spec *rect = &change->get_rects()[i];
			painter.drawImage(QRect(rect->x, rect->y, rect->cx, rect->cy), new_image, QRect(rect->x, rect->y, rect->cx, rect->cy));
		}
		update();
	} catch (const std::exception &e) {
		log(LOG_ERROR, "paint_rect_slot caught exception: %s\n", e.what());
		qt->exit();
	}
#ifdef USE_COPIES
	delete change;
#endif
#ifdef USE_BORROWS
	change->signal_data_is_not_used();
#endif
}

void QtWindow::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	painter.drawImage(event->rect(), framebuffer, event->rect());
}

void QtWindow::set_disable_paint(bool disable_paint) {
	setUpdatesEnabled(!disable_paint);
}

int QtWindow::qt_mouse_button_to_xrdp_mouse_button(Qt::MouseButton button) {
	switch (button) {
		case Qt::LeftButton:
			return 1;
		case Qt::RightButton:
			return 2;
		case Qt::MiddleButton:
			return 4;
		case Qt::BackButton:
			return 8;
		case Qt::ForwardButton:
			return 9;
		default:
			return -1;
	}
}
void QtWindow::mousePressEvent(QMouseEvent *event) {
	int x_button = qt_mouse_button_to_xrdp_mouse_button(event->button());
	if (x_button == -1) {
		log(LOG_WARN, "mousePressEvent: Unknown Qt button %d\n", event->button());
		return;
	}

	log(LOG_DEBUG, "mousePressEvent: %d, %d, qt=%d x=%d\n", event->position().x(), event->position().y(), event->button(), x_button);
	qt->get_xrdp_local()->get_xup()->event_mouse_down(event->position().x(), event->position().y(), x_button);
}

void QtWindow::mouseReleaseEvent(QMouseEvent *event) {
	int x_button = qt_mouse_button_to_xrdp_mouse_button(event->button());
	if (x_button == -1) {
		log(LOG_WARN, "mouseReleaseEvent: Unknown Qt button %d\n", event->button());
		return;
	}

	log(LOG_DEBUG, "mouseReleaseEvent: %d, %d, qt=%d x=%d\n", event->position().x(), event->position().y(), event->button(), x_button);
	qt->get_xrdp_local()->get_xup()->event_mouse_up(event->position().x(), event->position().y(), x_button);
}

void QtWindow::mouseMoveEvent(QMouseEvent *event) {
	log(LOG_DEBUG, "mouseMoveEvent: %d, %d\n", event->position().x(), event->position().y());
	qt->get_xrdp_local()->get_xup()->event_mouse_move(event->position().x(), event->position().y());
}

void QtWindow::wheelEvent(QWheelEvent *event) {
	log(LOG_DEBUG, "wheelEvent: %d, %d, %d, %d\n", event->position().x(), event->position().y(), event->pixelDelta().x(), event->pixelDelta().y());
	if (event->pixelDelta().y() != 0) {
		qt->get_xrdp_local()->get_xup()->event_scroll_vertical(event->position().x(), event->position().y(), event->pixelDelta().y() > 0 ? 1 : -1);
	}
	if (event->pixelDelta().x() != 0) {
		qt->get_xrdp_local()->get_xup()->event_scroll_horizontal(event->position().x(), event->position().y(), event->pixelDelta().x() > 0 ? 1 : -1);
	}
}

void QtWindow::keyPressEvent(QKeyEvent *event) {
	log(LOG_DEBUG, "keyPressEvent: native=%d\n", event->nativeScanCode());
	qt->get_xrdp_local()->get_xup()->key_down(event->nativeScanCode());
}

void QtWindow::keyReleaseEvent(QKeyEvent *event) {
	log(LOG_DEBUG, "keyReleaseEvent: native=%d\n", event->nativeScanCode());
	qt->get_xrdp_local()->get_xup()->key_up(event->nativeScanCode());
}
