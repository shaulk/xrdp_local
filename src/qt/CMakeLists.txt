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

# Make sure we don't accidentally use deprecated Qt APIs
target_compile_definitions(qt PRIVATE
	QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	# We compile with -Werror, so disable deprecation warnings in release builds
	target_compile_definitions(qt PRIVATE
		QT_NO_DEPRECATED_WARNINGS
	)
endif()

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
