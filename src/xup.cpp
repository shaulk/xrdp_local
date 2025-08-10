#include <stdexcept>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

// Our xup.h
#include "xup.h"
#include "common.h"
#include "info.h"

extern "C" {
	// Headers from xrdp
	#include "log.h"
	#include "trans.h"
	#include "parse.h"
}

int XRDPModState::server_begin_update(struct mod *v) {
	log(LOG_DEBUG, "server_begin_update\n");
	return 0;
}

int XRDPModState::server_end_update(struct mod *v) {
	log(LOG_DEBUG, "server_end_update\n");
	return 0;
}	

int XRDPModState::server_fill_rect(struct mod *v, int x, int y, int cx, int cy) {
	throw std::runtime_error("server_fill_rect is not implemented.");
}

int XRDPModState::server_screen_blt(struct mod *v, int x, int y, int cx, int cy,
						int srcx, int srcy) {
	throw std::runtime_error("server_screen_blt is not implemented.");
}
int XRDPModState::server_paint_rect(struct mod *v, int x, int y, int cx, int cy,
						char *data, int width, int height,
						int srcx, int srcy) {
	log(LOG_DEBUG, "server_paint_rect: %d, %d, %d, %d, %d, %d\n", x, y, cx, cy, srcx, srcy);
	throw std::runtime_error("server_paint_rect is not implemented.");
}

int XRDPModState::server_set_cursor(struct mod *v, int x, int y, char *data, char *mask) {
	throw std::runtime_error("server_set_cursor is not implemented.");
}

int XRDPModState::server_palette(struct mod *v, int *palette) {
	throw std::runtime_error("server_palette is not implemented.");
}

int XRDPModState::server_msg(struct mod *v, const char *msg, int code) {
	log(LOG_INFO, "Got message from xup: %s\n", msg);
	return strlen(msg);
}

int XRDPModState::server_is_term(void) {
	log(LOG_DEBUG, "Server is closing.\n");
	return 0;
}

int XRDPModState::server_set_clip(struct mod *v, int x, int y, int cx, int cy) {
	throw std::runtime_error("server_set_clip is not implemented.");
}

int XRDPModState::server_reset_clip(struct mod *v) {
	throw std::runtime_error("server_reset_clip is not implemented.");
}

int XRDPModState::server_set_fgcolor(struct mod *v, int fgcolor) {
	throw std::runtime_error("server_set_fgcolor is not implemented.");
}

int XRDPModState::server_set_bgcolor(struct mod *v, int bgcolor) {
	throw std::runtime_error("server_set_bgcolor is not implemented.");
}

int XRDPModState::server_set_opcode(struct mod *v, int opcode) {
	throw std::runtime_error("server_set_opcode is not implemented.");
}

int XRDPModState::server_set_mixmode(struct mod *v, int mixmode) {
	throw std::runtime_error("server_set_mixmode is not implemented.");
}

int XRDPModState::server_set_brush(struct mod *v, int x_origin, int y_origin,
						int style, char *pattern) {
	throw std::runtime_error("server_set_brush is not implemented.");
}

int XRDPModState::server_set_pen(struct mod *v, int style,
					int width) {
	throw std::runtime_error("server_set_pen is not implemented.");
}

int XRDPModState::server_draw_line(struct mod *v, int x1, int y1, int x2, int y2) {
	throw std::runtime_error("server_draw_line is not implemented.");
}

int XRDPModState::server_add_char(struct mod *v, int font, int character,
					int offset, int baseline,
					int width, int height, char *data) {
	throw std::runtime_error("server_add_char is not implemented.");
}

int XRDPModState::server_draw_text(struct mod *v, int font,
						int flags, int mixmode, int clip_left, int clip_top,
						int clip_right, int clip_bottom,
						int box_left, int box_top,
						int box_right, int box_bottom,
						int x, int y, char *data, int data_len) {
	throw std::runtime_error("server_draw_text is not implemented.");
}

int XRDPModState::client_monitor_resize(struct mod *v, int width, int height,
							int num_monitors,
							const struct monitor_info *monitors) {
	throw std::runtime_error("client_monitor_resize is not implemented.");
}

