#include "CommonTools.h"

/// @brief 获取文件路径文件名和文件字节数
/// @param filePath 文件路径
/// @return fileName:fileSize形式的组合
std::wstring getFileInfo(QString filePath)
{
	
	int ret = 0;
	unsigned long long fileSize = 0;
	std::wstring fileName = L"";
	std::wstring md5Value = L"";

	fileSize = getFileSize(filePath);
	if (fileSize < 0) {
		return L"";
	}

	fileName = getFileName(filePath);
	if (!fileName.size())
		return L"";

	md5Value = getHashValue(filePath);			// 校验文件hash
	if (!md5Value.size())
		return L"";

	std::wstring wsfileSize = std::to_wstring(fileSize);
	return fileName + L":" + wsfileSize + L":" + md5Value;
}

/// @brief 获取文件字节数，支持4G以上文件
/// @param filePath 文件路径
/// @return unsigned long long 类型文件字节数，若获取失败，则返回-1
unsigned long long getFileSize(QString filePath)
{
	unsigned long long fileSize = 0;
	try {
		fileSize = std::filesystem::file_size(filePath.toStdWString());
	}
	catch (std::filesystem::filesystem_error& e) {
		ESDEBUG("std::filesystem::file_size failed", e.what());
		return -1;
	}

	return fileSize;
}

/// @brief 返回无路径文件名
/// @param filePath 文件路径
/// @return 无路径文件名，如果获取失败，返回空字节
std::wstring getFileName(QString filePath)
{
	std::wstring fileName = L"";
	std::filesystem::path file_path(filePath.toStdWString());
	if (file_path.has_filename()) {
		fileName = file_path.filename();
	}

	return fileName;
}

/// @brief 对fileName:fileSize结构进行解析，提取文件名和字节数
/// @param wbuf 含有fileName:fileSize的buf
/// @return pair.first = fileName; pair.second = fileSize
std::pair<std::wstring, long long> fileInfoTrans(const wchar_t* wbuf)
{
	std::pair <std::wstring, long long> result;
	std::wstring wstr(wbuf);
	
	
	size_t pos = wstr.find(L":");
	if (pos == std::wstring::npos) {
		EDEBUG("file info trans failed");
		return result;
	}

	result.first = std::wstring(wstr.substr(0, pos));
	
	long long temp = std::stoll(wstr.substr(pos + 1, wstr.size() - 1));
	result.second = temp;
}

/// @brief 计算文件sha256值，用于文件完整性校验
/// @param filePath 文件路径
/// @return wstring保存的sha256校验值 
std::wstring getHashValue(QString filePath)
{
	QFile file(filePath);
	std::wstring res = L"";
	if (!file.open(QIODevice::ReadOnly)) {
		EDEBUG("getHashValue failed");
		return res;
	}

	QCryptographicHash hash(QCryptographicHash::Md5);

	while (!file.atEnd()) {
		QByteArray data = file.read(1024);
		hash.addData(data);
	}

	QByteArray hashValue = hash.result();
	QString qsData(hashValue);
	res = qsData.toStdWString();
	return res;
}
