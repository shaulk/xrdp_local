# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Enable automatic MOC, UIC, and RCC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(qt STATIC
	state.cpp
	state.h
	window.cpp
	window.h
	egl.cpp
	egl.h
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Find X11
pkg_check_modules(X11 REQUIRED x11)

# Find EGL
pkg_check_modules(EGL REQUIRED egl)

target_link_libraries(qt PUBLIC
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	EGL
	X11
)
