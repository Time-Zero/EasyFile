#pragma once
#include <atomic>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <thread>
#include <qstring.h>
#include <sys/stat.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <qobject.h>
#include "CommonTools.h"
#include "MessageQueue.h"


#define DATABUFLEN 1024 

class TcpService : public QObject
{
	Q_OBJECT;

public:
	TcpService();
	~TcpService();

	void accept_start(int port);
	void accept_stop();
	void recv_file(SOCKET sock);

	void connect_server(std::string ip, int port, QString filePath);;
	//void send_file(SOCKET sock, QString filePath);
	void send_file(SOCKET sock, QString filePath, std::wstring fileInfo);


	void set_file_dst(const QString& val);

signals:
	void progressBarValueUpdate(int);

private:
	void send_progress_update();
	void stop_progressbar();
	void reset_processbar();

private:
	std::atomic_ullong totalFileSize_;			//总待发送字节数
	std::atomic_ullong hasSentBytes_;			//已经发送的字节数
	std::atomic_bool stop_;						//监听停止
	std::atomic_bool stop_bar_;					//进度条刷新线程停止
	std::atomic_int file_count_;				//待发送文件计数
	QString fileDst;							//文件保存地址
};