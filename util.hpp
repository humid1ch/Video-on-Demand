#pragma once

#include <cstdint>
#include <ios>
#include <iostream>
#include <fstream>
#include <json/reader.h>
#include <json/writer.h>
#include <ostream>
#include <sstream>
#include <string>
#include <memory>

#include <json/json.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logMessage.hpp"

namespace aod {
	class fileUtil {
	public:
		fileUtil(const std::string& filename)
			: _filename(filename) {}

		// 判断文件是否存在
		bool exists() {
			// 这里直接使用 linux 系统调用接口 access();
			int ret = access(_filename.c_str(), F_OK); // 如果 第二个参数传入 F_OK 就返回0, 表示已经存在
			if (ret != 0) {
				LOG(WARNING, "exists():: file(%s) does not exists, will be created!", _filename.c_str());
				return false;
			}

			return true;
		}
		// 获取文件大小
		std::int64_t size() {
			// linux 系统调用接口 stat(), 可以获取文件的一些相关属性
			// 具体可以通过 man 2 stat 查看
			struct stat fileStat;
			int ret = stat(_filename.c_str(), &fileStat); // 获取成功返回0
			if (ret != 0) {
				LOG(WARNING, "size():: get file(%s) stat failed!", _filename.c_str());
				return 0;
			}

			return fileStat.st_size;
		}
		// 从文件中读取数据到 内存中
		bool getContent(std::string* body) {
			std::ifstream ifs(_filename, std::ios::binary);
			if (!ifs.is_open()) {
				LOG(WARNING, "getContent():: open file(%s) failed!", _filename.c_str());
				return false;
			}

			// std::streamsize 是有符号整型
			std::streamsize fileLenth = this->size(); // 获取文件大小
			body->resize(fileLenth);
			ifs.read(&(*body)[0], fileLenth);
			if (!ifs.good()) {
				LOG(WARNING, "getContent():: read file(%s) content failed!", _filename.c_str());
				ifs.close(); // 返回前 记得关闭文件流
				return false;
			}

			ifs.close();

			return true;
		}
		// 将内存中的数据 写入到文件中
		bool setContent(const std::string& body) {
			std::ofstream ofs(_filename, std::ios::binary);
			if (!ofs.is_open()) {
				LOG(WARNING, "setContent():: open file(%s) failed!", _filename.c_str());
				return false;
			}
			ofs.write(body.c_str(), body.size());
			if (!ofs.good()) {
				LOG(WARNING, "setContent():: write file(%s) content failed!", _filename.c_str());
				ofs.close();
				return false;
			}

			ofs.close();

			return true;
		}
		// 创建目录?
		bool createDir() {
			if (this->exists()) {
				return true;
			}
			int ret = mkdir(_filename.c_str(), 0766);
			if (ret != 0) {
				LOG(WARNING, "createDir():: mkdir %s failed!", _filename.c_str());
				return false;
			}

			return true;
		}

	private:
		std::string _filename;
	};

	class jsonUtil {
		// 需要实现 数据的序列化与反序列化
	public:
		// 序列化, 将Json::Value 内容, 序列化为字符串并设置到 body中
		static bool serialize(const Json::Value& root, std::string* body) {
			Json::StreamWriterBuilder swb;
			std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

			std::stringstream ss;
			int ret = sw->write(root, &ss); // 写入字符串流中
			// 源码中 StreamWriter::write() 返回值, 写入成功返回0
			if (ret != 0) {
				LOG(WARNING, "jsonUtil::serialize():: StreamWriter::write() failed!");
				return false;
			}

			*body = ss.str();

			return true;
		}
		static bool deserialize(const std::string& body, Json::Value* root) {
			Json::CharReaderBuilder crb;
			std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

			std::string err;
			bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), root, &err);
			// 源码中, CharReader::parse() 的接口返回值是 bool 类型的.
			if (!ret) {
				LOG(WARNING, "jsonUtil::deserialize():: CharReader::parse() failed!");
				return false;
			}

			return true;
		}
	};
} // namespace aod
