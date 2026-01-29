#pragma once
#include "const.h"
class HttpConnection :public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(boost::asio::io_context& ioc);
	void Start();
	tcp::socket& GetSocket() {
		return _socket;
	}
private:
	void CheckDeadLine();
	void WriteResponse();
	void HandleReq();
	void PreParseGetParam();
	tcp::socket _socket;
	beast::flat_buffer _buffer{8192};//8KB
	http::request<http::dynamic_body> _request;//存储解析后的完整 HTTP 请求，供 HandleReq() 处理
	http::response<http::dynamic_body> _response;//HandleReq() 构造好响应后，赋值给该变量，再由 WriteRespond() 异步发送给客户端
	net::steady_timer deadline_{
		_socket.get_executor(),std::chrono::seconds(60)
	};

	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

