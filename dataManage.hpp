#include "logMessage.hpp"
#include "util.hpp"
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <mysql/mysql.h>
#include <ostream>
#include <string>
#include <unistd.h>

#define HOST "127.0.0.1"	 // 连接数据库地址
#define USER "root"			 // 连接数据库用户名
#define PASSWD ""			 // 用户名密码
#define NAME "AoD_System_DB" // 数据库名

namespace aod {
	// mysqlInit() 初始化操作
	// 主要实现 数据库句柄获取  数据库连接 以及 客户端字符集设置 相关操作
	static MYSQL* mysqlInit() {
		MYSQL* mysql = mysql_init(NULL);
		if (mysql == NULL) {
			LOG(WARNING, "mysqlInit():: get mysql handler failed!");
			return NULL;
		}

		// 获取数据库句柄之后, 就可以连接数据库的
		mysql = mysql_real_connect(mysql, HOST, USER, PASSWD, NAME, 0, NULL, 0);
		if (mysql == NULL) {
			LOG(WARNING, "mysqlInit():: connect mysql server failed!");
			mysql_close(mysql);
			return NULL;
		}

		int ret = mysql_set_character_set(mysql, "utf8");
		if (ret != 0) {
			LOG(WARNING, "mysqlInit():: mysql client set character failed!");
			mysql_close(mysql);
			return NULL;
		}

		return mysql;
	}

	// mysqlDestroy()
	// 主要实现 数据库句柄销毁的相关操作
	static void mysqlDestroy(MYSQL* mysql) {
		if (mysql != NULL) {
			mysql_close(mysql);
			// LOG(NOTICE, "mysql handler destroy success!");
		}
		else {
			LOG(NOTICE, "mysql handler does not exists, do nothing!");
		}
	}

	// mysqlQuery()
	// 主要实现 数据库的查询操作
	// 需要通过此接口实现各种查询操作
	static bool mysqlQuery(MYSQL* mysql, const std::string& sql) {
		if (mysql == NULL) {
			LOG(WARNING, "mysql handler does not exists!");
			return false;
		}

		int ret = mysql_query(mysql, sql.c_str());
		if (ret != 0) {
			LOG(WARNING, "mysqlQuery():: sql(%s) query failed! error:: %s!", sql.c_str(), mysql_error(mysql));
			return false;
		}

		return true;
	}

	// 接口封装了, 下面就是 数据管理类的实现了
	// 类需要实现一些功能, 比如 添加数据 更新数据 删除数据 查询数据等
	class tableVideo {
	public:
		tableVideo() {
			_mysql = mysqlInit();
			if (_mysql == NULL) {
				LOG(FATAL, "tableVideo():: mysqlInit() failed!, process is exiting!");
				sleep(1);
				LOG(FATAL, "process exit success!");
				exit(-1);
			}
		}
		~tableVideo() {
			mysqlDestroy(_mysql);
		}

		// 插入数据, 都是反序列化之后的数据
		bool insertVideo(const Json::Value& video) {
			// 规定video 数据 按照: v_title v_info v_path v_cover 顺序进行序列化和反序列化
			// v_info 可能会很长
			std::string sql;									  // 用来存储sql语句
			sql.resize(4096 + video["v_info"].asString().size()); // v_info 是简介内容, 4096 是为其他字段预留的内容

			// 插入数据的操作
			// 注意, 我们预先创建的 tb_video 表结构是这样的: v_id(主键\自增) v_title v_info v_path v_cover owner_id(扩展用);
#define INSERT_VIDEO "INSERT INTO tb_video(v_title, v_info, v_path, v_cover) VALUE('%s', '%s', '%s', '%s');"

			sprintf(&sql[0], INSERT_VIDEO,
					video["v_title"].asCString(),
					video["v_info"].asCString(),
					video["v_path"].asCString(),
					video["v_cover"].asCString());
			// 将 sql 内容格式化为 INSERT INTO tb_video(v_title, v_info, v_path, v_cover) VALUE(v_title, v_info, v_path, v_cover)
			_mutex.lock();
			// 该执行sql了, 先上锁
			bool ret = mysqlQuery(_mysql, sql);
			_mutex.unlock();

			return ret;
		}

		// 更新操作
		bool updateVideo(int videoId, const Json::Value& video) {
			std::string sql;
			sql.resize(4096 + video["v_info"].asString().size());

			// 我们这里不设置对视频封面和内容的修改, 只修改标题和简介
#define UPDATE_VIDEO "UPDATE tb_video SET v_title='%s', v_info='%s' WHERE v_id=%d;"
			sprintf(&sql[0], UPDATE_VIDEO,
					video["v_title"].asCString(),
					video["v_info"].asCString(),
					videoId);
			_mutex.lock();
			// 该执行sql了, 先上锁
			bool ret = mysqlQuery(_mysql, sql);
			_mutex.unlock();

			return ret;
		}

		// 删除操作
		bool deleteVideo(int videoId) {
			std::string sql;
			sql.resize(1024);
#define DELETE_VIDEO "DELETE FROM tb_video WHERE v_id=%d;"
			sprintf(&sql[0], DELETE_VIDEO, videoId);
			_mutex.lock();
			// 该执行sql了, 先上锁
			bool ret = mysqlQuery(_mysql, sql);
			_mutex.unlock();

			return ret;
		}

		/* 以上数据操作 实际是需要对数据进行校验的, 因为上传视频的各种内容 可能会为空*/

