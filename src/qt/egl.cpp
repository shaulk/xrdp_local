#include <cstdlib>
#include <stdexcept>

#include "common.h"

#include "egl.h"

bool EGLState::is_supported(const char *x11_display) {
	EGLDisplay eglDisplay;

	if (x11_display[0] != ':') {
		log(LOG_WARN, "DMA-BUF not supported on remote X11 display (%s), disabling DMA-BUF.\n", x11_display);
		return false;
	}

	intptr_t x11_display_num = atoi(&x11_display[1]);

	eglDisplay = eglGetDisplay((EGLNativeDisplayType)x11_display_num);
	if (eglDisplay == EGL_NO_DISPLAY) {
		log(LOG_INFO, "eglGetDisplay failed, disabling DMA-BUF.\n");
		return false;
	}

	if (eglInitialize(eglDisplay, nullptr, nullptr) == EGL_FALSE) {
		log(LOG_INFO, "EGL failed to initialize, disabling DMA-BUF.\n");
		return false;
	}

	eglTerminate(eglDisplay);

	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
		(PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	if (eglCreateImageKHR == nullptr) {
		log(LOG_INFO, "EGL EGL_KHR_gl_texture_2D_image extension not supported, DMA-BUF not supported.\n");
		return false;
	}
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC eglImageTargetTexture2DOES =
		(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
			eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if (eglImageTargetTexture2DOES == nullptr) {
		log(LOG_INFO, "EGL EGL_EXT_image_dma_buf_import extension not supported, DMA-BUF not supported.\n");
		return false;
	}

	log(LOG_INFO, "DMA-BUF acceleration is supported by xrdp and our environment, trying to enable...\n");
	return true;
}

EGLState::EGLState(const char *x11_display, int window_id, int fd, uint32_t width, uint32_t height, uint16_t stride, uint32_t size, uint32_t format) {
	int numConfigs = 0;

	log(LOG_DEBUG, "EGLState: %s, %d, %d, %d, %d, %d, %d, %X\n", x11_display, window_id, fd, width, height, stride, size, format);

	if (x11_display[0] != ':') {
		throw std::runtime_error("DMA-BUF not supported on remote X11 display.");
	}

	intptr_t x11_display_num = atoi(&x11_display[1]);

	this->width = width;
	this->height = height;

	int egl_config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_NONE,
	};
	int egl_ctx_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};

	egl_display = eglGetDisplay((EGLNativeDisplayType)x11_display_num);
	if (egl_display == EGL_NO_DISPLAY) {
		throw std::runtime_error("eglGetDisplay failed");
	}

	if (eglInitialize(egl_display, nullptr, nullptr) == EGL_FALSE) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglInitialize failed");
	}

	if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglBindAPI failed");
	}

	if (eglChooseConfig(egl_display, egl_config_attribs, &egl_config, 1, &numConfigs) == EGL_FALSE) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglChooseConfig failed");
	}

	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_ctx_attribs);
	if (egl_context == EGL_NO_CONTEXT) {
		eglTerminate(egl_display);
		throw std::runtime_error("eglCreateContext failed");
	}

	egl_surface = eglCreateWindowSurface(egl_display, egl_config,
										(EGLNativeWindowType)window_id, nullptr);
	if (egl_surface == EGL_NO_SURFACE) {
		eglDestroyContext(egl_display, egl_context);
		eglTerminate(egl_display);
		throw std::runtime_error("eglCreateWindowSurface failed");
	}

	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

	import_dma_buf_fd(fd, width, height, stride, size, format);
	setup_gl_state();
}

EGLState::~EGLState() {
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (egl_image != EGL_NO_IMAGE_KHR) {
		PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
		if (eglDestroyImageKHR != nullptr) {
			eglDestroyImageKHR(egl_display, egl_image);
			egl_image = EGL_NO_IMAGE_KHR;
		}
	}

	if (texture != 0) {
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	if (egl_surface != EGL_NO_SURFACE) {
		eglDestroySurface(egl_display, egl_surface);
		egl_surface = EGL_NO_SURFACE;
	}

	if (egl_context != EGL_NO_CONTEXT) {
		eglDestroyContext(egl_display, egl_context);
		egl_context = EGL_NO_CONTEXT;
	}

	if (egl_display != EGL_NO_DISPLAY) {
		eglTerminate(egl_display);
		egl_display = EGL_NO_DISPLAY;
	}

	eglReleaseThread();
}

void EGLState::import_dma_buf_fd(int fd, uint32_t width, uint32_t height, uint16_t stride, uint32_t size, uint32_t format) {
	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
		(PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC eglImageTargetTexture2DOES =
		(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
			eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if (eglCreateImageKHR == nullptr || eglImageTargetTexture2DOES == nullptr) {
		throw std::runtime_error("eglCreateImageKHR or eglImageTargetTexture2DOES not supported.");
	}

	EGLint attrs[] = {
		EGL_WIDTH, (EGLint)width,
		EGL_HEIGHT, (EGLint)height,
		EGL_LINUX_DRM_FOURCC_EXT, (EGLint)format,
		EGL_DMA_BUF_PLANE0_FD_EXT, fd,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)stride,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_NONE,
	};
	egl_image = eglCreateImageKHR(
		egl_display,
		EGL_NO_CONTEXT,
		EGL_LINUX_DMA_BUF_EXT,
		(EGLClientBuffer) nullptr,
		attrs
	);
	if (egl_image == EGL_NO_IMAGE_KHR)
	{
		throw std::runtime_error("Failed to create EGLImage from dma_buf");
		return;
	}

	glGenTextures(1, &texture);
	if (texture == 0) {
		throw std::runtime_error("glGenTextures failed");
	}

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	eglImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image);

	log(LOG_DEBUG, "EGLState: import_dma_buf_fd: success, texture is %d.\n", texture);
}

void EGLState::setup_gl_state() {
	// Set up GL state
	glViewport(0, 0, width, height);

	// Set up orthographic 2D projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, 0.0, height, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
}

void EGLState::render() {
	// Enable 2D texturing
	glBindTexture(GL_TEXTURE_2D, texture);

	// Draw a fullscreen quad with the texture
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f((GLfloat)width, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f((GLfloat)width, (GLfloat)height);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, (GLfloat)height);
	glEnd();

	// display the rendered image
	eglSwapBuffers(egl_display, egl_surface);
}
