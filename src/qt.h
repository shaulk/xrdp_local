#ifndef __QT_H__
#define __QT_H__

// Qt client module
// This module is responsible for displaying the screens, and handling mouse and
// keyboard events, by launching a Qt application.

#include <QApplication>
#include <QWidget>
#include <QObject>
#include <QThread>
#include <QImage>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>

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

private:
	unsigned char *data;
	int num_rects;
	xrdp_rect_spec *rects;
	int width;
	int height;

	// This lock is used internally by signal_data_is_not_used and the destructor
	std::mutex release_lock;
};

class QtState;

// This is the main window that displays all screens
// There is just one and it's as big as the theoretical rectangle that contains
// all screens.
class QtWindow : public QWidget
{
	Q_OBJECT

public slots:
	// This is called by the QtState thread to paint a rectangle
	// It's connected to QtState::paint_rect_signal and used to allow the xup
	// thread to paint a rectangle.
	void paint_rect_slot(SyncChangeReference *change, int x, int y);

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

private:
	QtState *qt;
	QImage image;
};

// Main Qt state class
class QtState : public QObject
{
	Q_OBJECT

public:
	// max_displays can be used to limit the number of displays that are allowed
	// The application defaults to using all available displays
	QtState(XRDPLocalState *xrdp_local, int max_displays);
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

signals:
	// Used to trigger QtWindow::paint_rect_slot
	void paint_rect_signal(SyncChangeReference *change, int x, int y);

private:
	// The maximum number of displays to use
	int max_displays;

	// The width and height of the rectangle that contains all screens
	int full_width;
	int full_height;

	// The Qt application
	QApplication *app;

	// The main window that shows all displays
	QtWindow *window;

	// The global state of the application
	XRDPLocalState *xrdp_local;

	// This prevents the xup client thread from calling painting before the
	// Qt application is ready
	std::mutex app_ready_lock;
};

#endif
