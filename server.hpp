#include "dataManage.hpp"
#include "logMessage.hpp"
#include "util.hpp"
#include "httplib.h"
#include <cstdio>
#include <cstdlib>
#include <string>

namespace aod {
	const std::string WWW_ROOT = "./www";
    // 视频路径 以及 封面路径 可以不在本项目中, 否则git会将视频和封面一起提交
	const std::string VIDEO_ROOT = "/video/";
	const std::string COVER_ROOT = "/cover/";

	tableVideo* tbVideo = NULL;
	class server {
	public:
		server(int port)
			: _port(port) {}

		// 运行模块
		bool runModule() {
			// 主要负责 服务端的初始化等操作
			// 1. 初始化 数据管理类
			tbVideo = new tableVideo();
			// 2. 创建资源目录
			std::string videoRealPath = WWW_ROOT + VIDEO_ROOT; // 视频资源路径
			std::string coverRealPath = WWW_ROOT + COVER_ROOT; // 封面资源路径
			fileUtil(WWW_ROOT).createDir();
			fileUtil(videoRealPath).createDir();
			fileUtil(coverRealPath).createDir();

			// 3. 设置服务器 静态资源根目录
			_srv.set_mount_point("/", WWW_ROOT.c_str());
			// 4. 设置 请求方法 与 处理函数的映射
			//  使用 restful 风格设计: GET 对应查询操作, POST 对应添加操作, PUT 对应修改操作, DELETE 对应删除操作

			// 请求对应的处理方法(回调函数)中需要传入的 httplib::Request 和 httplib::Response 都是 在即将调用回调函数时 httplib::Server自动创建的
			// 同时 httplib::Request 也会被填充
			//  1. 新增, 即上传视频
			_srv.Post("/video", insertV);
			// 新增, 使用http协议 POST方法, 向 www/video/ 上传视频

			//  2. 删除
			_srv.Delete("/video/(\\d+)", deleteV);
			// 删除, 使用http协议 DELETE方法, 将/video/指定id视频删除. (\\d+) C++中的正则表达式, 表示获取多个一位数字, 即一个整数.
			// \d 表示一位整数[0, 9], + 表示获取多个前面的内容, ()表示单独捕捉括号内的内容. 即 如果是 /video/1234 就会捕捉到1234

			// http请求的url 会被存储到 httplib::Request 的一个 数组中: matches.
			// matches[0] 会存储整个请求url, 而之后的 matches[1] 或 matches[2]... 则按照顺序存储 ()捕获到的内容
			// 比如, 如果    "/video/(\\d+)/xxxx/(\\d+)/yyyyy/(\\d+)"
			// 而请求的url是 "/video/12344/love/5678/me/890"
			// 那么 matches[0]:"/video/12344/love/5678/me/890", matches[1]: "12344", matches[2]: "5678", matches[3]: "890"

			// 有关于matches是什么类型的, 其存储元素是什么类型的, 下面是httplib::Request 中的相关定义
			// Match matches;
			// using Match = std::smatch;
			// 官方文档中, std::smatch: typedef match_results<string::const_iterator> smatch;
			// match_results:
			// template<class BidirectionalIterator, class Alloc = allocator<sub_match<BidirectionalIterator>>>
			// class match_results;
			// 那么 matches 的类型就是 match_results<std::string::const_iterator>, 可以看作是一个存储string::cbegin()的容器, 是专门用来存储正则表达式的相关匹配项的
			// 它有一些接口, 比如 operator[] 返回对应的匹配项, str() 以string类型返回对应的匹配项... 相关内容可以去查看官方文档

			//  3. 更新
			_srv.Put("/video/(\\d+)", updateV);

			//  4. 查找所有 和 模糊查找
			// _srv.Get("/video", selectAllV);
			_srv.Get("/video", videoHandle);
			// 查找所有 和 模糊查找请求头相似, 所有: GET /video HTTP/1.1, 模糊: GET /video?search=key HTTP/1.1
			// 所以 一起使用 selectAllV

			//  5. 指定查找
			_srv.Get("/video/(\\d+)", selectOneV);

			// 启动服务器
			_srv.listen("0.0.0.0", _port);

			return true;
		}

		~server() {
			delete tbVideo;
		}

