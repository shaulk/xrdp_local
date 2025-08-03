#include "qt.h"
#include "common.h"

#include <sys/mman.h>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPoint>
#include <QThread>
#include <QBitmap>
#include <cstdlib>

static int fake_argc = 1;
static char *fake_argv[] = { (char *)"xrdp_local", nullptr };

// Choose between two implementations of moving screen data from xup to Qt
// USE_COPIES mallocs new buffers, copies the data and returns to xup
// immediately.
// USE_BORROWS blocks the xup client thread until Qt finishes drawing and gives
// Qt a borrowed pointer to the data.
// #define USE_COPIES
#define USE_BORROWS

QtState::QtState(XRDPLocalState *xrdp_local, int max_displays)
{
	this->xrdp_local = xrdp_local;
	this->max_displays = max_displays;

	app = new QApplication(fake_argc, fake_argv);
	window = nullptr;
}

QtState::~QtState()
{
	if (window != nullptr) {
		delete window;
	}
	delete app;
}

void QtState::launch()
{
	full_width = 0;
	full_height = 0;
	for (int i = 0; i < std::min(max_displays, QGuiApplication::screens().count()); i++) {
		QScreen *screen = QGuiApplication::screens().at(i);
		auto geometry = screen->geometry();
		full_width = std::max(full_width, geometry.x() + geometry.width());
		full_height = std::max(full_height, geometry.y() + geometry.height());
	}
	log(LOG_DEBUG, "full_width: %d, full_height: %d\n", full_width, full_height);
	window = new QtWindow(this, full_width, full_height);
	connect(this, &QtState::paint_rect_signal, window, &QtWindow::paint_rect_slot);
}

void QtState::run()
{
	log(LOG_DEBUG, "run running\n");
	app->exec();
	log(LOG_DEBUG, "run done\n");
}

void QtState::paint_rects(int x, int y, unsigned char *data, int srcx, int srcy, int width, int height, int num_rects, xrdp_rect_spec *rects)
{
	if (srcx != 0 || srcy != 0) {
		throw "srcx and srcy must be 0";
	}
	log(LOG_DEBUG, "paint_rect x: %d, y: %d, width: %d, height: %d\n", x, y, width, height);
 #ifdef USE_COPIES
	SyncChangeReference *change = new SyncChangeReference(width, height, data, num_rects, rects);
	emit paint_rect_signal(change, x, y);
#endif
#ifdef USE_BORROWS
	SyncChangeReference change = SyncChangeReference(width, height, data, num_rects, rects);
	emit paint_rect_signal(&change, x, y);
#endif
}

void QtState::set_cursor(int x, int y, unsigned char *data, unsigned char *mask, int width, int height, int bpp)
{
	if (bpp == 32) {
		// Assume ARGB32 with correct alpha
		QImage image = QImage((unsigned char *)data, width, height, QImage::Format_ARGB32).mirrored(false, true);
		QCursor cursor(QPixmap::fromImage(image), x, y);
		window->setCursor(cursor);
		return;
	}

	// Convert an RGB888 image and a monochrome mask to an ARGB32 image with opacity set to the mask
	int Bpp = (bpp == 0) ? 3 : (bpp + 7) / 8;
	uint32_t *data_argb32 = (uint32_t *)malloc(width * height * 4);
	memset(data_argb32, 0, width * height * 4);
	uint32_t bit_mask = 0xFFFFFFFF >> (32 - bpp);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (((mask[(y * width + x) / 8] >> (7 - (x % 8))) & 1) == 0) {
				data_argb32[(height - y - 1) * width + x] = 0xFF000000 | (*(uint32_t *)&data[y * width * Bpp + x * Bpp] & bit_mask);
			}
		}
	}
	QCursor cursor(QPixmap::fromImage(QImage((unsigned char *)data_argb32, width, height, QImage::Format_ARGB32)), x, y);
	window->setCursor(cursor);
}

