#pragma once
#include "FileUpDown.h"
#include <WinSock2.h>
//#pragma comment(lib, "ws2_32.lib")

#define NET_ASCII 1
#define OCTET 2

bool TftpClient::uploadFile()
{
	FILE* logFile = this->getLogFilePointer();
	logWriteHead(logFile);

	//初始化Winsock
	WSADATA wsaData;
	int flag = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (flag == 0)
		logWrite(logFile, "WSA init Successfully\n");
	else {
		if (flag == WSASYSNOTREADY)
			logWrite(logFile, "The underlying network subsystem is not ready\n");
		else if (flag == WSAVERNOTSUPPORTED)
			logWrite(logFile, "Winsock version information number is not supported\n");
		else if (flag == WSAEPROCLIM)
			logWrite(logFile, "Winsock usage limit reached\n");
		else if (flag == WSAEFAULT)
			logWrite(logFile, "lpWSAData is not a valid pointer\n");
		else
			logWrite(logFile, "WSA failed to initialize for some reason\n");
		return false;
	}

	logWrite(logFile, "WSA init Successfully\n");

	unsigned long long startTime;
	unsigned long long endTime;
	SYSTEMTIME startSysTime;
	SYSTEMTIME sysTime;
	//创建并配置socket
	SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	logWrite(logFile, QString("Create clientSocket ") + QString().sprintf("%08x AF_INET SOCK_DGRAM IPPROTO_UDP\n", clientSocket));
	//创建并配置sockaddr_in clientSockaddr 端口为0表示随机分配 ip地址为TftpClient类中已指定的clientIP
	sockaddr_in clientSockaddr;
	TftpClient::setSockaddr_in(&clientSockaddr, PF_INET, htons(0), inet_addr(this->getClientIP()));

	logWrite(logFile, sockaddrToQString(QString("Create clientSockaddr "), clientSockaddr));

	//创建并配置sockaddr_in serverSockaddr 端口为69 ip地址为TftpClient类中已指定的serverIP
	sockaddr_in serverSockaddr;
	TftpClient::setSockaddr_in(&serverSockaddr, PF_INET, htons(69), inet_addr(this->getServerIP()));
	logWrite(logFile, sockaddrToQString(QString("Create serverSockaddr "), serverSockaddr));

	//创建源sockaddr 用于在recvfrom中获取server为本次连接提供的端口
	sockaddr_in sourceSockaddr;
	memset(&sourceSockaddr, 0, sizeof(sockaddr_in));
	int sourceSockaddr_len = sizeof(sourceSockaddr);

	//配置超时机制 超时时间为0.05秒
	int waittime = 50;
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)(&waittime), sizeof(waittime));

	//尝试与server建立连接
	flag = bind(clientSocket, (sockaddr*)&clientSockaddr, sizeof(clientSockaddr));
	if (flag != SOCKET_ERROR)
	{
		char* wrqMessage;
		char* fileName = TftpClient::getFileNameFromPath((char*)this->getUploadFilePath());
		char* transferMode = (char*)this->getTransferMode();
		int modeForTransfer;
		//配置传输模式 若不为指定的两种模式 退出执行 传出errormessage
		if (strcmp("octet", transferMode) == 0)modeForTransfer = OCTET;
		else if (strcmp("netascii", transferMode) == 0)modeForTransfer = NET_ASCII;
		else
		{
			this->appendErrorMessage("TransferMode ERROR!\n");
			logWrite(logFile, QString("Request has wrong TransferMode,Transfer End\n"));
			logWriteTail(logFile);
			return false;
		}

		logWrite(logFile, QString("Set TransferMode ") + QString().sprintf("%s\n", transferMode));

		//获取请求报文 length为报文的长度 传入报文字符串指针 需要上传的文件名 传输模式 以及报文中的operationCode
		int length = generateRQMessage(&wrqMessage, fileName, transferMode, WRQ_OPERATION);
		//发出报文
		sendto(clientSocket, wrqMessage, length, 0, (sockaddr*)&serverSockaddr, sizeof(serverSockaddr));

		logWrite(logFile, QString("Request has been sent\n"));

		//用于保存ack报文的字符数组
		char ackPacket[600];
		//指向ack报文头部的操作数
		unsigned short* ackOpCode = (unsigned short*)ackPacket;
		//指向ack报文的数据块号
		unsigned short* ackBlockNumber = (unsigned short*)(ackPacket + 2);
		*ackOpCode = 0;
		memset(ackPacket, 0, sizeof(char) * 600);
		//若ackOpCode为0表示未收到响应报文 10秒仍未收到则退出执行 传出请求无响应的errorMessage
		for (int i = 0; i < 50 && *ackOpCode == 0; i++)
		{
			int back = recvfrom(clientSocket, ackPacket, 600 * sizeof(char), 0, (struct sockaddr*)&sourceSockaddr, (int*)&sourceSockaddr_len);
			int error = WSAGetLastError();
			TftpClient::byteHandle((unsigned short*)ackPacket);
			TftpClient::byteHandle((unsigned short*)(ackPacket + 2));
			if (*ackOpCode == 0 && (i + 1) % 10 == 0)
			{
				//重发请求报文
				sendto(clientSocket, wrqMessage, length, 0, (sockaddr*)&serverSockaddr, sizeof(serverSockaddr));
				logWrite(logFile, QString("Request has been sent again\n"));
			}
			Sleep(20);
		}
		if (*ackOpCode == 0)
		{
			this->appendErrorMessage("request no ack ERROR!\n");
			logWrite(logFile, QString("Request has no answer,Transfer End\n"));
			logWriteTail(logFile);
			return false;
		}
		//到这里说明实际上收到了返回报文 故检查是否ackopcode为5 表示响应报文为ERROR报文
		else if (*ackOpCode == 5)
		{
			this->appendErrorMessage(ackPacket + 4);
			this->appendErrorMessage("\n");
			logWrite(logFile, QString("Request answer is error with error message:") + QString().sprintf("%s\n", ackPacket + 4));
			logWrite(logFile, QString("Transfer End\n"));
			logWriteTail(logFile);
			return false;
		}
		//emit updateTextBrowser(QString("Starting upload ") + QString(fileName));
		sendStartUploadMessage();
		GetSystemTime(&startSysTime);

		startTime = (unsigned long long)startSysTime.wMilliseconds + startSysTime.wSecond * 1000ull + startSysTime.wMinute * 60 * 1000ull + startSysTime.wHour * 60 * 60 * 1000ull;

		//执行到此说明已得到请求的响应
		//这里是从请求响应报文中获取server为此次连接提供的端口号
		//将其填入之前使用的clientSockaddr即可继续之后的传输
		unsigned short newPort = ((struct sockaddr_in*)(&sourceSockaddr))->sin_port;
		TftpClient::byteHandle(&newPort);
		serverSockaddr.sin_port = ntohs(newPort);
		logWrite(logFile, sockaddrToQString(QString("Update serverSockaddr "), serverSockaddr));
		logWrite(logFile, "Upload start\n");

		//如果响应报文中ackOpCode为4 blockNumber为0表示连接建立成功
		if (4 == *ackOpCode && 0 == *ackBlockNumber)
		{
			FILE* file;
			//按照传输模式来确定打开需要上传的文件的方式
			if (modeForTransfer == OCTET) file = fopen(this->getUploadFilePath(), "rb");
			else file = fopen(this->getUploadFilePath(), "r");
			//检查文件是否成功打开 若不成功 传出打开文件失败的errormessage
			if (file != NULL)
			{
				logWrite(logFile, QString("File open successfully. File Path:") + QString().sprintf("%s\n", this->getUploadFilePath()));
				//用于保存data的字符数组 2字节的opcode 2字节的blocknumber 最大512字节data
				//冗余4字节 降低越界风险
				char message[520];
				memset(message, 0, 520 * sizeof(char));
				unsigned short* messageOpCode = (unsigned short*)message;
				unsigned short* messageBlockNumber = (unsigned short*)(message + 2);
				unsigned short us_blockNumber = 0;
				int readLength = 0;
				int flag = 1;

				readLength = fread(message + 4, sizeof(char), 512, file);
				logWrite(logFile, QString("Read File successfully\n"));

				//每读取512字节的data就执行传输
				while (1 == flag)
				{
					if (readLength < 512)flag = 0;
					*ackOpCode = 0;
					*messageOpCode = 3;

					//循环blocknumber，使得可以传输大于32Mb的包
					if (*messageBlockNumber == 0xffff) *messageBlockNumber = 0x0;
					else *messageBlockNumber += 1;
					TftpClient::byteHandle(messageOpCode);
					TftpClient::byteHandle(messageBlockNumber);

					//发送data报文，告诉服务器即将发送的data
					int back = sendto(clientSocket, message, 4 + readLength, 0, (struct sockaddr*)&serverSockaddr, sizeof(serverSockaddr));
					int error = WSAGetLastError();
					//每0.5秒一次的无限次接收ack返回报文
					for (int i = 0; *ackOpCode == 0; i++)
					{
						recvfrom(clientSocket, ackPacket, 600, 0, (sockaddr*)&sourceSockaddr, &sourceSockaddr_len);
						TftpClient::byteHandle((unsigned short*)ackPacket);
						TftpClient::byteHandle((unsigned short*)(ackPacket + 2));

						//未接收到正确的ack，传输超时,设置延迟
						unsigned short TmpBlockNumber = TftpClient::tempByteHandle(messageBlockNumber);
						if (TmpBlockNumber != *ackBlockNumber)
							*ackOpCode = 0;
						//重新发送数据包;
						if (*ackOpCode == 0 && i % 10 == 0)
							sendto(clientSocket, message, 4 + readLength, 0, (struct sockaddr*)&serverSockaddr, sizeof(serverSockaddr));
					}
					TftpClient::byteHandle(messageBlockNumber);
					//如果ackopcode为5 说明server返回的报文为ERROR报文
					if (*ackOpCode == 5)
					{
						this->appendErrorMessage(ackPacket + 4);
						this->appendErrorMessage("\n");
						logWrite(logFile, QString("Request answer is error with error message:") + QString().sprintf("%s\n", ackPacket + 4));
						logWrite(logFile, QString("Transfer End\n"));
						logWriteTail(logFile);
						return false;
					}
					//如果ackopcode为4 但blocknumber与发送的不同 说明发生了未指定的错误
					//则结束传输 传出block number错误的errormessage
					else if (*ackOpCode == 4 && (*ackBlockNumber != *messageBlockNumber))
					{
						this->appendErrorMessage("ack block number ERROR!\n");
						logWrite(logFile, QString("Transfer is error with wrong ack number\n"));
						logWrite(logFile, QString("Transfer End\n"));
						logWriteTail(logFile);
						return false;
					}
					//如果收到的ackopcode既不是5也不是4 说明发生了未指定的错误
					//则结束传输 传出ack opcode错误的errormessage
					else if (*ackOpCode != 4)
					{
						this->appendErrorMessage("ack opcode ERROR!\n");
						logWrite(logFile, QString("Transfer is error with wrong opcode\n"));
						logWrite(logFile, QString("Transfer End\n"));
						logWriteTail(logFile);
						return false;
					}
					//到这里说明一切正常 继续循环 发送数据
					//若已经读完则不再继续读
					if (flag == 1) readLength = fread(message + 4, sizeof(char), 512, file);
				}

				fclose(file);
				GetSystemTime(&sysTime);
				endTime = (unsigned long long)sysTime.wMilliseconds + sysTime.wSecond * 1000ull + sysTime.wMinute * 60 * 1000ull + sysTime.wHour * 60 * 60 * 1000ull;
				unsigned long long byteNumber = (unsigned long long)(*messageBlockNumber - 1) * 512 + readLength;

				double averageSpeed;
				//传输过快，为防止divide 0用此近似操作
				if (endTime == startTime) averageSpeed = byteNumber;
				//正常计算传输时间
				else averageSpeed = byteNumber / (endTime - startTime);
				averageSpeed *= 1000;
				averageSpeed /= 1024;
				QString speedString = QString().sprintf("%.2lf KB/s", averageSpeed);
				logWrite(logFile, QString("Average transfer speed is ") + QString().sprintf("%.2lf KB/s\n", averageSpeed));
				logWriteTail(logFile);
				emit updateAverageSpeed(speedString);
			}
			else
			{
				//文件路径错误或文件不存在
				this->appendErrorMessage("fileOpen ERROR!\n");
				logWrite(logFile, QString("File open failed. File Path:") + QString().sprintf("%s\n", this->getUploadFilePath()));
				logWriteTail(logFile);
				return false;
			}
		}
		//释放数据包
		free(wrqMessage);
	}
	else
	{
		//ip地址绑定错误
		this->appendErrorMessage("bind ERROR!\n");
		return false;
	}
	closesocket(clientSocket);
	WSACleanup();
	return true;
}

