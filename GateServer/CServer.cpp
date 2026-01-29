#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"
//tcp::acceptor 是 Boost.Asio 对操作系统「监听端口」功能的封装，是 TCP 服务器的「连接入口」。
//tcp::socket 是 Boost.Asio 对操作系统底层 TCP 套接字（Socket） 的封装，是 TCP 通信的「核心端点」 无拷贝构造
//io_context 是一个跨平台的「事件 + 任务」管理器：
//它底层封装了 Linux 的 epoll/Windows 的 IOCP，帮你监控 socket、定时器等 IO 事件；
//上层通过「事件循环」把「IO 事件的回调」和「普通任务」统一调度，分配给线程执行；
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc),
_acceptor(ioc,tcp::endpoint(tcp::v4(),port)) {

}

void CServer::Start()
{
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection>new_con = std::make_shared<HttpConnection>(io_context);//创建新的连接
	//handler 执行在：acceptor 所属的 io_context 也就是 accept 线程
	//new_con->GetSocket()是空的、未打开的 socket 引用对象”，Asio 会在 accept 成功时，把 OS 新建的 fd 填进去。
	_acceptor.async_accept(new_con->GetSocket(), [self,new_con](beast::error_code ec) {
		try {
			//出错放弃这个链接，继续监听其他链接
			if (ec) {
				self->Start();
				return;
			}
			
			new_con->Start();//makeshared 堆上创建对象  其中调用share from this 然后返回智能指针 

			//继续监听
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->Start();
		}
		});
}
