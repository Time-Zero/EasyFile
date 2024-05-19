#pragma once
#include <string>
#include <qstring.h>
#include <iostream>
#include <filesystem>
#include <sys/stat.h>
#include <cwchar>
#include <qcryptographichash.h>
#include <qfile.h>
#include "MessageQueue.h"

#define EDEBUG(s) std::cout << "Tcp Service : " << s << std::endl;
#define ESDEBUG(s,n) std::cout << "Tcp Service : " << s \
								<< " , error code : " << n << std::endl; 


std::wstring getFileInfo(QString filePath);
unsigned long long getFileSize(QString filePath);
std::wstring getFileName(QString filePath);
std::pair<std::wstring, long long> fileInfoTrans(const wchar_t* wbuf);
std::wstring getHashValue(QString filePath);