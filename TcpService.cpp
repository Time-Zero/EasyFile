#include "TcpService.h"

/// @brief 构造函数
TcpService::TcpService() :
	stop_(true),
	file_count_(0),
	stop_bar_(false),
	totalFileSize_(0),
	hasSentBytes_(0)
{
	EDEBUG("Tcp Srevice init");
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != NO_ERROR) {
		ESDEBUG("Wsa Service start up failed", ret);
	}
	
	fileDst = "";
	send_progress_update();
}

/// @brief 析构函数
TcpService::~TcpService()
{
	EDEBUG("Tcp Service destruct");
	accept_stop();
	stop_progressbar();
	WSACleanup();
}

/// @brief 启动accept监听
/// @param port 监听的端口
void TcpService::accept_start(int port)
{
	if (!stop_.load())
		return;
	
	EDEBUG("Accept Service Start");
	MessageQueue::GetInstance().push("监听已经启动");
	int ret = 0;
	SOCKET sock = INVALID_SOCKET;
	struct addrinfo* result = nullptr;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	std::string s_port = std::to_string(port);
	EDEBUG("accept port is " + s_port);

	// pNodeName == null， socket将监听发送到本机所有ip的请求
	ret = getaddrinfo(NULL, s_port.c_str(), &hints, &result);
	if (ret != 0) {
		ESDEBUG("getaddrinfo failed", ret);
		freeaddrinfo(result);
		return;
	}

	sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET) {
		ESDEBUG("socket create failed,", WSAGetLastError());
		freeaddrinfo(result);
		return;
	}

	ret = bind(sock, result->ai_addr, (size_t)result->ai_addrlen);
	if (ret == SOCKET_ERROR) {
		ESDEBUG("socket bind failed ", WSAGetLastError());
		closesocket(sock);
		freeaddrinfo(result);
		return;
	}
	freeaddrinfo(result);

	ret = listen(sock, SOMAXCONN);
	if (ret == SOCKET_ERROR) {
		ESDEBUG("socket listen failed", WSAGetLastError());
		closesocket(sock);
		return;
	}

	// 端口立即复用
	bool bReuseAddr = true;
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuseAddr, bReuseAddr);
	if (ret == SOCKET_ERROR) {
		ESDEBUG("port reuse set failed", WSAGetLastError());
		closesocket(sock);
		return;
	}

	u_long mode = 1;
	ret = ioctlsocket(sock, FIONBIO, &mode);
	if (ret == SOCKET_ERROR) {
		ESDEBUG("ioctlsocket failed", WSAGetLastError());
		closesocket(sock);
		return;
	}

	EDEBUG("socket accept start");
	stop_.store(false);
	while (!stop_.load()) {
		auto clientSock = accept(sock, NULL, NULL);
		if (clientSock == INVALID_SOCKET) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				ESDEBUG("socket accept failed", WSAGetLastError());
				closesocket(sock);
				stop_.store(true);
				return;
			}

			continue;
		}

		EDEBUG("accept a new connection");
		std::thread t(&TcpService::recv_file, this, std::move(clientSock));
		t.detach();
	}

	stop_.store(true);

	EDEBUG("accept service stop success")
	MessageQueue::GetInstance().push(L"关闭监听成功");

}

/// @brief 关闭accpet监听
void TcpService::accept_stop()
{
	EDEBUG("try to stop accept service")

	if (stop_.load())
		return;

	MessageQueue::GetInstance().push(L"尝试关闭监听监听");
	stop_.store(true);
}


