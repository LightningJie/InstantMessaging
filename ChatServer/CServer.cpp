#include "CServer.h"
#include "AsioIOServicePool.h"
#include "UserMgr.h"
//构造一个 TCP 端点（endpoint = IP地址 + 端口），地址用 IPv4 的“任意地址”，端口用你给的 port
//0.0.0.0:port
CServer::CServer(boost::asio::io_context& io_context, short port):_io_context(io_context),
_port(port),_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
	std::cout << "Server start success, listen on port: " << _port << std::endl;
	StartAccept();
}

CServer::~CServer()
{
	std::cout << "Server destruct listen on port: " << _port << std::endl;
}

void CServer::ClearSession(std::string session_id)
{	
	if (_sessions.find(session_id) != _sessions.end()) {
		UserMgr::GetInstance()->RmvUserSession(_sessions[session_id]->GetUserId(),session_id);
	}
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.erase(session_id);
	}
	
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error)
{
	if (!error) {
		new_session->Start();//该函数的执行都是主线程执行，Start() 内部“发起的异步读写”会在哪个线程执行，取决于它用哪个 executor/io_context 来发起。
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.insert(make_pair(new_session->GetSessionId(), new_session));
	}
	else {
		std::cout << "session accept failed,error is" << error.what() << std::endl;
	}
	StartAccept();
}

void CServer::StartAccept()
{
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
	//acceptor 负责“新连接到来时，把连接绑定到 new_session->GetSocket() 这个 socket 上”
	//然后调用回调 HandleAccept(...)
	_acceptor.async_accept(new_session->GetSocket(), std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}
