#include "info.h"
#include "common.h"

display::display(int x, int y, int width, int height, int orientation, int physical_width, int physical_height, int refresh_rate) {
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->orientation = orientation;
	this->physical_width = physical_width;
	this->physical_height = physical_height;
	this->refresh_rate = refresh_rate;
	log(LOG_DEBUG, "Display: %d %d %d %d %d %d %d %d\n", x, y, width, height, orientation, physical_width, physical_height, refresh_rate);
}

display::~display() {
}
int display::get_x() {
	return x;
}
int display::get_y() {
	return y;
}
int display::get_width() {
	return width;
}
int display::get_height() {
	return height;
}
int display::get_physical_width() {
	return physical_width;
}
int display::get_physical_height() {
	return physical_height;
}
int display::get_orientation() {
	return orientation;
}
int display::get_refresh_rate() {
	return refresh_rate;
}

DisplayInfo::DisplayInfo(std::vector<display> displays) {
	this->displays = displays;
}

DisplayInfo::~DisplayInfo() {
}

std::vector<display> DisplayInfo::get_displays() {
	return displays;
}
