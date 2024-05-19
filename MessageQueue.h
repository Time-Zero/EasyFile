#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <codecvt>
#include <atomic>
#include <qstring.h>

/// @brief 一个简易的消息队列，不严谨的生产者消费者模型
class MessageQueue
{
public:
	MessageQueue(const MessageQueue&) = delete;
	MessageQueue& operator=(const MessageQueue&) = delete;
	MessageQueue(const MessageQueue&&) = delete;
	MessageQueue& operator=(const MessageQueue&&) = delete;
	
	~MessageQueue();

	static MessageQueue& GetInstance() {
		static MessageQueue ins;
		return ins;
	}
	
	bool empty();
	void push(std::wstring& str);
	void push(std::wstring&& str);
	void push(std::string&& str);
	void push(std::string& str);
	void get(std::wstring& mess);
private:
	MessageQueue();
	void stop();

private:
	std::queue<std::wstring> queue_;
	std::mutex mtx_;
	std::condition_variable cv_lock_;
	std::atomic_bool stop_;
};