bool TftpClient::downloadFile()
{
	WSADATA wsaData;
	int soo = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (soo != 0) {
		logWrite(logFile, "WSA init Failed\n");
		return false;
	}

	logWrite(logFile, "WSA init Successfully\n");
	unsigned long long startTime;
	unsigned long long endTime;
	SYSTEMTIME startSysTime;
	SYSTEMTIME sysTime;

	//创建并配置socket
	SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//创建并配置sockaddr_in clientSockaddr 端口为0表示随机分配 ip地址为TftpClient类中已指定的clientIP
	sockaddr_in clientSockaddr;
	TftpClient::setSockaddr_in(&clientSockaddr, PF_INET, htons(0), inet_addr(this->getClientIP()));
	logWrite(logFile, sockaddrToQString(QString("Create clientSockaddr "), clientSockaddr));

	//创建并配置sockaddr_in serverSockaddr 端口为69 ip地址为TftpClient类中已指定的serverIP
	sockaddr_in serverSockaddr;
	TftpClient::setSockaddr_in(&serverSockaddr, PF_INET, htons(69), inet_addr(this->getServerIP()));
	logWrite(logFile, sockaddrToQString(QString("Create serverSockaddr "), serverSockaddr));

	//创建源sockaddr 用于在recvfrom中获取server为本次连接提供的端口
	sockaddr_in sourceSockaddr;
	memset(&sourceSockaddr, 0, sizeof(sockaddr_in));
	int sourceSockaddr_len = sizeof(sourceSockaddr);

	//配置超时机制 超时时间为0.1秒
	int waittime = 50;
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)(&waittime), sizeof(waittime));

	//尝试与server建立连接
	if (SOCKET_ERROR != bind(clientSocket, (sockaddr*)&clientSockaddr, sizeof(clientSockaddr)))
	{
		char* rrqMessage;
		char* fileName = (char*)this->getDownloadFileName();
		char* transferMode = (char*)this->getTransferMode();
		int modeForTransfer;
		//配置传输模式 若不为指定的两种模式 退出执行 传出errormessage
		if (strcmp("octet", transferMode) == 0)modeForTransfer = OCTET;
		else if (strcmp("netascii", transferMode) == 0)modeForTransfer = NET_ASCII;
		else
		{
			this->appendErrorMessage("TransferMode ERROR!\n");
			logWrite(logFile, QString("Request has wrong TransferMode,Transfer End\n"));
			return false;
		}

		logWrite(logFile, QString("Set TransferMode ") + QString().sprintf("%s\n", transferMode));

		//获取请求报文 length为报文的长度 传入报文字符串指针 需要下载的文件名 传输模式 以及报文中的operationCode
		int length = generateRQMessage(&rrqMessage, fileName, transferMode, RRQ_OPERATION);
		//发出报文
		sendto(clientSocket, rrqMessage, length, 0, (sockaddr*)&serverSockaddr, sizeof(serverSockaddr));

		logWrite(logFile, QString("Request has been sent\n"));

		//用于保存ack报文的字符数组
		//在这个函数中实际用于保存来自server的data
		char ack[600];
		unsigned short* ackOpCode = (unsigned short*)ack;
		unsigned short* ackBlockNumber = (unsigned short*)(ack + 2);
		*ackOpCode = 0;
		memset(ack, 0, sizeof(char) * 600);
		//若ackOpCode为0表示未收到响应报文 10秒仍未收到则退出执行 传出请求无响应的errorMessage

		//用于保存收到的报文的长度
		int dataLength = 0;
		for (int i = 0; i < 50 && *ackOpCode == 0; i++)
		{
			dataLength = recvfrom(clientSocket, ack, 600 * sizeof(char), 0, (struct sockaddr*)&sourceSockaddr, (int*)&sourceSockaddr_len);
			//recv(clientSocket, ack, 600 * sizeof(char), 0);
			int error = WSAGetLastError();
			TftpClient::byteHandle((unsigned short*)ack);
			TftpClient::byteHandle((unsigned short*)(ack + 2));
			if (*ackOpCode == 0 && (i + 1) % 10 == 0)
			{
				//重发请求报文
				sendto(clientSocket, rrqMessage, length, 0, (sockaddr*)&serverSockaddr, sizeof(serverSockaddr));
				logWrite(logFile, QString("Request has been sent again\n"));
			}
			Sleep(20);
		}
		if (*ackOpCode == 0)
		{
			this->appendErrorMessage("request no ack ERROR!\n");
			logWrite(logFile, QString("Request has no answer,Transfer End\n"));
			return false;
		}
		//到这里说明实际上收到了返回报文 故检查是否ackopcode为5 表示响应报文为ERROR报文
		else if (*ackOpCode == 5)
		{
			this->appendErrorMessage(ack + 4);
			this->appendErrorMessage("\n");
			logWrite(logFile, QString("Request answer is error with error message:") + QString().sprintf("%s\n", ack + 4));
			logWrite(logFile, QString("Transfer End\n"));
			return false;
		}

		//emit updateTextBrowser(QString("Starting download ") + QString(fileName));
		sendStartDownloadMessage();


		GetSystemTime(&startSysTime);
		startTime = (unsigned long long)startSysTime.wMilliseconds + startSysTime.wSecond * 1000ull + startSysTime.wMinute * 60 * 1000ull + startSysTime.wHour * 60 * 60 * 1000ull;
		//执行到此说明已得到请求的响应
		//这里是从请求响应报文中获取server为此次连接提供的端口号
		//将其填入之前使用的clientSockaddr即可继续之后的传输
		unsigned short newPort = ((struct sockaddr_in*)(&sourceSockaddr))->sin_port;
		TftpClient::byteHandle(&newPort);
		serverSockaddr.sin_port = ntohs(newPort);
		logWrite(logFile, sockaddrToQString(QString("Update serverSockaddr "), serverSockaddr));
		logWrite(logFile, "Download start\n");
		//如果响应报文中ackOpCode为4 blockNumber为1表示连接建立成功 并且已经收到了data1
		if (3 == *ackOpCode && 1 == *ackBlockNumber && 0 != (dataLength - 4))
		{
			FILE* file;
			char path[MAX_PATH * 2];
			memset(path, 0, sizeof(char) * (MAX_PATH * 2));
			strcat(path, this->getSavePath());
			strcat(path, this->getDownloadFileName());

			//按照传输模式来确定打开需要下载的文件的方式
			if (modeForTransfer == OCTET)file = fopen(path, "wb");
			else file = fopen(path, "w");
			//检查文件是否成功打开 若不成功 传出打开文件失败的errormessage
			if (file != NULL)
			{
				logWrite(logFile, QString("File open successfully. File Path:") + QString(path) + QString("\n"));
				//用于保存ack的字符数组 2字节的opcode 2字节的blocknumber
				//冗余6字节 降低越界风险
				char message[10];
				memset(message, 0, 6 * sizeof(char));
				unsigned short* messageOpCode = (unsigned short*)message;
				unsigned short* messageBlockNumber = (unsigned short*)(message + 2);
				//*messageBlockNumber = 1;
				int writeLength = 0;
				int flag = 1;
				//writeLength = fread(message + 4, sizeof(char), 512, file);
				writeLength = fwrite(ack + 4, sizeof(char), dataLength - 4, file);

				logWrite(logFile, QString("Write File successfully\n"));

				//每读取512字节的data就执行传输
				while (1 == flag)
				{
					if (writeLength < 512)flag = 0;
					*messageOpCode = 4;
					if (*messageBlockNumber == 0xfffe) 
						*messageBlockNumber = 1;
					else *messageBlockNumber = *messageBlockNumber + 1;
					TftpClient::byteHandle(messageOpCode);
					TftpClient::byteHandle(messageBlockNumber);
					//发送ack报文
					int back = sendto(clientSocket, message, 4, 0, (struct sockaddr*)&serverSockaddr, sizeof(serverSockaddr));
					if (flag == 1)
					{
						int error = WSAGetLastError();
						*ackOpCode = 0;
						//每0.5秒一次的无限次重新发送
						for (int i = 0; *ackOpCode == 0; i++)
						{
							dataLength = recvfrom(clientSocket, ack, 600, 0, (sockaddr*)&sourceSockaddr, &sourceSockaddr_len);
							TftpClient::byteHandle((unsigned short*)ack);
							TftpClient::byteHandle((unsigned short*)(ack + 2));
							if (*ackBlockNumber <= TftpClient::tempByteHandle(messageBlockNumber))
							{
								*ackOpCode = 0;
							}
							if (*ackOpCode == 0 && i % 10 == 0)sendto(clientSocket, message, 4, 0, (struct sockaddr*)&serverSockaddr, sizeof(serverSockaddr));
						}
						//恢复messageBlockNumber的字节序
						TftpClient::byteHandle(messageBlockNumber);
						//如果ackopcode为5 说明server返回的报文为ERROR报文 
						if (*ackOpCode == 5)
						{
							this->appendErrorMessage(ack + 4);
							this->appendErrorMessage("\n");
							logWrite(logFile, QString("Request answer is error with error message:") + QString().sprintf("%s\n", ack + 4));
							logWrite(logFile, QString("Transfer End\n"));
							return false;
						}
						//如果ackopcode为3 但blocknumber与发送的不同 说明发生了未指定的错误
						//则结束传输 传出blocknumber错误的errormessage
						else if (*ackOpCode == 3 && *ackBlockNumber != (*messageBlockNumber + 1))
						{
							this->appendErrorMessage("ack block number ERROR!\n");
							logWrite(logFile, QString("Transfer is error with wrong ack number\n"));
							logWrite(logFile, QString("Transfer End\n"));
							return false;
						}
						//如果收到的ackopcode既不是5也不是3 说明发生了未指定的错误
						//则结束传输 传出ack opcode错误的errormessage
						else if (*ackOpCode != 3)
						{
							this->appendErrorMessage("ack opcode ERROR!\n");
							logWrite(logFile, QString("Transfer is error with wrong opcode\n"));
							logWrite(logFile, QString("Transfer End\n"));
							return false;
						}
						//到这里说明一切正常 继续循环 写入数据
						//logWrite(logFile, QString().sprintf("Get packet %u successfully.sendLength:%d\n", *messageBlockNumber, writeLength));
						writeLength = fwrite(ack + 4, sizeof(char), dataLength - 4, file);
						//emit sendDownloadPacketNumberMessage(*ackBlockNumber);
						//logWrite(logFile, QString("Write File successfully. writelength:") + QString().sprintf("%d\n", writeLength));
					}
				}
				logWrite(logFile, QString().sprintf("Last Packet Get successfully\n"));

				GetSystemTime(&sysTime);
				endTime = (unsigned long long)sysTime.wMilliseconds + sysTime.wSecond * 1000ull + sysTime.wMinute * 60 * 1000ull + sysTime.wHour * 60 * 60 * 1000ull;
				unsigned long long byteNumber = TftpClient::tempByteHandle(messageBlockNumber);
				byteNumber = ((unsigned long long)(byteNumber - 1) * 512 + writeLength);
				double averageSpeed = byteNumber / (endTime - startTime);
				averageSpeed *= 1000;
				averageSpeed /= 1024;
				QString speedString = QString().sprintf("%.2lf KB/s", averageSpeed);
				emit updateAverageSpeed(speedString);
				logWrite(logFile, QString("Average transfer speed is ") + QString().sprintf("%.2lf KB/s\n", averageSpeed));
				//logWriteTail(logFile);
				fclose(file);
			}
			else
			{
				this->appendErrorMessage("fileOpen ERROR!\n");
				logWrite(logFile, QString("File open failed. File Path:") + QString().sprintf("%s\n", this->getUploadFilePath()));
				return false;
			}
		}
		free(rrqMessage);
	}
	else
	{
		this->appendErrorMessage("bind ERROR!\n");
		return false;
	}

	closesocket(clientSocket);
	WSACleanup();
	return true;
}

