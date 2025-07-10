#ifndef XRDPLOCAL_H
#define XRDPLOCAL_H

#include "xup.h"
#include "qt.h"

int main(int argc, char *argv[]);

class QtState;
class XRDPModState;


class XRDPLocalState {
	friend class QtState;
	friend int main(int argc, char *argv[]);

private:
	int feedback_fd;
	XRDPModState *xup;
	QtState *qt;
	char xrdpdev_socket_path[1024];
	void notify_feedback_fd(const char *msg);

public:
	XRDPLocalState(const char *socket_path, int feedback_fd);
	~XRDPLocalState();

	XRDPModState *get_xup();
};


#endif // XRDPLOCAL_H
