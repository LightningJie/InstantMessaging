#include "VarifyGrpcClient.h"
#include "ConfigMgr.h"
RPCConnPool::RPCConnPool(size_t poolsize, std::string host, std::string port):
poolSize_(poolsize),host_(host),port_(port),b_stop_(false)
{
	for (size_t i = 0; i < poolSize_; i++) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+port , grpc::InsecureChannelCredentials());
		connections_.push(VarifyService::NewStub(channel));
	}
}
RPCConnPool::~RPCConnPool()
{
	std::lock_guard<std::mutex> lock(mutex_);
	Close();
	while (!connections_.empty()) {
		connections_.pop();//std::queue::pop() 的行为：销毁队列头部的元素（调用其析构函数）移除该元素
	}
}
void RPCConnPool::Close()
{
	b_stop_ = true;
	cond_.notify_all();
}

std::unique_ptr<VarifyService::Stub> RPCConnPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_);//与条件变量配合必须用 unique_lock
	cond_.wait(lock, [this]() {
		if (b_stop_) {
			return true;
		}
		return !connections_.empty();
		});
	if (b_stop_) {
		return nullptr;
	}
	auto context=std::move(connections_.front());
	connections_.pop();
	return context;
}

void RPCConnPool::returnConnection(std::unique_ptr<VarifyService::Stub> context)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) {
		return;
	}
	connections_.push(std::move(context));
	cond_.notify_one();

}

VarifyGrpcClient::VarifyGrpcClient() {
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["VarifyServer"]["Host"];
	std::string port = gCfgMgr["VarifyServer"]["Port"];
	pool_.reset(new RPCConnPool(5, host, port));
}
GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
	ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);
	auto stub_ = pool_->getConnection();
	Status status = stub_->GetVarifyCode(&context, request, &reply);
	if (status.ok()) {
		pool_->returnConnection(std::move(stub_));
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		pool_->returnConnection(std::move(stub_));
		return reply;
	}
}