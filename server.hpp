#include "dataManage.hpp"
#include "logMessage.hpp"
#include "util.hpp"
#include "httplib.h"
#include <cstdio>
#include <cstdlib>
#include <string>

namespace aod {
	const std::string WWW_ROOT = "./www";
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
			_srv.set_mount_point("/", WWW_ROOT);
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
			_srv.Get("/video", selectAllV);
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
				resp.status = 500;
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
		}
		static void selectOneV(const httplib::Request& req, httplib::Response& resp) {}
		static void selectAllV(const httplib::Request& req, httplib::Response& resp) {}

	private:
		int _port;
		httplib::Server _srv;
	};
} // namespace aod
