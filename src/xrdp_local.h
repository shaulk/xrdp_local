#ifndef XRDPLOCAL_H
#define XRDPLOCAL_H

// Main application class

#include "xup.h"
#include "qt.h"

class QtState;
class XRDPModState;

int main(int argc, char *argv[]);

// Main application state class
class XRDPLocalState {
	friend class QtState;
	friend int main(int argc, char *argv[]);

private:
	// The file descriptor to write feedback to
	// Used by a wrapper script to know that we've successfully connected to
	// Xorg and are displaying the screen.
	int feedback_fd;

	// The xup client state
	XRDPModState *xup;

	// The Qt client state
	QtState *qt;

	// The path to the xrdpdev socket
	char xrdpdev_socket_path[1024];

	// Notify the wrapper script of an event using the feedback fd
	void notify_feedback_fd(const char *msg);

public:
	XRDPLocalState(const char *socket_path, int feedback_fd, int max_displays, bool use_dma_buf);
	~XRDPLocalState();

	// Getters
	XRDPModState *get_xup();
};

#endif // XRDPLOCAL_H
