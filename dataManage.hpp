#include "logMessage.hpp"
#include "util.hpp"
#include <mutex>
#include <mysql/mysql.h>

#define HOST "127.0.0.1"	 // 连接数据库地址
#define USER "root"			 // 连接数据库用户名
#define PASSWD "Dxyt200209." // 用户名密码
#define NAME "AoD_System_DB" // 数据库名

namespace aod {
	// mysqlInit 初始化操作
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
} // namespace aod