int TftpClient::generateRQMessage(char** dst, char* fileName, char* TransferMode, unsigned short operationCode)
{
	int length = 4 + strlen(fileName) + strlen(TransferMode);
	char* result = (char*)malloc(sizeof(char) * (length + 5));
	if (result != NULL)
	{
		(*dst) = result;
		length += 5;
		memset(result, 0, sizeof(char) * length);

		unsigned short* opCode = (unsigned short*)(result);
		*opCode = operationCode;
		result += 2;
		strcat(result, fileName);
		result += (strlen(fileName) + 1);
		strcat(result, TransferMode);

		TftpClient::byteHandle((unsigned short*)(*dst));

		return length - 5;
	}
	return -1;
}

char* TftpClient::getFileNameFromPath(char* path)
{
	while (*path != '\0')path++;
	while (*path != '/')path--;
	return path + 1;
}

void TftpClient::byteHandle(unsigned short* dst)
{
	char* result = (char*)dst;
	char a = *result;
	char b = *(result + 1);
	*result = b;
	*(result + 1) = a;
}

unsigned short TftpClient::tempByteHandle(unsigned short* dst)
{
	unsigned short temp = *dst;
	temp = ((0xff & temp) << 8) | ((0xff00 & temp) >> 8);
	return temp;
}

