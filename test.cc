#include <string>
#include <iostream>

#define WWW_ROOT "./www"
#define VIDEO_ROOT "/covers/"
#define COVER_ROOT "/videos/"

std::string videoRealPath = std::string(WWW_ROOT) + std::string(VIDEO_ROOT);
std::string coverRealPath = std::string(WWW_ROOT) + std::string(COVER_ROOT);

int main() {
	std::cout << videoRealPath << std::endl;
	std::cout << coverRealPath << std::endl;

	return 0;
}
