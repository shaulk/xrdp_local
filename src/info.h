#ifndef INFO_H
#define INFO_H

// Misc information structures

#include <cstdint>
#include <vector>

// Represents a single display as returned by Qt, to be used by xup to inform
// xorgxrdp of our desired display configuration
struct display {
	int x;
	int y;
	int width;
	int height;
	int physical_width;
	int physical_height;
	int orientation;
	int refresh_rate;
};

// Represents the display configuration as returned by Qt, to be used by xup
// to inform xorgxrdp of our desired display configuration
struct display_info {
	std::vector<display> displays;
};

// This struct is an implementation detail of xorgxrdp and its protocol with
// xrdp, if it changes we need to change it too. It's here as an optimization
// to save on copies.
// xrdp doesn't define it, it reads it raw from xorgxrdp, so we define it here.
struct __attribute__((packed)) xrdp_rect_spec {
	int16_t x;
	int16_t y;
	int16_t cx;
	int16_t cy;
};

#endif