int XRDPModState::server_monitor_resize_done(struct mod *v) {
	log(LOG_DEBUG, "Got server_monitor_resize_done.\n");
	return 0;
}

int XRDPModState::server_get_channel_count(struct mod *v) {
	throw std::runtime_error("server_get_channel_count is not implemented.");
}

int XRDPModState::server_query_channel(struct mod *v, int index,
							char *channel_name,
							int *channel_flags) {
	throw std::runtime_error("server_query_channel is not implemented.");
}

int XRDPModState::server_get_channel_id(struct mod *v, const char *name) {
	throw std::runtime_error("server_get_channel_id is not implemented.");
}

int XRDPModState::server_send_to_channel(struct mod *v, int channel_id,
							char *data, int data_len,
							int total_data_len, int flags) {
	throw std::runtime_error("server_send_to_channel is not implemented.");
}

int XRDPModState::server_bell_trigger(struct mod *v) {
	throw std::runtime_error("server_bell_trigger is not implemented.");
}

int XRDPModState::server_chansrv_in_use(struct mod *v) {
	throw std::runtime_error("server_chansrv_in_use is not implemented.");
}

/* off screen bitmaps */
int XRDPModState::server_create_os_surface(struct mod *v, int rdpindex,
								int width, int height) {
	throw std::runtime_error("server_create_os_surface is not implemented.");
}

int XRDPModState::server_switch_os_surface(struct mod *v, int rdpindex) {
	throw std::runtime_error("server_switch_os_surface is not implemented.");
}

int XRDPModState::server_delete_os_surface(struct mod *v, int rdpindex) {
	throw std::runtime_error("server_delete_os_surface is not implemented.");
}

int XRDPModState::server_paint_rect_os(struct mod *v, int x, int y,
							int cx, int cy,
							int rdpindex, int srcx, int srcy) {
	throw std::runtime_error("server_paint_rect_os is not implemented.");
}

int XRDPModState::server_set_hints(struct mod *v, int hints, int mask) {
	throw std::runtime_error("server_set_hints is not implemented.");
}

/* rail */
int XRDPModState::server_window_new_update(struct mod *v, int window_id,
								struct rail_window_state_order *window_state,
								int flags) {
	throw std::runtime_error("server_window_new_update is not implemented.");
}

int XRDPModState::server_window_delete(struct mod *v, int window_id) {
	throw std::runtime_error("server_window_delete is not implemented.");
}

int XRDPModState::server_window_icon(struct mod *v,
						int window_id, int cache_entry, int cache_id,
						struct rail_icon_info *icon_info,
						int flags) {
	throw std::runtime_error("server_window_icon is not implemented.");
}
int XRDPModState::server_window_cached_icon(struct mod *v,
								int window_id, int cache_entry,
								int cache_id, int flags) {
	throw std::runtime_error("server_window_cached_icon is not implemented.");
}

int XRDPModState::server_notify_new_update(struct mod *v,
								int window_id, int notify_id,
								struct rail_notify_state_order *notify_state,
								int flags) {
	throw std::runtime_error("server_notify_new_update is not implemented.");
}

int XRDPModState::server_notify_delete(struct mod *v, int window_id,
							int notify_id) {
	throw std::runtime_error("server_notify_delete is not implemented.");
}

int XRDPModState::server_monitored_desktop(struct mod *v,
								struct rail_monitored_desktop_order *mdo,
								int flags) {
	throw std::runtime_error("server_monitored_desktop is not implemented.");
}

int XRDPModState::server_set_cursor_ex(struct mod *v, int x, int y, char *data,
							char *mask, int bpp) {
	log(LOG_DEBUG, "server_set_cursor_ex: %d, %d, %d\n", x, y, bpp);
	// This is exactly the same as in xrdp
	int Bpp = (bpp == 0) ? 3 : (bpp + 7) / 8;
	XRDPModState *xrdp_mod_state = xrdp_mod_state_from_mod(v);
	xrdp_mod_state->qt->set_cursor(x, y, (unsigned char *)data, (unsigned char *)mask, 32, 32, Bpp * 8);
	return 0;
}

