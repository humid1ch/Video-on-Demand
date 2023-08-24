#include "server.hpp"

void serverTest() {
	aod::server srv(9091);
	srv.runModule();
}

int main() {
	serverTest();

	return 0;
}