/// @brief 接受处理
/// @param sock 从哪个socket中读取文件
void TcpService::recv_file(SOCKET sock)
{
	EDEBUG("recv file submode start");

	int ret = 0;
	char buf[DATABUFLEN];
	memset(buf, 0, DATABUFLEN);
	
	fd_set readset;
	FD_ZERO(&readset);
	FD_SET(sock, &readset);
	timeval tv = { 0,600 };

	ret = select(0, &readset, NULL, NULL, &tv);
	if (ret == SOCKET_ERROR) {
		ESDEBUG("recv file socket error", WSAGetLastError());
		closesocket(sock);
		return;
	}
		
	ret = recv(sock, buf, sizeof(buf), 0);
	if (ret == SOCKET_ERROR) {
		ESDEBUG("recv failed", WSAGetLastError());
		closesocket(sock);
		return;
	}

	const wchar_t* wbuf = reinterpret_cast<const wchar_t*>(buf);
	auto fileVec = fileInfoTrans(wbuf);
	std::wstring fileName = fileDst.toStdWString() + L"\\" + fileVec[0];
	unsigned long long fileSize = std::stoull(fileVec[1]);
	EDEBUG(fileSize);
	MessageQueue::GetInstance().push(L"收到文件:" + fileVec[0]);
	std::wstring recvMd5 = fileVec[2];			//这是文件的md5值

	//const wchar_t* wbuf = reinterpret_cast<const wchar_t*>(buf);
	//auto filePair = fileInfoTrans(wbuf);
	//std::wstring fileName = fileDst.toStdWString() + L"\\" + filePair.first;
	//unsigned long long fileSize = filePair.second;
	//EDEBUG(fileSize);
	//MessageQueue::GetInstance().push(L"收到文件:" + filePair.first);


	CREATEFILE2_EXTENDED_PARAMETERS params;
	params.dwSize = sizeof(params);
	params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	params.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	params.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	params.lpSecurityAttributes = NULL;
	params.hTemplateFile = NULL;
	HANDLE hFile = CreateFile2(
		fileName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		CREATE_ALWAYS,
		&params);
	if (hFile == INVALID_HANDLE_VALUE) {
		ESDEBUG("CreateFile failed", GetLastError());
		closesocket(sock);
		return;
	}

	LARGE_INTEGER liFileSize;
	liFileSize.QuadPart = fileSize;
	SetFilePointerEx(hFile, liFileSize, NULL, FILE_BEGIN);
	SetEndOfFile(hFile);

	HANDLE mFile = CreateFileMappingA(
		hFile,
		NULL,
		PAGE_READWRITE,
		0,
		0,
		NULL
	);
	if (mFile == NULL) {
		ESDEBUG("CreateFileMapping failed", GetLastError());
		closesocket(sock);
		CloseHandle(hFile);
		return;
	}

	LPVOID lpFile = MapViewOfFile(
		mFile,
		FILE_MAP_WRITE,
		0,
		0,
		0);
	if (lpFile == NULL) {
		ESDEBUG("MapViewOfFile failed", GetLastError());
		closesocket(sock);
		CloseHandle(mFile);
		CloseHandle(hFile);
		return;
	}


	ULONGLONG totalByteWrite = 0;
	do {
		memset(buf, 0, sizeof(buf));
		ret = recv(sock, buf, sizeof(buf), 0);
		if (ret == 0) {
			break;
		}
		else if (ret > 0) {
			ULONGLONG byteToWrite = std::min(fileSize - totalByteWrite, (ULONGLONG)DATABUFLEN);
			memcpy((char*)lpFile + totalByteWrite, buf, byteToWrite);
			totalByteWrite += byteToWrite;
		}
	} while (1);

	UnmapViewOfFile(lpFile);
	CloseHandle(mFile);
	CloseHandle(hFile);

	closesocket(sock);

	std::wstring localMd5 = getHashValue(QString::fromStdWString(fileName));
	if (recvMd5 != localMd5) {
		MessageQueue::GetInstance().push(L"文件校验失败:" + fileVec[0]);
		if (DeleteFile(fileName.c_str())) {
			MessageQueue::GetInstance().push(L"文件已经被删除");
		}
		else {
			MessageQueue::GetInstance().push(L"文件删除失败，请手动删除");
		}
	}

	EDEBUG("end file recv");

	//std::fstream outFile;
	//outFile.open(fileName, std::ios::out | std::ios::app);

	////TODO: 补充数据接收部分
	//
	//ULONGLONG totalByteWrite = 0;
	//do {
	//	ret = recv(sock, buf, DATABUFLEN, 0);
	//	
	//	if (ret == 0) {
	//		EDEBUG("connect has been closed");
	//		break;
	//	}
	//	else if(ret > 0) {
	//		//EDEBUG("recv data");
	//		ULONGLONG byteToWrite = std::min(fileSize - totalByteWrite, (ULONGLONG)DATABUFLEN);
	//		outFile.write(buf, byteToWrite);
	//		totalByteWrite += byteToWrite;
	//	}
	//} while (1);

	//
	//outFile.close();
	//closesocket(sock);
	//EDEBUG("recv file end");
}