int XRDPModState::server_add_char_alpha(struct mod *v, int font, int character,
							int offset, int baseline,
							int width, int height, char *data) {
	throw std::runtime_error("server_add_char_alpha is not implemented.");
}
int XRDPModState::server_create_os_surface_bpp(struct mod *v, int rdpindex,
									int width, int height, int bpp) {
	throw std::runtime_error("server_create_os_surface_bpp is not implemented.");
}

int XRDPModState::server_paint_rect_bpp(struct mod *v, int x, int y, int cx, int cy,
							char *data, int width, int height,
							int srcx, int srcy, int bpp) {
	throw std::runtime_error("server_paint_rect_bpp is not implemented.");
}

int XRDPModState::server_composite(struct mod *v, int srcidx, int srcformat, int srcwidth,
						int srcrepeat, int *srctransform, int mskflags, int mskidx,
						int mskformat, int mskwidth, int mskrepeat, int op,
						int srcx, int srcy, int mskx, int msky,
						int dstx, int dsty, int width, int height, int dstformat) {
	throw std::runtime_error("server_composite is not implemented.");
}

int XRDPModState::server_paint_rects(struct mod *v,
						int num_drects, short *drects,
						int num_crects, short *crects,
						char *data, int width, int height,
						int flags, int frame_id) {
	throw std::runtime_error("server_paint_rects is not implemented.");
}

int XRDPModState::server_dma_buf_notify(struct mod *v,
						enum dma_buf_server_notify state) {
	log(LOG_DEBUG, "server_dma_buf_notify: %d\n", state);
	return 0;
}

int XRDPModState::server_dma_buf_receive_pixmap_fd(struct mod *v,
							int fd, uint32_t width,
							uint32_t height, uint16_t stride,
							uint32_t size, uint32_t format) {
	log(LOG_DEBUG, "server_dma_buf_receive_pixmap_fd: %d, %d, %d, %d, %d, %X\n", fd, width, height, stride, size, format);

	XRDPModState *xrdp_mod_state = xrdp_mod_state_from_mod(v);
	if (!xrdp_mod_state->qt->enable_dma_buf(fd, width, height, stride, size, format)) {
		log(LOG_ERROR, "Failed to enable DMA buf.\n");
		v->mod_send_dma_buf_notify(v, DMA_BUF_NOTIFY_INACTIVE);
		return 0;
	}

	log(LOG_INFO, "DMA-BUF enabled.\n");

	v->mod_send_dma_buf_notify(v, DMA_BUF_NOTIFY_ACTIVE);

	return 0;
}

int XRDPModState::server_dma_buf_deactivate(struct mod *v) {
	log(LOG_DEBUG, "server_dma_buf_deactivate\n");

	XRDPModState *xrdp_mod_state = xrdp_mod_state_from_mod(v);
	xrdp_mod_state->qt->disable_dma_buf();

	log(LOG_INFO, "DMA-BUF disabled.\n");

	return 0;
}

int XRDPModState::server_dma_buf_paint_pixmap(struct mod *v) {
	log(LOG_DEBUG, "server_dma_buf_paint_pixmap\n");
	XRDPModState *xrdp_mod_state = xrdp_mod_state_from_mod(v);
	xrdp_mod_state->qt->paint_dma_buf();
	log(LOG_DEBUG, "server_dma_buf_paint_pixmap done\n");
	return 0;
}

int XRDPModState::server_session_info(struct mod *v, const char *data,
						int data_bytes) {
	throw std::runtime_error("server_session_info is not implemented.");
}

int XRDPModState::server_set_pointer_large(struct mod *v, int x, int y,
								char *data, char *mask, int bpp,
								int width, int height) {
	// This code is untested, but it should work according to a reading of xorgxrdp's code
	log(LOG_DEBUG, "server_set_pointer_large: %d, %d, %d, %d, %d\n", x, y, bpp, width, height);
	int Bpp = (bpp == 0) ? 3 : (bpp + 7) / 8;
	XRDPModState *xrdp_mod_state = xrdp_mod_state_from_mod(v);
	xrdp_mod_state->qt->set_cursor(x, y, (unsigned char *)data, (unsigned char *)mask, width, height, Bpp * 8);
	return 0;
}

