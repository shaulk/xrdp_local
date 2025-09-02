cmake_minimum_required(VERSION 3.16)
project(xrdp_local LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source and header files
set(SOURCES
	src/common.cpp
	src/xrdp_local.cpp
	src/xup.cpp
)

set(HEADERS
	src/common.h
	src/xrdp_local.h
	src/xup.h
	src/info.h
	src/mock_config_ac.h
)

# Add include directories
include_directories(
	./src
	./xrdp
	./xrdp/common
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wold-style-cast")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Optimize release builds with -O3
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Werror")


# Find PkgConfig
find_package(PkgConfig REQUIRED)

# Find argparse
find_package(argparse REQUIRED)

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

add_subdirectory(src/qt)

target_link_libraries(${PROJECT_NAME}
	qt

	xup
	common
)
# cmake strips rpath on install by default, so add it explicitly
set_target_properties(${PROJECT_NAME} PROPERTIES
	INSTALL_RPATH "${XRDP_LIB_DIR}/xrdp"
)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
