#ifndef __QT_H__
#define __QT_H__

// Qt client module
// This module is responsible for displaying the screens, and handling mouse and
// keyboard events, by launching a Qt application.

#include <latch>
#include <QApplication>
#include <QWidget>
#include <QObject>
#include <QThread>
#include <QImage>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "xrdp_local.h"
#include "info.h"

class XRDPLocalState;

// Used to move screen data between the xup client thread and the qt thread
// The xup client thread will create a SyncChangeReference object and hand it
// to the qt thread, the destructor blocks so the xup client thread can wait
// for the qt thread to finish processing the data.
class SyncChangeReference{
public:
	// These are the raw values from the xup client thread
	SyncChangeReference(int width, int height, unsigned char *data, int num_rects, xrdp_rect_spec *rects);

	// The destructor blocks until a call to signal_data_is_not_used
	~SyncChangeReference();

	// Getters
	unsigned char *get_data();
	xrdp_rect_spec *get_rects();
	int get_num_rects();
	int get_width();
	int get_height();

	// Signal that the data is no longer needed
	void signal_data_is_not_used();
	void block_until_data_is_not_used();

private:
	unsigned char *data;
	int num_rects;
	xrdp_rect_spec *rects;
	int width;
	int height;

	// This latch is used to prevent SyncChangeReference from never being
	// freed (by the paint_rect_signal leading to nowhere before the app is
	// properly initialized), which causes hangs (with USE_BORROWS) or memory
	// leaks (with USE_COPIES).
	std::latch release_latch;
};

class QtState;

// This is the EGL state for the window, if DMA-BUF is enabled, this will keep
// a reference to the pixmap used by the X server to maintain the screen state,
// so it can be displayed without having to copy the data from the GPU and back.
class EGLState {
public:
	EGLState(
		intptr_t x11_display,
		int window_id,
		int fd,
		uint32_t width,
		uint32_t height,
		uint16_t stride,
		uint32_t size,
		uint32_t format
	);
	~EGLState();

	// Render the shared texture to the screen
	void render();

	// Check if EGL is supported on the given display
	static bool is_supported(intptr_t x11_display);

private:
	// Import a DMA-BUF image reference into our EGL state using
	// EGL_EXT_image_dma_buf_import
	void import_dma_buf_fd(int fd,
		uint32_t width,
		uint32_t height,
		uint16_t stride,
		uint32_t size,
		uint32_t format
	);

	// Set up global GL state AFTER loading the shared pixmap
	void setup_gl_state();

	// EGL state
	EGLDisplay egl_display = EGL_NO_DISPLAY;
	EGLConfig egl_config = EGL_NO_CONFIG_KHR;
	EGLContext egl_context = EGL_NO_CONTEXT;
	EGLSurface egl_surface = EGL_NO_SURFACE;
	EGLImageKHR egl_image = EGL_NO_IMAGE_KHR;
	GLuint texture = 0;

	// The size of the shared pixmap
	int width;
	int height;
};

// This is the main window that displays all screens
// There is just one and it's as big as the theoretical rectangle that contains
// all screens.
class QtWindow : public QWidget
{
	Q_OBJECT

public slots:
	// This is called by the QtState thread to paint a rectangle.
	// It's connected to QtState::paint_rect_signal and used to allow the
	// xup thread to paint a rectangle.
	// When using DMA-BUF, this is skipped and paint_dma_buf (which calls
	// EGLState::render) is used instead.
	void paint_rects_slot(SyncChangeReference *change, int x, int y);

public:
	QtWindow(QtState *QtState, int width, int height);
	~QtWindow();

	// Overriden QtWidget events

	// Called when paint_rect_slot calls QWidget::update
	void paintEvent(QPaintEvent *event);

	// Mouse events
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);

	// Keyboard events
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

	void set_disable_paint(bool disable_paint);

private:
	QtState *qt;

	// This is the actual framebuffer of the screens
	// We need to keep it in RAM because X11 sometimes asks applications to
	// redraw parts of themselves when not using compositing
	QImage framebuffer;

	// This is used to map Qt mouse buttons to xrdp mouse buttons
	int qt_mouse_button_to_xrdp_mouse_button(Qt::MouseButton button);
};

// Main Qt state class
class QtState : public QObject
{
	Q_OBJECT

public:
	// max_displays can be used to limit the number of displays that are allowed
	// The application defaults to using all available displays
	QtState(XRDPLocalState *xrdp_local, int max_displays, bool use_dma_buf);
	~QtState();

	// This is called by the xup client thread to paint screen data
	void paint_rects(int x, int y, unsigned char *data, int srcx, int srcy, int width, int height, int num_rects, xrdp_rect_spec *rects);

	// This is called by the xup client thread to set the cursor shape
	void set_cursor(int x, int y, unsigned char *data, unsigned char *mask, int width, int height, int bpp);

	// Run the main loop
	void run();

	// Launch the application after xup is initialized
	void launch();

	// Exit the application
	void exit();

	// Returns display info from Qt
	DisplayInfo *get_display_info();

	// Getters
	XRDPLocalState *get_xrdp_local();

	// DMA-BUF management functions
	bool enable_dma_buf(int fd, uint32_t width, uint32_t height, uint16_t stride, uint32_t size, uint32_t format);
	void disable_dma_buf();
	void paint_dma_buf();

signals:
	// Used to trigger QtWindow::paint_rect_slot
	void paint_rects_signal(SyncChangeReference *change, int x, int y);

private:
	int x11_display();

	// The maximum number of displays to use
	int max_displays;
	int displays_to_use;
	bool use_dma_buf;

	// The width and height of the rectangle that contains all screens
	int full_width;
	int full_height;

	// The Qt application
	QApplication *app;

	// The main window that shows all displays
	QtWindow *window;

	// The EGL state for the window, if DMA-BUF is enabled
	EGLState *egl = nullptr;

	// The global state of the application
	XRDPLocalState *xrdp_local;

	// This prevents the xup client thread from calling painting before the
	// Qt application is ready
	std::latch app_ready_latch;
};

#endif