int XRDPModState::server_paint_rects_ex(struct mod *v,
							int num_drects, short *drects,
							int num_crects, short *crects,
							char *data, int left, int top,
							int width, int height,
							int flags, int frame_id,
							void *shmem_ptr, int shmem_bytes) {
	log(LOG_DEBUG, "server_paint_rects_ex: %d, %d, %d, %d, %d, %d, %d, %d\n", num_drects, num_crects, left, top, width, height, flags, frame_id);
	XRDPModState *xrdp_mod_state = xrdp_mod_state_from_mod(v);
	xrdp_mod_state->qt->paint_rects(left, top, (unsigned char *)data, 0, 0, width, height, num_drects, (xrdp_rect_spec *)drects);
	v->mod_frame_ack(v, flags, frame_id);
	if (shmem_ptr != nullptr) {
		munmap(shmem_ptr, shmem_bytes);
	}
	return 0;
}

int XRDPModState::server_egfx_cmd(struct mod *v,
					char *cmd, int cmd_bytes,
					char *data, int data_bytes) {
	throw std::runtime_error("server_egfx_cmd is not implemented.");
}

XRDPModState *xrdp_mod_state_from_mod(struct mod *mod) {
	return (XRDPModState *)mod->wm;
}

XRDPModState::XRDPModState(XRDPLocalState *xrdp_local, QtState *qt, const char *socket_path) {
	log_start_from_param(log_config_init_for_console(LOG_LEVEL_DEBUG, "xrdp_local"));

	this->xrdp_local = xrdp_local;
	this->socket_path = socket_path;
	this->qt = qt;

	mod_dl = dlopen("/usr/lib/x86_64-linux-gnu/xrdp/libxup.so", RTLD_NOW);
	if (mod_dl == nullptr) {
		throw std::runtime_error("Failed to open libxup.so.");
	}

	mod_init = (struct mod *(*)(void))dlsym(mod_dl, "mod_init");
	mod_exit = (int (*)(struct mod *v))dlsym(mod_dl, "mod_exit");
	if (mod_init == nullptr || mod_exit == nullptr) {
		throw std::runtime_error("Failed to get mod_init or mod_exit symbol.");
	}

	xup_mod = mod_init();
	if (xup_mod == nullptr) {
		throw std::runtime_error("Failed to initialize xup mod.");
	}

	// wm is a pointer that is used by xrdp to store the upper layer state, so we can use it to do the same
	xup_mod->wm = (tintptr)this;

	check_dma_buf_supported_in_libxup();

	setup_xup_mod();
}

XRDPModState::~XRDPModState() {
	running = 0;
	xup_communicator_thread.join();
	mod_exit(xup_mod);
	dlclose(mod_dl);
	log_end();
}