SyncChangeReference::SyncChangeReference(int width, int height, unsigned char *input_data, int num_rects, xrdp_rect_spec *rects)
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
	release_lock.lock();
#endif
}

SyncChangeReference::~SyncChangeReference()
{
#ifdef USE_COPIES
	free(data);
	free(rects);
#endif
#ifdef USE_BORROWS
	release_lock.lock();
	release_lock.unlock();
#endif
}

void SyncChangeReference::signal_data_is_not_used()
{
#ifdef USE_BORROWS
	release_lock.unlock();
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
	image = QImage(width, height, QImage::Format_RGB32);
	resize(width, height);
	setMouseTracking(true);
	setWindowFlags(Qt::FramelessWindowHint);
	show();
}

QtWindow::~QtWindow() {
}

void QtWindow::paint_rect_slot(SyncChangeReference *change, int x, int y)
{
//	log(LOG_DEBUG, "paint_rect_slot x: %d, y: %d\n", x, y);
	QPainter painter(&this->image);
	QImage new_image(change->get_data(), change->get_width(), change->get_height(), QImage::Format_RGB32);
	// painter.drawImage(QPoint(x, y), new_image);
	int num_rects = change->get_num_rects();
	for (int i = 0; i < num_rects; i++) {
		xrdp_rect_spec *rect = &change->get_rects()[i];
		painter.drawImage(QRect(rect->x, rect->y, rect->cx, rect->cy), new_image, QRect(rect->x, rect->y, rect->cx, rect->cy));
	}
	painter.drawImage(QPoint(x, y), new_image);
	update();
#ifdef USE_COPIES
	delete change;
#endif
#ifdef USE_BORROWS
	change->signal_data_is_not_used();
#endif
}

void QtWindow::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	painter.drawImage(event->rect(), image, event->rect());
}

void QtWindow::mousePressEvent(QMouseEvent *event) {
	log(LOG_DEBUG, "mousePressEvent: %d, %d, qt=%d\n", event->x(), event->y(), event->button());
	qt->get_xrdp_local()->get_xup()->event_mouse_down(event->x(), event->y(), event->button());
}

void QtWindow::mouseReleaseEvent(QMouseEvent *event) {
	log(LOG_DEBUG, "mouseReleaseEvent: %d, %d, qt=%d\n", event->x(), event->y(), event->button());
	qt->get_xrdp_local()->get_xup()->event_mouse_up(event->x(), event->y(), event->button());
}

void QtWindow::mouseMoveEvent(QMouseEvent *event) {
	log(LOG_DEBUG, "mouseMoveEvent: %d, %d\n", event->x(), event->y());
	qt->get_xrdp_local()->get_xup()->event_mouse_move(event->x(), event->y());
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

XRDPLocalState *QtState::get_xrdp_local()
{
	return xrdp_local;
}

DisplayInfo *QtState::get_display_info()
{
	std::vector<display> displays;
	auto screens = QGuiApplication::screens();
	for (int i = 0; i < std::min(screens.count(), max_displays); i++) {
		QScreen *screen = screens.at(i);
		int orientation;
		// RDP uses degrees for orientation
		switch (screen->orientation()) {
			case Qt::LandscapeOrientation:
				orientation = 0;
				break;
			case Qt::PortraitOrientation:
				orientation = 90;
				break;
			case Qt::InvertedLandscapeOrientation:
				orientation = 180;
				break;
			case Qt::InvertedPortraitOrientation:
				orientation = 270;
				break;
			default:
				log(LOG_WARN, "Unknown screen orientation: %d\n", screen->orientation());
				orientation = 0;
		}
		displays.push_back(display(screen->geometry().x(), screen->geometry().y(), screen->geometry().width(), screen->geometry().height(), orientation, screen->physicalSize().width(), screen->physicalSize().height(), screen->refreshRate()));
	}
	return new DisplayInfo(displays);
}

void QtState::exit()
{
	app->quit();
}
