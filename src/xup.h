#ifndef XRDPLOCAL_XUP_H
#define XRDPLOCAL_XUP_H

// xrdp xup client module
// This module is responsible for communicating with xorgxrdp (the driver
// loaded into Xorg by xrdp), by using xrdp's libxup.

// Headers from xrdp/common
extern "C" {
	#include "mock_config_ac.h"
	#include "xup/xup.h"
	#include <xrdp_client_info.h>
}

#include <thread>
#include <mutex>
#include <queue>

#include "qt.h"

// These are private in xrdp, but we need them for the xup client module
typedef intptr_t tbus;
typedef intptr_t tintptr;

// Forward declarations
class XRDPLocalState;
class QtState;

// A queued event to be sent to xorgxrdp
struct xrdp_event {
	int msg;
	tbus param1;
	tbus param2;
	tbus param3;
	tbus param4;
};

// Wraps around libxup and provides a convenient interface to xorgxrdp
class XRDPModState {
	friend XRDPModState *xrdp_mod_state_from_mod(struct mod *mod);

private:
	// Application state
	XRDPLocalState *xrdp_local;
	QtState *qt;

	// The path to the socket that xorgxrdp listens on
	const char *socket_path;

	// The xup module reference as initialized by mod_init
	struct mod *xup_mod;

	// The client info we send to xorgxrdp
	struct xrdp_client_info client_info;

	// dlopen handle to libxup
	void *mod_dl;

	// The libxup init and exit functions
	struct mod *(*mod_init)(void);
	int (*mod_exit)(struct mod *v);

	// Queue of xrdp events we need to send using mod_event
	std::queue<xrdp_event> xrdp_events;

	int do_request_dma_buf = 0;

	// Enqueue an xrdp event to be processed by process_xrdp_events
	void enqueue_xrdp_event(int msg, tbus param1, tbus param2, tbus param3, tbus param4);

	// Send events in the queue using mod_event
	void process_xrdp_events();

	// Setup the xup module
	void setup_xup_mod();

	// Setup the client info we send to xorgxrdp
	void setup_xup_client_info();

	// Fill out our implementation of the xup functions for libxup
	void setup_xup_functions();

	// The thread that polls for messages from xorgxrdp
	void xup_communicator_thread_func();
	std::thread xup_communicator_thread;
	int running = 1;

	// Heuristic to check if libxup is compiled with our DMA-BUF patch.
	// This can be removed if it gets upstreamed.
	int dma_buf_supported_in_libxup = 0;
	static int dma_buf_supported_in_libxup_callback(struct dl_phdr_info *info, size_t size, void *data);
	void check_dma_buf_supported_in_libxup();

	// Synchonizes communication on the xup socket
	std::mutex xup_communicator_mutex;

	// Used by keyboard events to convert xrdp scancodes to xrdp events
	void send_key_event_from_x_scancode(int event_type, int x_scancode);

	// Functions we export to libxup
	// These are all static and use mod->wm to save a pointer to XRDPModState
	static int server_begin_update(struct mod *v);
	static int server_end_update(struct mod *v);

	static int server_fill_rect(struct mod *v, int x, int y, int cx, int cy);

	static int server_screen_blt(struct mod *v, int x, int y, int cx, int cy,
							int srcx, int srcy);
	static int server_paint_rect(struct mod *v, int x, int y, int cx, int cy,
							char *data, int width, int height,
							int srcx, int srcy);

	static int server_set_cursor(struct mod *v, int x, int y, char *data, char *mask);

	static int server_palette(struct mod *v, int *palette);

	static int server_msg(struct mod *v, const char *msg, int code);

	static int server_is_term(void);

	static int server_set_clip(struct mod *v, int x, int y, int cx, int cy);

	static int server_reset_clip(struct mod *v);

	static int server_set_fgcolor(struct mod *v, int fgcolor);

	static int server_set_bgcolor(struct mod *v, int bgcolor);

	static int server_set_opcode(struct mod *v, int opcode);

	static int server_set_mixmode(struct mod *v, int mixmode);

	static int server_set_brush(struct mod *v, int x_origin, int y_origin,
							int style, char *pattern);

	static int server_set_pen(struct mod *v, int style,
						int width);

	static int server_draw_line(struct mod *v, int x1, int y1, int x2, int y2);

	static int server_add_char(struct mod *v, int font, int character,
						int offset, int baseline,
						int width, int height, char *data);

	static int server_draw_text(struct mod *v, int font,
							int flags, int mixmode, int clip_left, int clip_top,
							int clip_right, int clip_bottom,
							int box_left, int box_top,
							int box_right, int box_bottom,
							int x, int y, char *data, int data_len);

