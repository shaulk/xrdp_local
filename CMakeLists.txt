cmake_minimum_required(VERSION 3.16)
project(xrdp_local LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic MOC, UIC, and RCC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set the source and header files
set(SOURCES
	src/common.cpp
	src/qt.cpp
	src/xrdp_local.cpp
	src/xup.cpp
	src/info.cpp
)

set(HEADERS
	src/common.h
	src/qt.h
	src/xrdp_local.h
	src/xup.h
	src/info.h
	src/mock_config_ac.h
)

# Add include directories
include_directories(
	.
	./xrdp
	./xrdp/common
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Make sure we don't accidentally use deprecated Qt APIs
target_compile_definitions(${PROJECT_NAME} PRIVATE
	QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Optimize release builds with -O3
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Find PkgConfig
find_package(PkgConfig REQUIRED)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Find argparse
find_package(argparse REQUIRED)

# Find X11
pkg_check_modules(X11 REQUIRED x11)

# Find EGL
pkg_check_modules(EGL REQUIRED egl)

# xrdp doesn't declare a libdir because it doesn't expect other binaries to link
# against its libraries, so we extract its libdir from pkg-config
pkg_check_modules(XRDP REQUIRED xrdp)
pkg_get_variable(XRDP_LIB_DIR xrdp libdir)

target_link_directories(${PROJECT_NAME} PRIVATE ${XRDP_LIB_DIR}/xrdp)

set_target_properties(${PROJECT_NAME} PROPERTIES
	BUILD_RPATH ${XRDP_LIB_DIR}/xrdp
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
	-Wno-unused-parameter
	-DXRDP_LIB_DIR="${XRDP_LIB_DIR}/xrdp"
)

target_link_libraries(${PROJECT_NAME}
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	EGL
	X11
	xup
	common
)

