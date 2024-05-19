#include "MessageQueue.h"

MessageQueue::~MessageQueue()
{
	stop();
}

bool MessageQueue::empty()
{
	std::lock_guard<std::mutex> lck(mtx_);
	return queue_.empty();
}

void MessageQueue::push(std::wstring& str)
{
	std::lock_guard<std::mutex> lck(mtx_);
	queue_.emplace(str);
	cv_lock_.notify_one();
}

void MessageQueue::push(std::wstring&& str)
{
	this->push(str);
}

void MessageQueue::push(std::string&& str)
{
	this->push(str);
}

void MessageQueue::push(std::string& str)
{
	QString qstr = QString::fromStdString(str);
	std::wstring wstr = qstr.toStdWString();
	this->push(wstr);
}

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

MessageQueue::MessageQueue() : stop_(false)
{
	
}

void MessageQueue::stop()
{
	stop_.store(true);
	cv_lock_.notify_all();
}