		// 查询数据操作:
		// 1. 查询表中所有数据
		bool selectAllVideo(Json::Value* videos) {
#define SELECTALL_VIDEO "SELECT * FROM tb_video;"
			// 需要保证查询 和 获取结果集 过程 互斥
			// 因为可能存在多线程查询, 结果集会改变
			// 并且, 不允许在修改数据时 查询
			_mutex.lock();
			bool ret = mysqlQuery(_mysql, SELECTALL_VIDEO);
			if (!ret) {
				LOG(WARNING, "selectAllVideo():: mysqlQuery(%s) failed!", SELECTALL_VIDEO);
				_mutex.unlock(); // 这里要退出了, 要解锁
				return false;
			}

			// 获取结果集
			MYSQL_RES* result = mysql_store_result(_mysql);
			if (result == NULL) {
				LOG(WARNING, "selectAllVideo():: get query result failed!");
				_mutex.unlock();
				return false;
			}
			_mutex.unlock();
			// 下面就不在数据库操作了, 所以可以解锁了
			// 然后就可以将 查询到的结果记录到 Json::Value 中了

			// 获取结果行数
			int rowNum = mysql_num_rows(result);
			for (int i = 0; i < rowNum; i++) {
				MYSQL_ROW rowData = mysql_fetch_row(result); // 获取当前行数据

				// 每行数据 v_id v_title v_info v_path v_cover owner_id
				Json::Value video;
				video["v_id"] = atoi(rowData[0]); // 结果集中的数据都是以字符串形式存储的
				video["v_title"] = rowData[1];
				video["v_info"] = rowData[2];
				video["v_path"] = rowData[3];
				video["v_cover"] = rowData[4];
				video["v_updatetime"] = rowData[5];
				// video["owner_id"] = rowData[5]; // owner_id 当前为空, 不用做转换
				videos->append(video);
			}
			mysql_free_result(result); // 释放结果集

			return true;
		}

		// 2. 查询指定数据
		bool selectOneVideo(int videoId, Json::Value* video) {
			std::string sql;
			sql.resize(1024);
#define SELECTONE_VIDEO "SELECT * FROM tb_video WHERE v_id=%d;"
			sprintf(&sql[0], SELECTONE_VIDEO, videoId);

			// 查询
			_mutex.lock();
			bool ret = mysqlQuery(_mysql, sql);
			if (!ret) {
				LOG(WARNING, "selectOneVideo():: mysqlQuery(%s) failed!", sql.c_str());
				_mutex.unlock();
				return false;
			}
			// 获取结果集
			MYSQL_RES* result = mysql_store_result(_mysql);
			if (result == NULL) {
				// 只有查询失败才会为NULL, 如果实际只是 查询不到, 那么还是会返回一个 MYSQL_RES的指针的
				LOG(WARNING, "selectOneVideo():: get query result failed!");
				_mutex.unlock();
				return false;
			}
			_mutex.unlock();
			// 此次获取最多只有一行结果, 因为是根据 videoId 查询的, 此字段是唯一的
			if (mysql_num_rows(result) == 0) {
				// result 中 数据行数为0, 表示没有查到数据, 但并不是查询失败
				LOG(WARNING, "selectOneVideo():: select nothing!");
				return false;
			}
			MYSQL_ROW data = mysql_fetch_row(result);
			(*video)["v_id"] = videoId;
			(*video)["v_title"] = data[1];
			(*video)["v_info"] = data[2];
			(*video)["v_path"] = data[3];
			(*video)["v_cover"] = data[4];
			(*video)["v_updatetime"] = data[5];
			// (*video)["owner_id"] = data[5]; // owner_id 当前为空, 不用做转换

			mysql_free_result(result);

			return true;
		}

		// 3. 模糊查找
		// 根据标题进行模糊查找
		bool selectLikeVideo(const std::string& key, Json::Value* videos) {
			std::string sql;
			sql.resize(1024);
#define SELECTLIKE_VIDEO "SELECT * FROM tb_video WHERE v_title LIKE '%%%s%%';" // 注意转义
			sprintf(&sql[0], SELECTLIKE_VIDEO, key.c_str());
			std::cout << sql << std::endl;

			// 查询
			_mutex.lock();
			bool ret = mysqlQuery(_mysql, sql);
			if (!ret) {
				LOG(WARNING, "selectAllVideo():: mysqlQuery(%s) failed!", SELECTLIKE_VIDEO);
				_mutex.unlock(); // 这里要退出了, 要解锁
				return false;
			}

			// 获取结果集
			MYSQL_RES* result = mysql_store_result(_mysql);
			if (result == NULL) {
				LOG(WARNING, "selectAllVideo():: get query result failed!");
				_mutex.unlock();
				return false;
			}
			_mutex.unlock();
			// 下面就不在数据库操作了, 所以可以解锁了
			// 然后就可以将 查询到的结果记录到 Json::Value 中了

			// 获取结果行数
			int rowNum = mysql_num_rows(result);
			for (int i = 0; i < rowNum; i++) {
				MYSQL_ROW rowData = mysql_fetch_row(result); // 获取当前行数据

				// 每行数据 v_id v_title v_info v_path v_cover owner_id
				Json::Value video;
				video["v_id"] = atoi(rowData[0]); // 结果集中的数据都是以字符串形式存储的
				video["v_title"] = rowData[1];
				video["v_info"] = rowData[2];
				video["v_path"] = rowData[3];
				video["v_cover"] = rowData[4];
				video["v_updatetime"] = rowData[5];
				// video["owner_id"] = rowData[5]; // owner_id 当前为空 查不到
				videos->append(video);
			}
			mysql_free_result(result); // 释放结果集

			return true;
		}

	private:
		MYSQL* _mysql;
		std::mutex _mutex;
	};
} // namespace aod
