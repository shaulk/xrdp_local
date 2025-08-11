#ifndef QT_EGL_H
#define QT_EGL_H

#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

// This is the EGL state for the window, if DMA-BUF is enabled, this will keep
// a reference to the pixmap used by the X server to maintain the screen state,
// so it can be displayed without having to copy the data from the GPU and back.
class EGLState {
public:
	EGLState(
		const char *x11_display,
		int window_id,
		int fd,
		uint32_t width,
		uint32_t height,
		uint16_t stride,
		uint32_t size,
		uint32_t format
	);
	~EGLState();

	// Render the shared texture to the screen
	void render();

	// Check if EGL is supported on the given display
	static bool is_supported(const char *x11_display);

private:
	// Import a DMA-BUF image reference into our EGL state using
	// EGL_EXT_image_dma_buf_import
	void import_dma_buf_fd(int fd,
		uint32_t width,
		uint32_t height,
		uint16_t stride,
		uint32_t size,
		uint32_t format
	);

	// Set up global GL state AFTER loading the shared pixmap
	void setup_gl_state();

	// EGL state
	EGLDisplay egl_display = EGL_NO_DISPLAY;
	EGLConfig egl_config = EGL_NO_CONFIG_KHR;
	EGLContext egl_context = EGL_NO_CONTEXT;
	EGLSurface egl_surface = EGL_NO_SURFACE;
	EGLImageKHR egl_image = EGL_NO_IMAGE_KHR;
	GLuint texture = 0;

	// The size of the shared pixmap
	int width;
	int height;
};

#endif