	private:
		// _srv.Post("/video", insertV);
		static void insertV(const httplib::Request& req, httplib::Response& resp) {
			// 添加视频的操作, 实际就是 html 通过form表单 上传文件 然后提交请求
			// form 表单可以设置数据编码类型 `enctype="multipart/form-data"`
			// 设置了之后, 表单数据在创建为请求时 就会设置成 multipart/form-data 格式的
			// 这个格式具体怎么样可以搜一下.
			// 这里只简单提一下 form表单数据格式设置成 multipart/form-data 之后, 请求的Content-Type就会被设置为 multipart/form-data; bonudary:xxxxxxxxxxxx;
			// 然后请求的内容 就是以 bonudary内容分割开的 上传的各数据的 multipart/form-data 格式

			// 并且, 可以通过每个 multipart 的name 在请求中获取 MultipartFormData 类
			// 此类中包括 name(内容名), Content-Type(内容类型), filename(原文件名), Content(真正内容)

			// 比如, 如果在form表单中的一个 name="title" 的文本框输入了 "变形金刚10086", 那么请求中就会设置 multipart/form-data 格式,
			// 通过httplib 可以获取到 MultipartFormData类:
			// name="title", Content-Type="text/plain", filename="", Content="变形金刚10086"
			// 再比如, 如果 使用form表单中的一个上传图片按钮(名叫imgae) 上传了一个图片文件, 那么请求中对应的就会设置 multipart/form-data 格式
			// 同样可以通过 httplib 获取到MultipartFormData 类:
			// name="image", Content-Type="image/png...", filename="原图片名.后缀", Content="字符串形式的二进制内容"

			// 尽量在前端就保证 不会上传空内容, 比如 标题 简介 还有文件 都不为空
			if (!req.has_file("title") || !req.has_file("info") || !req.has_file("videoFile") || !req.has_file("coverFile")) {
				// 不存在相关数据
				// 设置响应
				resp.status = 400;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"({"result": false, "reason": "用户上传信息/数据错误"})"; // json 格式传入数据
				return;
			}

			// 先获取请求中携带的数据
			httplib::MultipartFormData title = req.get_file_value("title");		// 获取html输入并上传的标题 的 multipart/form-data 数据
			httplib::MultipartFormData info = req.get_file_value("info");		// 获取html输入并上传的简介 的 multipart/form-data 数据
			httplib::MultipartFormData video = req.get_file_value("videoFile"); // 获取html上传的视频文件本体 的 multipart/form-data 数据
			httplib::MultipartFormData cover = req.get_file_value("coverFile"); // 获取html上传的封面文件本体 的 multipart/form-data 数据

			std::string v_title = title.content;
			std::string v_info = info.content;
			std::string v_path = WWW_ROOT + VIDEO_ROOT + video.filename;
			std::string v_cover = WWW_ROOT + COVER_ROOT + cover.filename;

			// 然后就可以根据 v_path 和 v_cover 调用 fileUtil 里的接口, 实现对文件的创建 以及 内容的填充
			if (!fileUtil(v_path).setContent(video.content)) {
				LOG(WARNING, "server::insertV():: set video file content failed!");
				// 还要设置 响应
				resp.status = 500;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"({"result": false, "reason": "视频文件内容设置失败"})";
				return;
			}
			if (!fileUtil(v_cover).setContent(cover.content)) {
				LOG(WARNING, "server::insertV():: set cover file content failed!");
				resp.status = 500;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"({"result": false, "reason": "封面文件内容设置失败"})";
				return;
			}

			// 调用fileUtil::setContent 实际就是创建对应的文件, 接受来自客户端上传的文件
			// 文件上传结束之后, 就可以填充数据库了
			Json::Value videoValue;
			videoValue["v_title"] = v_title;
			videoValue["v_info"] = v_info;
			// 视频文件 和 封面文件的填充, 不需要 添加 .www/ 因为, 我们已经设置了 资源根目录 `/` 为 ./www
			// 在请求 /* 资源时, 会默认在./www 下进行查找
			videoValue["v_path"] = std::string(VIDEO_ROOT) + video.filename;
			videoValue["v_cover"] = std::string(COVER_ROOT) + cover.filename;
			// Json::Value 需要按照 tableVideo::insertVideo() 接口内部的实现 进行填充

			// 填充之后, 就可以添加在数据库中了
			if (!tbVideo->insertVideo(videoValue)) {
				// 添加失败
				LOG(WARNING, "server::insertV():: insert video to database failed!");
				resp.status = 500;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"({"result": false, "reason": "数据库新增视频信息失败"})";
				return;
			}

			resp.set_redirect("/index.html", 303);
			// 添加完成之后, 设置重定向状态码以及 url, 以实现返回主页
			// 状态码303, 可以看作此次重定向, see other 本次响应其他链接
		}

