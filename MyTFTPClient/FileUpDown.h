#pragma once
#include <WinSock2.h>
#include <cstdlib>
#include <cstringt.h>
#include <qstring.h>
#include "qobject.h"
#include "qdatetime.h"
#pragma comment(lib, "ws2_32.lib") 

#define RRQ 1
#define WRQ 2
#define RRQ_OPERATION 1
#define WRQ_OPERATION 2


class TftpClient :public QObject
{
	Q_OBJECT
		;
	//TftpClient workClient;
public:
	void setUploadFilePath(char* str) { strcpy_s(uploadFilePath, sizeof(char) * MAX_PATH, str); }
	const char* getUploadFilePath() { return uploadFilePath; }
	void setDownloadFileName(char* str) { strcpy_s(downloadFileName, sizeof(char) * MAX_PATH, str); }
	const char* getDownloadFileName() { return downloadFileName; }
	void setSavePath(char* str) { strcpy_s(savePath, sizeof(char) * MAX_PATH, str); }
	const char* getSavePath() { return savePath; }
	void setTransferMode(char* str) { strcpy_s(transferMode, sizeof(char) * 10, str); }
	const char* getTransferMode() { return transferMode; }
	void setErrorMessage(char* message) { strcpy_s(errorMessage, 1000, message); }
	void appendErrorMessage(const char* message) { strcat_s(errorMessage, 1000 - strlen(errorMessage), message); }
	void clearErrorMessage() { memset(errorMessage, 0, sizeof(errorMessage)); }
	const char* getErrorMessage() { return errorMessage; }
	void setServerIP(char* ip) { strcpy_s(serverIP, sizeof(char) * 20, ip); }
	const char* getServerIP() { return serverIP; }
	void setClientIP(char* ip) { strcpy_s(clientIP, sizeof(char) * 20, ip); }
	const char* getClientIP() { return clientIP; }
	void setRequestCode(int code) { requestCode = code; }
	int getRequestCode() { return requestCode; }
	void setLogFilePointer(FILE* file) { this->logFile = file; }
	FILE* getLogFilePointer() { return this->logFile; }


	int generateRQMessage(char** dst, char* fileName, char* TransferMode, unsigned short operationCode);

	static char* getFileNameFromPath(char* path);
	static void byteHandle(unsigned short* dst);
	static unsigned short tempByteHandle(unsigned short* dst);
	static void setSockaddr_in(sockaddr_in* in, short sin_family, unsigned short ports, long ip);
	bool uploadFile();
	bool downloadFile();
	void sendStartDownloadMessage();
	void sendDownloadPacketNumberMessage(unsigned short blockNumber);
	void sendStartUploadMessage();
	void sendUploadPacketNumberMessage(unsigned short blockNumber);
	int logWrite(FILE* file, char* message);
	int logWrite(FILE* file, QString message);
	int logWriteHead(FILE* file);
	int logWriteTail(FILE* file);

	QString sockaddrToQString(QString headText, sockaddr_in sockaddr);
private:
	char uploadFilePath[MAX_PATH + 1];
	char downloadFileName[MAX_PATH];
	char savePath[MAX_PATH + 1];
	char transferMode[10];
	char clientIP[20];
	char serverIP[20];
	char errorMessage[1000];
	int requestCode;
	FILE* logFile;

signals:
	void updateTextBrowser(const char* message);
	void updateTextBrowser(QString message);
	void updateAverageSpeed(QString);
};