	static int client_monitor_resize(struct mod *v, int width, int height,
								int num_monitors,
								const struct monitor_info *monitors);

	static int server_monitor_resize_done(struct mod *v);

	static int server_get_channel_count(struct mod *v);

	static int server_query_channel(struct mod *v, int index,
								char *channel_name,
								int *channel_flags);

	static int server_get_channel_id(struct mod *v, const char *name);

	static int server_send_to_channel(struct mod *v, int channel_id,
								char *data, int data_len,
								int total_data_len, int flags);

	static int server_bell_trigger(struct mod *v);

	static int server_chansrv_in_use(struct mod *v);

	/* off screen bitmaps */
	static int server_create_os_surface(struct mod *v, int rdpindex,
									int width, int height);

	static int server_switch_os_surface(struct mod *v, int rdpindex);

	static int server_delete_os_surface(struct mod *v, int rdpindex);

	static int server_paint_rect_os(struct mod *v, int x, int y,
								int cx, int cy,
								int rdpindex, int srcx, int srcy);

	static int server_set_hints(struct mod *v, int hints, int mask);

	/* rail */
	static int server_window_new_update(struct mod *v, int window_id,
									struct rail_window_state_order *window_state,
									int flags);

	static int server_window_delete(struct mod *v, int window_id);

	static int server_window_icon(struct mod *v,
							int window_id, int cache_entry, int cache_id,
							struct rail_icon_info *icon_info,
							int flags);
	static int server_window_cached_icon(struct mod *v,
									int window_id, int cache_entry,
									int cache_id, int flags);

	static int server_notify_new_update(struct mod *v,
									int window_id, int notify_id,
									struct rail_notify_state_order *notify_state,
									int flags);

	static int server_notify_delete(struct mod *v, int window_id,
								int notify_id);

	static int server_monitored_desktop(struct mod *v,
									struct rail_monitored_desktop_order *mdo,
									int flags);

	static int server_set_cursor_ex(struct mod *v, int x, int y, char *data,
								char *mask, int bpp);

	static int server_add_char_alpha(struct mod *v, int font, int character,
								int offset, int baseline,
								int width, int height, char *data);

	static int server_create_os_surface_bpp(struct mod *v, int rdpindex,
										int width, int height, int bpp);

	static int server_paint_rect_bpp(struct mod *v, int x, int y, int cx, int cy,
								char *data, int width, int height,
								int srcx, int srcy, int bpp);

	static int server_composite(struct mod *v, int srcidx, int srcformat, int srcwidth,
							int srcrepeat, int *srctransform, int mskflags, int mskidx,
							int mskformat, int mskwidth, int mskrepeat, int op,
							int srcx, int srcy, int mskx, int msky,
							int dstx, int dsty, int width, int height, int dstformat);

	static int server_paint_rects(struct mod *v,
							int num_drects, short *drects,
							int num_crects, short *crects,
							char *data, int width, int height,
							int flags, int frame_id);

	static int server_session_info(struct mod *v, const char *data,
							int data_bytes);

	static int server_set_pointer_large(struct mod *v, int x, int y,
									char *data, char *mask, int bpp,
									int width, int height);

	static int server_paint_rects_ex(struct mod *v,
								int num_drects, short *drects,
								int num_crects, short *crects,
								char *data, int left, int top,
								int width, int height,
								int flags, int frame_id,
								void *shmem_ptr, int shmem_bytes);

	static int server_egfx_cmd(struct mod *v,
						char *cmd, int cmd_bytes,
						char *data, int data_bytes);

	static int server_dma_buf_notify(struct mod *v,
							enum dma_buf_server_notify state);
	static int server_dma_buf_receive_pixmap_fd(struct mod *v,
							int fd, uint32_t width,
							uint32_t height, uint16_t stride,
							uint32_t size, uint32_t format);
	static int server_dma_buf_deactivate(struct mod *v);
	static int server_dma_buf_paint_pixmap(struct mod *v);

public:
	XRDPModState(XRDPLocalState *xrdp_local, QtState *qt, const char *socket_path);
	~XRDPModState();

	// Event handlers called by the Qt client
	void event_mouse_move(int x, int y);
	void event_mouse_down(int x, int y, int button);
	void event_mouse_up(int x, int y, int button);
	void event_scroll_horizontal(int x, int y, int direction);
	void event_scroll_vertical(int x, int y, int direction);
	void key_down(int scan_code);
	void key_up(int scan_code);

	void request_dma_buf();
};

// Helper function to get the XRDPModState from the xup module reference
XRDPModState *xrdp_mod_state_from_mod(struct mod *mod);

#endif // XRDPLOCAL_XUP_H
