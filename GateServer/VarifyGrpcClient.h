#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;
class RPCConnPool {
public:
	RPCConnPool(size_t poolsize, std::string host, std::string port);
	~RPCConnPool();
	void Close();

	std::unique_ptr< VarifyService::Stub > getConnection();
	void returnConnection(std::unique_ptr<VarifyService::Stub> context);

private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<VarifyService::Stub>>connections_;//可以优化为并发编程里的头尾锁
	std::mutex mutex_;
	std::condition_variable cond_;
};
class VarifyGrpcClient:public Singleton<VarifyGrpcClient>
{
	friend class Singleton<VarifyGrpcClient>;
public:
	~VarifyGrpcClient() {}
	GetVarifyRsp GetVarifyCode(std::string email);
private:
	VarifyGrpcClient();
	std::unique_ptr<RPCConnPool>pool_;


};

