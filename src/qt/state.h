#ifndef __QT_H__
#define __QT_H__

// Qt client module
// This module is responsible for displaying the screens, and handling mouse and
// keyboard events, by launching a Qt application.

#include <QApplication>
#include <memory>

#include "xrdp_local.h"
#include "info.h"
#include "window.h"
#include "egl.h"

class XRDPLocalState;

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
	std::unique_ptr<struct display_info> get_display_info();

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
	char *x11_display();

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
