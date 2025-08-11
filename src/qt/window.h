#ifndef QT_WINDOW_H
#define QT_WINDOW_H

#include <latch>
#include <QWidget>
#include <QObject>
#include <QThread>
#include <QImage>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>

// Choose between two implementations of moving screen data from xup to Qt
// USE_COPIES mallocs new buffers, copies the data and returns to xup
// immediately.
// USE_BORROWS blocks the xup client thread until Qt finishes drawing and gives
// Qt a borrowed pointer to the data.
// #define USE_COPIES
#define USE_BORROWS

class QtState;

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

#endif
