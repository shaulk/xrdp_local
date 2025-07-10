#ifndef __QT_H__
#define __QT_H__

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

class SyncChangeReference{
public:
	SyncChangeReference(int width, int height, unsigned char *data, int num_rects, xrdp_rect_spec *rects);
	~SyncChangeReference();

	unsigned char *get_data();
	xrdp_rect_spec *get_rects();
	int get_num_rects();
	int get_width();
	int get_height();
	void signal_data_is_not_used();
private:
	unsigned char *data;
	int num_rects;
	xrdp_rect_spec *rects;
	int width;
	int height;
	std::mutex release_lock;
};

class QtState;

class QtWindow : public QWidget
{
	Q_OBJECT

public slots:
	void paint_rect_slot(SyncChangeReference *change, int x, int y);

public:
	QtWindow(QtState *QtState, int width, int height);
	~QtWindow();

	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

private:
	QtState *qt;
	QImage image;

	int next_update_x;
	int next_update_y;
	int next_update_width;
	int next_update_height;
};

class QtState : public QObject
{
	Q_OBJECT

public:
	QtState(XRDPLocalState *xrdp_local, int max_displays);
	~QtState();

	void paint_rects(int x, int y, unsigned char *data, int srcx, int srcy, int width, int height, int num_rects, xrdp_rect_spec *rects);
	void set_cursor(int x, int y, unsigned char *data, unsigned char *mask, int width, int height, int bpp);

	void run();
	void launch();
	void exit();

	XRDPLocalState *get_xrdp_local();
	DisplayInfo *get_display_info();

signals:
	void paint_rect_signal(SyncChangeReference *change, int x, int y);

private:
	int full_width;
	int full_height;

	QApplication *app;
	QtWindow *window;
	XRDPLocalState *xrdp_local;
	int max_displays;
};

#endif
