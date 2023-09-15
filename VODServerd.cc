#include "server.hpp"
#include "daemonize.hpp"
#include "logMessage.hpp"

int main() {
	daemonize();
	class log logSvr;
	logSvr.enable();

	aod::server vodSvr(9091);
	vodSvr.runModule();

	return 0;
}