inline void TftpClient::setSockaddr_in(sockaddr_in* in, short sin_family, unsigned short ports, long ip)
{
	in->sin_family = sin_family;
	in->sin_port = ports;
	in->sin_addr.s_addr = ip;
}

inline void TftpClient::sendStartDownloadMessage()
{
	QString message = QString::fromLocal8Bit("成功与服务器建立连接，开始下载");
	message += QString(this->getDownloadFileName());
	emit updateTextBrowser(message);
}

inline void TftpClient::sendDownloadPacketNumberMessage(unsigned short blockNumber)
{
	QString message = QString::fromLocal8Bit("下载完成packet ");
	message += QString::number(blockNumber);
	emit updateTextBrowser(message);
}

inline void TftpClient::sendStartUploadMessage()
{
	QString message = QString::fromLocal8Bit("成功与服务器建立连接，开始上传");
	message += QString(TftpClient::getFileNameFromPath((char*)this->getUploadFilePath()));
	emit updateTextBrowser(message);
}

inline void TftpClient::sendUploadPacketNumberMessage(unsigned short blockNumber)
{
	QString message = QString::fromLocal8Bit("上传完成packet ");
	message += QString::number(blockNumber);
	emit updateTextBrowser(message);
}

inline int TftpClient::logWrite(FILE* file, char* message)
{
	if (file != NULL)
	{
		QString s = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") + QString("  ") + QString(message);
		QByteArray qb = s.toLatin1();
		char* h = qb.data();
		fwrite(h, sizeof(char), strlen(h), file);
		return 1;
	}
	return 0;
}

inline int TftpClient::logWrite(FILE* file, QString message)
{
	if (file != NULL)
	{
		message = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz") + QString("  ") + message;
		QByteArray qb = message.toLatin1();
		char* h = qb.data();
		fwrite(h, sizeof(char), strlen(h), file);
		return 1;
	}
	return 0;
}

inline int TftpClient::logWriteHead(FILE* file)
{
	if (file != NULL)
	{
		const char* message = "\n ----------------------------------- \n";
		fwrite(message, sizeof(char), strlen(message), file);
		return 1;
	}
	return 0;
}

inline int TftpClient::logWriteTail(FILE* file)
{
	if (file != NULL)
	{
		const char* message = " ----------------------------------- \n\n";
		fwrite(message, sizeof(char), strlen(message), file);
		return 1;
	}
	return 0;
}

inline QString TftpClient::sockaddrToQString(QString headText, sockaddr_in sockaddr)
{
	QString message;
	message += headText;
	message += QString("ip:") + QString(inet_ntoa(sockaddr.sin_addr));
	message += QString("  port:") + QString().sprintf("%u\n", ntohs(sockaddr.sin_port));
	return message;
}