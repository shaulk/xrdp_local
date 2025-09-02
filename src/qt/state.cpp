#include "common.h"
#include "state.h"
#include "window.h"

#include <sys/mman.h>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QPoint>
#include <QThread>
#include <QBitmap>
#include <X11/Xlib.h>

static int fake_argc = 1;
static char *fake_argv[] = { reinterpret_cast<char *>(const_cast<char *>("xrdp_local")), nullptr };

QtState::QtState(XRDPLocalState *xrdp_local, int max_displays, bool use_dma_buf) : app_ready_latch(1)
{
	this->xrdp_local = xrdp_local;
	this->max_displays = max_displays;
	this->use_dma_buf = use_dma_buf;

	QCoreApplication::setAttribute(Qt::AA_Use96Dpi);

	app = new QApplication(fake_argc, fake_argv);
	window = nullptr;

	displays_to_use = QGuiApplication::screens().count();
	log(LOG_DEBUG, "Got %d displays from qt, max set at %d\n", displays_to_use, max_displays);
	if (max_displays > 0) {
		displays_to_use = std::min(max_displays, displays_to_use);
		log(LOG_DEBUG, "Using %d displays\n", displays_to_use);
	}

}

QtState::~QtState()
{
	if (egl != nullptr) {
		delete egl;
	}
	if (window != nullptr) {
		delete window;
	}
	delete app;
}

char *QtState::x11_display() {
	auto x11_app = app->nativeInterface<QNativeInterface::QX11Application>();
	if (x11_app == nullptr) {
		log(LOG_WARN, "QX11Application is not available.");
		return nullptr;
	}
	auto display = x11_app->display();
	if (display == nullptr) {
		log(LOG_WARN, "QX11Application::display() is not available.");
		return nullptr;
	}
	char *displayName = XDisplayString(display);
	if (displayName == nullptr) {
		log(LOG_WARN, "XDisplayString returned nullptr.");
		return nullptr;
	}
	if (strlen(displayName) == 0) {
		log(LOG_WARN, "XDisplayString returned an empty string.");
		return nullptr;
	}
	return displayName;
}

void QtState::launch()
{
	full_width = 0;
	full_height = 0;

	for (int i = 0; i < displays_to_use; i++) {
		QScreen *screen = QGuiApplication::screens().at(i);
		auto geometry = screen->geometry();
		full_width = std::max(full_width, geometry.x() + geometry.width());
		full_height = std::max(full_height, geometry.y() + geometry.height());
		log(LOG_DEBUG, "Display %d at %dx%d, %dx%d\n", i, geometry.x(), geometry.y(), geometry.width(), geometry.height());
	}

	log(LOG_DEBUG, "Initialized Qt with %d displays, full_width: %d, full_height: %d\n", displays_to_use, full_width, full_height);

	window = new QtWindow(this, full_width, full_height);

	connect(this, &QtState::paint_rects_signal, window, &QtWindow::paint_rects_slot);

	// Unblock painting calls
	app_ready_latch.count_down();

	if (use_dma_buf && EGLState::is_supported(x11_display())) {
		xrdp_local->get_xup()->request_dma_buf();
	}
}

void QtState::run()
{
	log(LOG_DEBUG, "run running\n");
	app->exec();
	log(LOG_DEBUG, "run done\n");
}

void QtState::paint_rects(int x, int y, unsigned char *data, int srcx, int srcy, int width, int height, int num_rects, xrdp_rect_spec *rects)
{
	app_ready_latch.wait();
	if (srcx != 0 || srcy != 0) {
		throw std::runtime_error("srcx and srcy must be 0");
	}
	SyncChangeReference change = SyncChangeReference(width, height, data, num_rects, rects);
	emit paint_rects_signal(&change, x, y);
	change.block_until_data_is_not_used();
}

void QtState::set_cursor(int x, int y, unsigned char *data, unsigned char *mask, int width, int height, int bpp)
{
	if (bpp == 32) {
		// Assume ARGB32 with correct alpha (which is what xorgxrdp sends when using full color cursors)
		QImage image = QImage(reinterpret_cast<unsigned char *>(data), width, height, QImage::Format_ARGB32).mirrored(false, true);
		QCursor cursor(QPixmap::fromImage(image), x, y);
		window->setCursor(cursor);
		return;
	}

	// Convert an RGB888 image and a monochrome mask to an ARGB32 image with opacity set to the mask
	int Bpp = (bpp == 0) ? 3 : (bpp + 7) / 8;
	uint32_t *data_argb32 = reinterpret_cast<uint32_t *>(malloc(width * height * 4));
	memset(data_argb32, 0, width * height * 4);
	uint32_t bit_mask = 0xFFFFFFFF >> (32 - bpp);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (((mask[(y * width + x) / 8] >> (7 - (x % 8))) & 1) == 0) {
				data_argb32[(height - y - 1) * width + x] = 0xFF000000 | (*reinterpret_cast<uint32_t *>(&data[y * width * Bpp + x * Bpp]) & bit_mask);
			}
		}
	}
	QCursor cursor(QPixmap::fromImage(QImage(reinterpret_cast<unsigned char *>(data_argb32), width, height, QImage::Format_ARGB32)), x, y);
	window->setCursor(cursor);
	free(data_argb32);
}

XRDPLocalState *QtState::get_xrdp_local()
{
	return xrdp_local;
}

std::unique_ptr<struct display_info> QtState::get_display_info()
{
	std::unique_ptr<struct display_info> display_info = std::make_unique<struct display_info>();
	auto screens = QGuiApplication::screens();
	for (int i = 0; i < std::min(static_cast<int>(screens.size()), displays_to_use); i++) {
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
		display_info->displays.push_back({
			.x = screen->geometry().x(),
			.y = screen->geometry().y(),
			.width = screen->geometry().width(),
			.height = screen->geometry().height(),
			.physical_width = static_cast<int>(screen->physicalSize().width()),
			.physical_height = static_cast<int>(screen->physicalSize().height()),
			.orientation = orientation,
			.refresh_rate = static_cast<int>(screen->refreshRate())
		});
	}
	return display_info;
}


void QtState::paint_dma_buf() {
	if (egl == nullptr) {
		throw std::runtime_error("Can't paint using DMA-BUF: EGLState is not initialized. This is a bug.");
	}
	egl->render();
}

bool QtState::enable_dma_buf(int fd, uint32_t width, uint32_t height, uint16_t stride, uint32_t size, uint32_t format) {
	if (window == nullptr) {
		log(LOG_ERROR, "Can't enable DMA-BUF: Qt window is not initialized. This is a bug.\n");
		return false;
	}
	try {
		egl = new EGLState(x11_display(), window->winId(), fd, width, height, stride, size, format);
		log(LOG_DEBUG, "enable_dma_buf: success\n");
		window->set_disable_paint(true);
		return true;
	} catch (const std::exception &e) {
		log(LOG_ERROR, "enable_dma_buf: %s\n", e.what());
		return false;
	}
}

void QtState::disable_dma_buf() {
	if (egl != nullptr) {
		delete egl;
		egl = nullptr;
	}
	window->set_disable_paint(false);
}

void QtState::exit()
{
	app->quit();
}