		// _srv.Delete("/video/(\\d+)", deleteV);
		static void deleteV(const httplib::Request& req, httplib::Response& resp) {
			// 删除的请求中, 是携带有 videoId 的, 并且 已经通过正则表达式捕获到了
			// int videoId = atoi(req.matches[1].str().c_str());
			int videoId = atoi(req.matches.str(1).c_str());
			// 获取到视频id 之后, 就可以在数据库中查询内容了
			// tableVideo::selectOneVideo(int videoId, Json::Value *video)
			Json::Value videoValue;
			if (!tbVideo->selectOneVideo(videoId, &videoValue)) {
				// 查询失败
				resp.status = 400;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "相关视频信息不存在"}")";
				return;
			}
			// 走到这里, 查询到的视频相关信息就都在 videoValue 中了
			// 然后需要 删除视频文件 删除封面文件 删除数据库数据
			std::string videoRealPath = WWW_ROOT + videoValue["v_path"].asString(); // 获取文件在系统中的位置
			std::string coverRealPath = WWW_ROOT + videoValue["v_cover"].asString();
			remove(videoRealPath.c_str()); // 系统调用, 删除Linux系统中文件
			remove(coverRealPath.c_str());
			// 删除数据库数据
			if (!tbVideo->deleteVideo(videoId)) {
				// 删除失败
				resp.status = 500;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "删除数据库中 相关视频数据失败"}")";
				return;
			}
		}

		// _srv.Put("/video/(\\d+)", updateV);
		static void updateV(const httplib::Request& req, httplib::Response& resp) {
			// 更新操作, 实现更新标题和简介
			// 更新操作 首先需要根据 videoId 查询数据库中的相关视频数据, 保证数据存在
			// 获取到请求中的 视频id
			int videoId = atoi(req.matches.str(1).c_str());
			// 查询相关内容
			Json::Value videoValue;
			if (!tbVideo->selectOneVideo(videoId, &videoValue)) {
				// 查询失败
				resp.status = 400;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "查询相关视频信息失败"}")";
				return;
			}

			// 走到这里查询成功
			// 需要 获取相关 请求中携带的需要更新的 标题和简介信息
			// 如何获取呢? (与 insertV 完全不同, insertV是通过表单完成了 multipart/form-data 格式化)
			// 我们之前已经规定了, 更新请求的正文内容 是 序列化的json串: {"v_title":"新标题", "v_info":"新简介"}
			// 所以, 我们可以针对 更新请求正文 进行反序列化 获取对应的更新信息
			Json::Value updateValue;
			if (!jsonUtil::deserialize(req.body, &updateValue)) {
				// 反序列化失败
				resp.status = 400;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "反序列化更新信息失败"}")";
				return;
			}
			// 此时 updateValue, 应该已经存在 updateValue["v_title"]: "新标题", updateValue["v_info"]: "新简介"

			// 更新数据库信息
			if (!tbVideo->updateVideo(videoId, updateValue)) {
				// 更新失败
				resp.status = 500;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "更新数据库中 相关视频信息失败"}")";
				return;
			}
		}

		// _srv.Get("/video/(\\d+)", selectOneV);
		static void selectOneV(const httplib::Request& req, httplib::Response& resp) {
			// 指定查找也是根据视频id查找的
			int videoId = atoi(req.matches.str(1).c_str());

			// 在数据库中查询相关数据
			Json::Value videoValue;
			if (!tbVideo->selectOneVideo(videoId, &videoValue)) {
				// 没找到 或 查询失败
				resp.status = 400;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "查询相关视频信息失败"}")";
				return;
			}

			// 查询成功, 就需要序列化并设置相应数据类型以及相关内容 到响应对象中
			if (!jsonUtil::serialize(videoValue, &resp.body)) {
				// 序列化失败
				resp.status = 400;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "响应设置序列化数据失败"}")";
				return;
			}
			// 走到这里 resp的正文内容就已经包含了一个 序列化的视频信息
			// 正文内容是json 序列, 所以 需要设置正文类型为 application/json类型
			resp.set_header("Content-Type", "application/json");
		}

		static void videoHandle(const httplib::Request& req, httplib::Response& resp) {
			// 1. 判断是否请求具体文件
			if (req.path.find(".") != std::string::npos) {
				// 请求包含".",可能是文件请求
				videoFileGet(req, resp);
			}
			else {
				// 普通视频信息查询请求
				selectAllV(req, resp);
			}
		}

		static void selectAllV(const httplib::Request& req, httplib::Response& resp) {
			// 这个接口稍微与指定查找有一些不同
			// 因为 这个接口要同时处理 查找所有 和 模糊查找的请求, 因为 这两个请求的url资源路径是相同的
			// 不同的是 我们规定了 模糊查找在资源的请求路径之后 要有一堆键值: search=xxxxx 的键值
			// httplib::Request 中提供了判断是否存在键值对 以及 获取键值内容的接口: has_param() 和 gei_param_value()

			// 1. 判断是否存在 search 键值, 如果存在 就记录是模糊查找, 后面执行模糊查找的操作
			bool LikeSelect = false;
			std::string searchKey;
			if (req.has_param("search")) {
				// 有search键值对, 就记录需要模糊查找
				LikeSelect = true;
				searchKey = req.get_param_value("search"); // 获取查找关键字, 也就是 search键的值

				// 可以单独处理 search值为空的情况. 可以提醒用户重新搜索
				// 或者 在前端禁止用户搜索空值
			}

			Json::Value videoValues;
			if (LikeSelect) {
				// 模糊查找
				if (!tbVideo->selectLikeVideo(searchKey, &videoValues)) {
					// 没找到 或 查询失败
					resp.status = 400;
					resp.set_header("Content-Type", "application/json");
					resp.body = R"("{"result": false, "reason": "查询相关视频信息失败"}")";
					return;
				}
			}
			else {
				// 全部查找
				if (!tbVideo->selectAllVideo(&videoValues)) {
					// 查询失败
					resp.status = 400;
					resp.set_header("Content-Type", "application/json");
					resp.body = R"("{"result": false, "reason": "查询相关视频信息失败"}")";
					return;
				}
			}
			// 查找到的数据, 都已经存储到了 videoValues 中了
			// 此时需要进行序列化并设置到相应正文中
			if (!jsonUtil::serialize(videoValues, &resp.body)) {
				// 序列化失败
				resp.status = 400;
				resp.set_header("Content-Type", "application/json");
				resp.body = R"("{"result": false, "reason": "响应设置序列化数据失败"}")";
				return;
			}

			// 走到这里就是成功找到了, 而且已经设置了响应正文
			// 不过还需要设置一下 响应正文类型
			resp.set_header("Content-Type", "application/json");
		}

		static void videoFileGet(const httplib::Request& req, httplib::Response& resp) {
			std::string videoRealPath = WWW_ROOT + VIDEO_ROOT; // 视频资源路径
			std::string fileName = req.path.substr(7);		   // 剪切掉"/video/" 获取文件名
			std::string filePath = videoRealPath + fileName;
			if (req.has_header("Range")) {
				// 处理Range请求
				std::int64_t startByte, endByte;
				auto rangeHeader = req.get_header_value("Range");
				std::ifstream video(filePath, std::ios::binary);
				video.seekg(startByte);
				size_t totalSize = video.tellg();
				std::string rangeData(endByte - startByte + 1, 0);
				video.read(&rangeData[0], rangeData.size());

				resp.status = 206;
				std::string rangeStr = "bytes 0-" + std::to_string(endByte) + "/" + std::to_string(totalSize);
				resp.set_header("Content-Range", rangeStr);
				resp.set_header("Content-Length", std::to_string(endByte));
			}
		}

	private:
		int _port;
		httplib::Server _srv;
	};
} // namespace aod