void XRDPModState::setup_xup_functions() {
	xup_mod->server_begin_update = server_begin_update;
	xup_mod->server_end_update = server_end_update;
	xup_mod->server_fill_rect = server_fill_rect;
	xup_mod->server_screen_blt = server_screen_blt;
	xup_mod->server_paint_rect = server_paint_rect;
	xup_mod->server_set_cursor = server_set_cursor;
	xup_mod->server_palette = server_palette;
	xup_mod->server_msg = server_msg;
	xup_mod->server_is_term = server_is_term;
	xup_mod->server_set_clip = server_set_clip;
	xup_mod->server_reset_clip = server_reset_clip;
	xup_mod->server_set_fgcolor = server_set_fgcolor;
	xup_mod->server_set_bgcolor = server_set_bgcolor;
	xup_mod->server_set_opcode = server_set_opcode;
	xup_mod->server_set_mixmode = server_set_mixmode;
	xup_mod->server_set_brush = server_set_brush;
	xup_mod->server_set_pen = server_set_pen;
	xup_mod->server_draw_line = server_draw_line;
	xup_mod->server_add_char = server_add_char;
	xup_mod->server_draw_text = server_draw_text;
	xup_mod->client_monitor_resize = client_monitor_resize;
	xup_mod->server_monitor_resize_done = server_monitor_resize_done;
	xup_mod->server_get_channel_count = server_get_channel_count;
	xup_mod->server_query_channel = server_query_channel;
	xup_mod->server_get_channel_id = server_get_channel_id;
	xup_mod->server_send_to_channel = server_send_to_channel;
	xup_mod->server_bell_trigger = server_bell_trigger;
	xup_mod->server_chansrv_in_use = server_chansrv_in_use;
	xup_mod->server_create_os_surface = server_create_os_surface;
	xup_mod->server_switch_os_surface = server_switch_os_surface;
	xup_mod->server_delete_os_surface = server_delete_os_surface;
	xup_mod->server_paint_rect_os = server_paint_rect_os;
	xup_mod->server_set_hints = server_set_hints;
	xup_mod->server_window_new_update = server_window_new_update;
	xup_mod->server_window_delete = server_window_delete;
	xup_mod->server_window_icon = server_window_icon;
	xup_mod->server_window_cached_icon = server_window_cached_icon;
	xup_mod->server_notify_new_update = server_notify_new_update;
	xup_mod->server_notify_delete = server_notify_delete;
	xup_mod->server_monitored_desktop = server_monitored_desktop;
	xup_mod->server_set_cursor_ex = server_set_cursor_ex;
	xup_mod->server_add_char_alpha = server_add_char_alpha;
	xup_mod->server_create_os_surface_bpp = server_create_os_surface_bpp;
	xup_mod->server_paint_rect_bpp = server_paint_rect_bpp;
	xup_mod->server_composite = server_composite;
	xup_mod->server_paint_rects = server_paint_rects;
	xup_mod->server_session_info = server_session_info;
	xup_mod->server_set_pointer_large = server_set_pointer_large;
	xup_mod->server_paint_rects_ex = server_paint_rects_ex;
	xup_mod->server_egfx_cmd = server_egfx_cmd;
	xup_mod->server_dma_buf_notify = server_dma_buf_notify;
	xup_mod->server_dma_buf_receive_pixmap_fd = server_dma_buf_receive_pixmap_fd;
	xup_mod->server_dma_buf_deactivate = server_dma_buf_deactivate;
	xup_mod->server_dma_buf_paint_pixmap = server_dma_buf_paint_pixmap;
}

void XRDPModState::setup_xup_client_info() {
	DisplayInfo *display_info = qt->get_display_info();
	memset(&client_info, 0, sizeof(client_info));
	client_info.size = sizeof(client_info);
	client_info.version = CLIENT_INFO_CURRENT_VERSION;

	// We always claim to have multiple displays even if we have only one so we
	// always use the multimon code path within xorgxrdp to avoid
	// incompatibility bugs
	client_info.multimon = 1;

	// Bits per pixel
	client_info.bpp = 32;

	// Enable color pointers (xrdp doesn't have a constant for this)
	client_info.pointer_flags = 1;

	// This enables server_set_pointer_large, which is untested but should work
	client_info.large_pointer_support_flags = LARGE_POINTER_FLAG_96x96;

	// This one is the fastest for xorgxrdp, because xorg uses it internally so xorgxrdp just does memcpy
	client_info.capture_format = XRDP_a8r8g8b8;

	client_info.capture_code = CC_SIMPLE;

	// Default to some sane values
	client_info.display_sizes.session_width = 1;
	client_info.display_sizes.session_height = 1;

	auto displays = display_info->get_displays();
	client_info.display_sizes.monitorCount = std::min((int)displays.size(), CLIENT_MONITOR_DATA_MAXIMUM_MONITORS);

	// Sane default for ~60 fps
	client_info.normal_frame_interval = 16;

	for (unsigned int i = 0; i < client_info.display_sizes.monitorCount; i++) {
		auto display = displays[i];
		client_info.display_sizes.minfo[i].left = display.get_x();
		client_info.display_sizes.minfo[i].top = display.get_y();

		// These specify the actual bottom and right pixel numbers, so the -1 is
		// not an error
		client_info.display_sizes.minfo[i].right = display.get_x() + display.get_width() - 1;
		client_info.display_sizes.minfo[i].bottom = display.get_y() + display.get_height() - 1;

		client_info.display_sizes.minfo[i].physical_width = display.get_physical_width();
		client_info.display_sizes.minfo[i].physical_height = display.get_physical_height();
		client_info.display_sizes.minfo[i].orientation = display.get_orientation();

		client_info.display_sizes.minfo[i].desktop_scale_factor = 100;
		client_info.display_sizes.minfo[i].device_scale_factor = 100;

		client_info.display_sizes.minfo[i].is_primary = (i == 0);

		// Resize the full width to include every display
		client_info.display_sizes.session_width = std::max((int)client_info.display_sizes.session_width, client_info.display_sizes.minfo[i].right + 1);
		client_info.display_sizes.session_height = std::max((int)client_info.display_sizes.session_height, client_info.display_sizes.minfo[i].bottom + 1);

		// Update the frame interval using the lowest refresh rate of all displays
		client_info.normal_frame_interval = std::min(client_info.normal_frame_interval, (int)(1000 / display.get_refresh_rate()));
	}

	// Set the client description to the name of the application
	snprintf(client_info.client_description, sizeof(client_info.client_description), "xrdp_local");

	log(LOG_DEBUG, "normal_frame_interval set to %d\n", client_info.normal_frame_interval);
}