/// @brief 连接到server端
/// @param ip sever端ip
/// @param port server端端口
/// @param filePath 待发送的文件
void TcpService::connect_server(std::string ip, int port, QString filePath)
{
	EDEBUG("try to connect to server");

	/*ULONGLONG temp_fileSize = getFileSize(filePath);
	if (temp_fileSize < 0) {
		MessageQueue::GetInstance().push(filePath.toStdWString() + L"文件存在问题，无法发送");
		return;
	}*/

	std::wstring fileInfo = getFileInfo(filePath);
	if (!fileInfo.size())
		return;

	int ret = 0;
	SOCKET sock = INVALID_SOCKET;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		ESDEBUG("create socket failed", WSAGetLastError());
		return;
	}

	sockaddr_in sockService;
	sockService.sin_family = AF_INET;
	inet_pton(AF_INET, ip.c_str(), &sockService.sin_addr.S_un.S_addr);
	sockService.sin_port = htons(port);

	ret = ::connect(sock, (SOCKADDR*)&sockService, sizeof(sockService));
	if (ret == SOCKET_ERROR) {
		ESDEBUG("connect to server failed", WSAGetLastError());
		closesocket(sock);
	}

	EDEBUG("connect to server success");

	send_file(sock, filePath, fileInfo);

	shutdown(sock, SD_SEND);	
	closesocket(sock);
	EDEBUG("end socket task, close it");
}

/// @brief 发送文件
/// @param sock 发送使用的socket
/// @param filePath 发送文件的路径
/// @param fileInfo 发送文件信息
void TcpService::send_file(SOCKET sock, QString filePath, std::wstring fileInfo)
{

	int ret = 0;
	char buf[DATABUFLEN];
	memset(buf, 0, DATABUFLEN);

	//TODO: 补充内存映射部分
	HANDLE hFile = CreateFile(filePath.toStdWString().c_str(),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		ESDEBUG("CreateFile failed", GetLastError());
		return;
	}

	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(hFile, &liFileSize)) {
		ESDEBUG("GetFileSizeEx failed", GetLastError());
		CloseHandle(hFile);
		return;
	}

	HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, liFileSize.HighPart, liFileSize.LowPart, NULL);
	if (hFileMapping == NULL) {
		ESDEBUG("CreateFileMapping failed", GetLastError());
		CloseHandle(hFile);
		return;
	}

	LPVOID lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (lpFileBase == NULL) {
		ESDEBUG("MapViewOfFile failed", GetLastError());
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return;
	}


	int mag = sizeof(wchar_t) / sizeof(char);
	memcpy(buf, fileInfo.c_str(), fileInfo.size() * mag);

	ret = send(sock, buf, DATABUFLEN, 0);
	if (ret == SOCKET_ERROR) {
		ESDEBUG("send file failed", WSAGetLastError())
		UnmapViewOfFile(lpFileBase);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return;
	}

	totalFileSize_ += getFileSize(filePath);
	++file_count_;
	ULONGLONG totalBytesSent = 0;
	while (totalBytesSent < liFileSize.QuadPart) {
		memset(buf, 0, sizeof(buf));
		ULONGLONG bytesToSend = std::min(liFileSize.QuadPart - totalBytesSent, (ULONGLONG)sizeof(buf));
		hasSentBytes_ += bytesToSend;
		memcpy(buf, (char*)lpFileBase + totalBytesSent, bytesToSend);
		int bytesSent = send(sock, buf, sizeof(buf), 0);
		if (bytesSent == SOCKET_ERROR) {
			ESDEBUG("send failed", WSAGetLastError());
			break;
		}
		totalBytesSent += bytesSent;
	}

	UnmapViewOfFile(lpFileBase);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);

	EDEBUG("end send file");
	--file_count_;
}

/// @brief 设置文件保存位置
/// @param val 文件保存位置
void TcpService::set_file_dst(const QString& val)
{
	this->fileDst = val;
}

void TcpService::send_progress_update()
{
	//TODO:完善文件进度条更新
	std::thread t([this]() {
		while (!stop_bar_.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (!file_count_.load()) {
				reset_processbar();
				emit progressBarValueUpdate(0);
				continue;
			}

			int rate = hasSentBytes_ * 100 / totalFileSize_;
			EDEBUG(rate);
			emit progressBarValueUpdate(rate);
		}
		});

	t.detach();
}

/// @brief 停止进度条线程循环，并且重置文件统计字节
void TcpService::stop_progressbar()
{
	stop_bar_.store(true);
	reset_processbar();
}

/// @brief 重置发送进度条
void TcpService::reset_processbar()
{
	hasSentBytes_.store(0);
	totalFileSize_.store(0);
}
