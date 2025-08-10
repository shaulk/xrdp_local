#include "qt.h"
#include "common.h"

#include <sys/mman.h>
#include <QApplication>
#include <QGuiApplication>
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

QtState::QtState(XRDPLocalState *xrdp_local, int max_displays, bool use_dma_buf) : app_ready_latch(1)
{
	this->xrdp_local = xrdp_local;
	this->max_displays = max_displays;
	this->use_dma_buf = use_dma_buf;

	QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

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

int QtState::x11_display() {
	// TODO
	return 0;
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
 #ifdef USE_COPIES
	SyncChangeReference *change = new SyncChangeReference(width, height, data, num_rects, rects);
	emit paint_rects_signal(change, x, y);
#endif
#ifdef USE_BORROWS
	SyncChangeReference change = SyncChangeReference(width, height, data, num_rects, rects);
	emit paint_rects_signal(&change, x, y);
	change.block_until_data_is_not_used();
#endif
}

void QtState::set_cursor(int x, int y, unsigned char *data, unsigned char *mask, int width, int height, int bpp)
{
	if (bpp == 32) {
		// Assume ARGB32 with correct alpha (which is what xorgxrdp sends when using full color cursors)
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
	free(data_argb32);
}

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

bool EGLState::is_supported(intptr_t x11_display) {
	EGLDisplay eglDisplay;

	eglDisplay = eglGetDisplay((EGLNativeDisplayType)x11_display);
	if (eglDisplay == EGL_NO_DISPLAY) {
		log(LOG_DEBUG, "eglGetDisplay failed.\n");
		return false;
	}

	if (eglInitialize(eglDisplay, nullptr, nullptr) == EGL_FALSE) {
		log(LOG_INFO, "EGL is not supported.\n");
		return false;
	}

	eglTerminate(eglDisplay);

	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
		(PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	if (eglCreateImageKHR == nullptr) {
		log(LOG_INFO, "EGL EGL_KHR_gl_texture_2D_image extension not supported, DMA-BUF not supported.\n");
		return false;
	}
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC eglImageTargetTexture2DOES =
		(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
			eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if (eglImageTargetTexture2DOES == nullptr) {
		log(LOG_INFO, "EGL EGL_EXT_image_dma_buf_import extension not supported, DMA-BUF not supported.\n");
		return false;
	}

	log(LOG_INFO, "EGL is supported by system and environment, enabling DMA-BUF.\n");
	return true;
}

EGLState::EGLState(intptr_t x11_display, int window_id, int fd, uint32_t width, uint32_t height, uint16_t stride, uint32_t size, uint32_t format) {
	int numConfigs = 0;

	log(LOG_DEBUG, "EGLState: %d, %d, %d, %d, %d, %d, %d, %X\n", x11_display, window_id, fd, width, height, stride, size, format);

	this->width = width;
	this->height = height;

	int egl_config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_NONE,
	};
	int egl_ctx_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};

	egl_display = eglGetDisplay((EGLNativeDisplayType)x11_display);
	if (egl_display == EGL_NO_DISPLAY) {
		throw std::runtime_error("eglGetDisplay failed");
	}

	if (eglInitialize(egl_display, nullptr, nullptr) == EGL_FALSE) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglInitialize failed");
	}

	if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglBindAPI failed");
	}

	if (eglChooseConfig(egl_display, egl_config_attribs, &egl_config, 1, &numConfigs) == EGL_FALSE) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglChooseConfig failed");
	}

	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_ctx_attribs);
	if (egl_context == EGL_NO_CONTEXT) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglCreateContext failed");
	}

	egl_surface = eglCreateWindowSurface(egl_display, egl_config,
										(EGLNativeWindowType)window_id, nullptr);
	if (egl_surface == EGL_NO_SURFACE) {
		eglDestroyContext(egl_display, egl_context);
		eglTerminate(egl_display);
		throw std::runtime_error("eglCreateWindowSurface failed");
	}

	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

	import_dma_buf_fd(fd, width, height, stride, size, format);
	setup_gl_state();
}

