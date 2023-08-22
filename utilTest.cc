#include "util.hpp"
#include <iostream>
#include <string>

void fileUtilTest() {
	// 创建web根目录文件夹
	aod::fileUtil("./testWwwRoot").createDir();
	// 设置主页文件内容
	aod::fileUtil("./testWwwRoot/index.html").setContent("<html></html>");

	std::string body;
	// 获取主页文件内容
	aod::fileUtil("./testWwwRoot/index.html").getContent(&body);
	std::cout << body << std::endl;
	std::cout << aod::fileUtil("./testWwwRoot/index.html").size() << std::endl;
}

void jsonUtilTest() {
	Json::Value root;
	root["姓名"] = "代潇莹";
	root["年龄"] = 21;
	root["性别"] = "女";
	root["成绩"].append(78);
	root["成绩"].append(78);
	root["成绩"].append(78);
	root["成绩"].append(132);

	std::string body;
	aod::jsonUtil::serialize(root, &body);
	std::cout << body << std::endl;
	std::cout << std::endl;

	Json::Value result;
	aod::jsonUtil::deserialize(body, &result);
	std::cout << result["姓名"].asString() << std::endl;
	std::cout << result["年龄"].asInt() << std::endl;
	std::cout << result["性别"].asString() << std::endl;
	for (int i = 0; i < result["成绩"].size(); i++) {
		std::cout << result["成绩"][i] << std::endl;
	}
}

int main() {
	// fileUtilTest();
	jsonUtilTest();

	return 0;
}