void XRDPModState::setup_xup_mod() {
	setup_xup_client_info();
	setup_xup_functions();
	DisplayInfo *display_info = qt->get_display_info();
	auto displays = display_info->get_displays();
	if (displays.size() < 1) {
		throw std::runtime_error("No displays found.");
	}
	int initial_width = displays[0].get_width();
	int initial_height = displays[0].get_height();
	log(LOG_DEBUG, "Setting up xup with initial size %dx%d\n", initial_width, initial_height);
	if (xup_mod->mod_start(xup_mod, initial_width, initial_height, 32) != 0) {
		throw std::runtime_error("Failed to start xup mod.");
	}
	log(LOG_DEBUG, "xup_mod.mod_start successful.\n");
	xup_mod->mod_set_param(xup_mod, "client_info", (const char *)&client_info);
	xup_mod->mod_set_param(xup_mod, "port", socket_path);
	if (xup_mod->mod_connect(xup_mod) != 0) {
		throw std::runtime_error("Failed to connect to xup mod.");
	}
	if (xup_mod->mod_event != 0)
	{
		log(LOG_DEBUG, "Sending WM_KEYBRD_SYNC event.\n");
		// TODO: Replace with actual lock key state
		xup_mod->mod_event(xup_mod, WM_KEYBRD_SYNC, 0, 0, 0, 0);
	}
	xup_communicator_thread = std::thread(&XRDPModState::xup_communicator_thread_func, this);
	log(LOG_INFO, "Connected to X server.\n");
}

