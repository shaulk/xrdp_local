#include <sys/select.h>
#include <unistd.h>

#include "common.h"
#include "xrdp_local.h"
#include "xup.h"
#include "qt.h"

XRDPLocalState::XRDPLocalState(const char *socket_path, int feedback_fd) {
	this->feedback_fd = feedback_fd;
	qt = new QtState(this, 1);
	xup = new XRDPModState(this, qt, socket_path);
	notify_feedback_fd("connected");
	qt->launch();
}

XRDPLocalState::~XRDPLocalState() {
	delete qt;
	delete xup;
}

XRDPModState *XRDPLocalState::get_xup() {
	return xup;
}

void XRDPLocalState::notify_feedback_fd(const char *msg) {
	if (feedback_fd < 0) {
		return;
	}
	char buf[1024];
	int count = snprintf(buf, sizeof(buf), "%s\n", msg);
	if (count < 0) {
		return;
	}
	if (write(feedback_fd, buf, count) < count) {
		log(LOG_ERROR, "Failed to write to feedback fd: %s\n", strerror(errno));
	}
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <socket_path> <feedback_fd>\n", argv[0]);
		return 1;
	}

	XRDPLocalState state(argv[1], atoi(argv[2]));

	state.qt->run();

	return 0;
}
