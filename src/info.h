#ifndef INFO_H
#define INFO_H

#include <cstdint>
#include <vector>

class display {
public:
	display(int x, int y, int width, int height, int physical_width, int physical_height, int orientation, int refresh_rate);
	~display();

	int get_x();
	int get_y();
	int get_width();
	int get_height();
	int get_physical_width();
	int get_physical_height();
	int get_orientation();
	int get_refresh_rate();

private:
	int x;
	int y;
	int width;
	int height;
	int physical_width;
	int physical_height;
	int orientation;
	int refresh_rate;
};

class DisplayInfo {
public:
	DisplayInfo(std::vector<display> displays);
	~DisplayInfo();

	std::vector<display> get_displays();

private:
	std::vector<display> displays;
};

// This struct is an implementation detail of xorgxrdp and its protocol with
// xrdp, if it changes we need to change it too. It's here as an optimization
// to save on copies.
// xrdp doesn't define it, it reads it raw from xorgxrdp, so we define it here.
// Also, the code that uses it assumes we're running on a little endian machine.
// If for some reason you switch to a big endian machine and this code is in use
// (which is unlikely), make sure you translate from xorgxrdp, which always uses
// little endian in its protocol.
struct __attribute__((packed)) xrdp_rect_spec {
	int16_t x;
	int16_t y;
	int16_t cx;
	int16_t cy;
};

#endif