int XRDPModState::dma_buf_supported_in_libxup_callback(struct dl_phdr_info *info, size_t size, void *data) {
	XRDPModState *mod_state = (XRDPModState *)data;
	if (info->dlpi_name != nullptr && strstr(info->dlpi_name, "libxup.so") != nullptr) {
		for (int i = 0; i < info->dlpi_phnum; i++) {
			if (info->dlpi_phdr[i].p_type == PT_LOAD) {
				uint8_t *start = (uint8_t *)(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
				size_t size = info->dlpi_phdr[i].p_memsz;
				log(LOG_DEBUG, "Checking for mod_send_dma_buf_notify %p in %s at %p %p\n", mod_state->xup_mod->mod_send_dma_buf_notify, info->dlpi_name, start, start + size);
				if (mod_state->xup_mod->mod_send_dma_buf_notify != nullptr && mod_state->xup_mod->mod_send_dma_buf_notify >= (void *)start && mod_state->xup_mod->mod_send_dma_buf_notify < (void *)(start + size)) {
					mod_state->dma_buf_supported_in_libxup = 1;
					return 1;
				}
			}
		}
	}
	return 0;
}

void XRDPModState::check_dma_buf_supported_in_libxup() {
	// libxup doesn't zero out its struct in init, so in unsupported versions
	// the DMA-BUF pointers are garbage. We check if they actually point to
	// somewhere in the libxup binary.
	dl_iterate_phdr(dma_buf_supported_in_libxup_callback, this);
	log(LOG_DEBUG, "DMA-BUF supported in libxup: %d\n", dma_buf_supported_in_libxup);
}

void XRDPModState::request_dma_buf() {
	if (!dma_buf_supported_in_libxup) {
		log(LOG_DEBUG, "DMA-BUF not supported in libxup, ignoring request to enable it.\n");
		return;
	}
	do_request_dma_buf = 1;
}

void XRDPModState::xup_communicator_thread_func() {
	log(LOG_DEBUG, "xup_communicator_thread_func started.\n");
	while (running) {
//		log(LOG_DEBUG, "xup_communicator_thread_func loop.\n");
		xup_communicator_mutex.lock();
		if (trans_check_wait_objs(xup_mod->trans) != 0) {
			log(LOG_ERROR, "xup_mod->trans closed connection.\n");
			xup_communicator_mutex.unlock();
			qt->exit();
			break;
		}
		process_xrdp_events();
		xup_communicator_mutex.unlock();
		usleep(1000);
	}
	log(LOG_DEBUG, "xup_communicator_thread_func ended.\n");
}

void XRDPModState::event_mouse_move(int x, int y) {
	log(LOG_DEBUG, "event_mouse_move: %d, %d\n", x, y);
	xup_mod->mod_event(xup_mod, WM_MOUSEMOVE, x, y, 0, 0);
}

void XRDPModState::event_mouse_down(int x, int y, int button) {
	log(LOG_DEBUG, "event_mouse_down: %d, %d, %d\n", x, y, button);
	int event = 0;
	switch (button) {
		case 1:
			event = WM_LBUTTONDOWN;
			break;
		case 2:
			event = WM_RBUTTONDOWN;
			break;
		case 4:
			event = WM_BUTTON3DOWN;
			break;
		case 8:
			event = WM_BUTTON8DOWN;
			break;
		case 9:
			event = WM_BUTTON9DOWN;
			break;
		default:
			log(LOG_WARN, "Ignoring unknown button: %d\n", button);
			return;
	}
	xup_mod->mod_event(xup_mod, event, x, y, 0, 0);
}

void XRDPModState::event_mouse_up(int x, int y, int button) {
	log(LOG_DEBUG, "event_mouse_up: %d, %d, %d\n", x, y, button);
	int event = 0;
	switch (button) {
		case 1:
			event = WM_LBUTTONUP;
			break;
		case 2:
			event = WM_RBUTTONUP;
			break;
		case 4:
			event = WM_BUTTON3UP;
			break;
		case 8:
			event = WM_BUTTON8UP;
			break;
		case 9:
			event = WM_BUTTON9UP;
			break;
		default:
			log(LOG_WARN, "Ignoring unknown button: %d\n", button);
			return;
	}
	enqueue_xrdp_event(event, x, y, 0, 0);
}

void XRDPModState::event_scroll_horizontal(int x, int y, int direction) {
	log(LOG_DEBUG, "event_scroll_horizontal: %d, %d, %d\n", x, y, direction);
	int event_down = WM_BUTTON6DOWN;
	int event_up = WM_BUTTON6UP;
	if (direction < 0) {
		event_down = WM_BUTTON7DOWN;
		event_up = WM_BUTTON7UP;
	}
	enqueue_xrdp_event(event_down, x, y, 0, 0);
	enqueue_xrdp_event(event_up, x, y, 0, 0);
}

void XRDPModState::event_scroll_vertical(int x, int y, int direction) {
	log(LOG_DEBUG, "event_scroll_vertical: %d, %d, %d\n", x, y, direction);
	int event_down = WM_BUTTON4DOWN;
	int event_up = WM_BUTTON4UP;
	if (direction < 0) {
		event_down = WM_BUTTON5DOWN;
		event_up = WM_BUTTON5UP;
	}
	enqueue_xrdp_event(event_down, x, y, 0, 0);
	enqueue_xrdp_event(event_up, x, y, 0, 0);
}

void XRDPModState::key_down(int scan_code) {
	log(LOG_DEBUG, "key_down: %d\n", scan_code);
	send_key_event_from_x_scancode(WM_KEYDOWN, scan_code);
}

void XRDPModState::key_up(int scan_code) {
	log(LOG_DEBUG, "key_up: %d\n", scan_code);
	send_key_event_from_x_scancode(WM_KEYUP, scan_code);
}

#define IS_EXT 256
#define IS_SPE 512

// This is basically the reverse of KbdAddEvent in xorgxrdp
void XRDPModState::send_key_event_from_x_scancode(int event_type, int x_scancode) {
	log(LOG_DEBUG, "send_key_event_from_x_scancode: %d, %d\n", event_type, x_scancode);
	switch (x_scancode) {
		case 108: // right alt
			enqueue_xrdp_event(event_type, 0, 0, 56, IS_EXT);
			break;
		case 105: // right ctrl
			enqueue_xrdp_event(event_type, 0, 0, 29, IS_EXT);
			break;
		case 127: // pause
			enqueue_xrdp_event(event_type, 0, 0, 102, 0);
			break;
		case 104: // return (enter on numpad)
			enqueue_xrdp_event(event_type, 0, 0, 28, IS_EXT);
			break;
		case 106: // /
			enqueue_xrdp_event(event_type, 0, 0, 104, IS_EXT);
			break;
		case 107: // Print Screen
			enqueue_xrdp_event(event_type, 0, 0, 55, IS_EXT);
			break;
		case 135: // menu
			enqueue_xrdp_event(event_type, 0, 0, 93, 0);
			break;
		case 111: // up arrow
			enqueue_xrdp_event(event_type, 0, 0, 72, IS_EXT);
			break;
		case 113: // left arrow
			enqueue_xrdp_event(event_type, 0, 0, 75, IS_EXT);
			break;
		case 114: // right arrow
			enqueue_xrdp_event(event_type, 0, 0, 77, IS_EXT);
			break;
		case 116: // down arrow
			enqueue_xrdp_event(event_type, 0, 0, 80, IS_EXT);
			break;
		case 112: // page up
			enqueue_xrdp_event(event_type, 0, 0, 73, IS_EXT);
			break;
		case 117: // page down
			enqueue_xrdp_event(event_type, 0, 0, 81, IS_EXT);
			break;
		case 110: // home
			enqueue_xrdp_event(event_type, 0, 0, 71, IS_EXT);
			break;
		case 115: // end
			enqueue_xrdp_event(event_type, 0, 0, 79, IS_EXT);
			break;
		case 118: // insert
			enqueue_xrdp_event(event_type, 0, 0, 82, IS_EXT);
			break;
		case 119: // delete
			enqueue_xrdp_event(event_type, 0, 0, 83, IS_EXT);
			break;
		default:
			enqueue_xrdp_event(event_type, 0, 0, x_scancode - 8, 0);
			break;
	}
}

void XRDPModState::enqueue_xrdp_event(int msg, tbus param1, tbus param2, tbus param3, tbus param4) {
	if (xup_mod->mod_event == nullptr) {
		return;
	}
	xrdp_event event;
	event.msg = msg;
	event.param1 = param1;
	event.param2 = param2;
	event.param3 = param3;
	event.param4 = param4;
	xrdp_events.push(event);
}

void XRDPModState::process_xrdp_events() {
	while (!xrdp_events.empty()) {
		xrdp_event event = xrdp_events.front();
		xrdp_events.pop();
		if (xup_mod->mod_event != nullptr) {
			xup_mod->mod_event(xup_mod, event.msg, event.param1, event.param2, event.param3, event.param4);
		}
	}
	if (do_request_dma_buf) {
		do_request_dma_buf = 0;
		log(LOG_DEBUG, "Sending DMA_BUF_REQUEST_ACTIVATE\n");
		xup_mod->mod_send_dma_buf_notify(xup_mod, DMA_BUF_REQUEST_ACTIVATE);
	}
}
