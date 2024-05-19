#include "MessageQueue.h"
/// @brief 析构
MessageQueue::~MessageQueue()
{
	stop();
}

/// @brief 是否为空
/// @return bool
bool MessageQueue::empty()
{
	std::lock_guard<std::mutex> lck(mtx_);
	return queue_.empty();
}

/// @brief 添加消息
/// @param str 消息
void MessageQueue::push(std::wstring& str)
{
	std::lock_guard<std::mutex> lck(mtx_);
	queue_.emplace(str);
	cv_lock_.notify_one();
}

/// @brief 重载
/// @param str 
void MessageQueue::push(std::wstring&& str)
{
	this->push(str);
}

/// @brief 重载
/// @param str 
void MessageQueue::push(std::string&& str)
{
	this->push(str);
}

/// @brief 重载
/// @param str 
void MessageQueue::push(std::string& str)
{
	QString qstr = QString::fromStdString(str);
	std::wstring wstr = qstr.toStdWString();
	this->push(wstr);
}

/// @brief 从消息队列中获取消息，如果消息队列为空，则会阻塞调用线程
/// @param mess 
void MessageQueue::get(std::wstring& mess)
{
	std::unique_lock<std::mutex> lck(mtx_);
	cv_lock_.wait(lck, [this]() {return stop_.load() || !queue_.empty(); });
	
	
	if (queue_.empty()) {
		mess = L"";
		return;
	}

	mess = queue_.front();
	queue_.pop();
}

/// @brief 构造函数
MessageQueue::MessageQueue() : stop_(false)
{
	
}

/// @brief 停止消息队列，唤醒所有阻塞进程并让其退出
void MessageQueue::stop()
{
	stop_.store(true);
	cv_lock_.notify_all();
}