EGLState::~EGLState() {
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (egl_image != EGL_NO_IMAGE_KHR) {
		PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
		if (eglDestroyImageKHR != nullptr) {
			eglDestroyImageKHR(egl_display, egl_image);
			egl_image = EGL_NO_IMAGE_KHR;
		}
	}

	if (texture != 0) {
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	if (egl_surface != EGL_NO_SURFACE) {
		eglDestroySurface(egl_display, egl_surface);
		egl_surface = EGL_NO_SURFACE;
	}

	if (egl_context != EGL_NO_CONTEXT) {
		eglDestroyContext(egl_display, egl_context);
		egl_context = EGL_NO_CONTEXT;
	}

	if (egl_display != EGL_NO_DISPLAY) {
		eglTerminate(egl_display);
		egl_display = EGL_NO_DISPLAY;
	}

	eglReleaseThread();
}

void EGLState::import_dma_buf_fd(int fd, uint32_t width, uint32_t height, uint16_t stride, uint32_t size, uint32_t format) {
	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
		(PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC eglImageTargetTexture2DOES =
		(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
			eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if (eglCreateImageKHR == nullptr || eglImageTargetTexture2DOES == nullptr) {
		throw std::runtime_error("eglCreateImageKHR or eglImageTargetTexture2DOES not supported.");
	}

	EGLint attrs[] = {
		EGL_WIDTH, (EGLint)width,
		EGL_HEIGHT, (EGLint)height,
		EGL_LINUX_DRM_FOURCC_EXT, (EGLint)format,
		EGL_DMA_BUF_PLANE0_FD_EXT, fd,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)stride,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_NONE,
	};
	egl_image = eglCreateImageKHR(
		egl_display,
		EGL_NO_CONTEXT,
		EGL_LINUX_DMA_BUF_EXT,
		(EGLClientBuffer) nullptr,
		attrs
	);
	if (egl_image == EGL_NO_IMAGE_KHR)
	{
		throw std::runtime_error("Failed to create EGLImage from dma_buf");
		return;
	}

	glGenTextures(1, &texture);
	if (texture == 0) {
		throw std::runtime_error("glGenTextures failed");
	}

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	eglImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image);

	log(LOG_DEBUG, "EGLState: import_dma_buf_fd: success, texture is %d.\n", texture);
}

void EGLState::setup_gl_state() {
	// Set up GL state
	glViewport(0, 0, width, height);

	// Set up orthographic 2D projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, 0.0, height, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
}

void EGLState::render() {
	// Enable 2D texturing
	glBindTexture(GL_TEXTURE_2D, texture);

	// Draw a fullscreen quad with the texture
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f((GLfloat)width, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f((GLfloat)width, (GLfloat)height);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, (GLfloat)height);
	glEnd();

	// display the rendered image
	eglSwapBuffers(egl_display, egl_surface);
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

	log(LOG_DEBUG, "mousePressEvent: %d, %d, qt=%d x=%d\n", event->x(), event->y(), event->button(), x_button);
	qt->get_xrdp_local()->get_xup()->event_mouse_down(event->x(), event->y(), x_button);
}

void QtWindow::mouseReleaseEvent(QMouseEvent *event) {
	int x_button = qt_mouse_button_to_xrdp_mouse_button(event->button());
	if (x_button == -1) {
		log(LOG_WARN, "mouseReleaseEvent: Unknown Qt button %d\n", event->button());
		return;
	}

	log(LOG_DEBUG, "mouseReleaseEvent: %d, %d, qt=%d x=%d\n", event->x(), event->y(), event->button(), x_button);
	qt->get_xrdp_local()->get_xup()->event_mouse_up(event->x(), event->y(), x_button);
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
	for (int i = 0; i < std::min(screens.count(), displays_to_use); i++) {
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
