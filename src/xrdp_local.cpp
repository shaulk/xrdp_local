#include <sys/select.h>
#include <unistd.h>
#include <argparse/argparse.hpp>

#include "common.h"
#include "xrdp_local.h"
#include "xup.h"
#include "qt/state.h"

XRDPLocalState::XRDPLocalState(const char *socket_path, int feedback_fd, int max_displays, bool use_dma_buf) {
	this->feedback_fd = feedback_fd;
	qt = new QtState(this, max_displays, use_dma_buf);
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
	argparse::ArgumentParser program("xrdp_local");

	program.add_argument("-v", "--verbose")
		.help("verbose output")
		.default_value(false)
		.implicit_value(true);

	program.add_argument("socket-path")
		.help("set the socket path");

	program.add_argument("feedback-fd")
		.help("set the feedback fd")
		.default_value(-1)
		.scan<'i', int>();

	program.add_argument("--max-displays")
		.help("set the maximum number of displays")
		.default_value(0)
		.scan<'i', int>();

	program.add_argument("--disable-dma-buf")
		.help("disable DMA-BUF acceleration")
		.default_value(true)
		.implicit_value(false);

	try {
		program.parse_args(argc, argv);
	} catch (const std::runtime_error& err) {
		fprintf(stderr, "%s\n", err.what());
		return 1;
	}

	set_log_level(program.get<bool>("-v") ? LOG_DEBUG : LOG_INFO);

	XRDPLocalState state(program.get<std::string>("socket-path").c_str(), program.get<int>("feedback-fd"), program.get<int>("--max-displays"), program.get<bool>("--disable-dma-buf"));

	state.qt->run();

	return 0;
}
