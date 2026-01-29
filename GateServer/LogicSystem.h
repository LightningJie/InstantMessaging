#pragma once
#include "const.h"
class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;//GetInstance 获得唯一实例时候要调用构造函数  但是LogicSystem类构造函数是私有的需要友元才能调用
public:
	~LogicSystem() = default;
	bool HandleGet(std::string path,std::shared_ptr<HttpConnection>con);
	bool HandlePost(std::string path, std::shared_ptr<HttpConnection>con);
	void RegGet(std::string url,HttpHandler handler);
	void RegPost(std::string url, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;

};

