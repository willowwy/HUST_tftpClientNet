#pragma once
#include "Worker.h"

void Worker::run()
{
	TftpClient* tftpClient = this->getTftpClient();
	tftpClient->clearErrorMessage();
	QString errorMessage;
	bool result = true;

	if (tftpClient->getRequestCode() == WRQ)
	{
		FILE* logFile = fopen("log/upload.log", "a");
		tftpClient->setLogFilePointer(logFile);
		result = tftpClient->uploadFile();
		if (result == false)
		{
			errorMessage += QString::fromLocal8Bit("传输失败\n");
			errorMessage = QString::fromLocal8Bit("错误信息为 ") + QString(tftpClient->getErrorMessage());
			emit updateTextBrowser(errorMessage);
		}
		else
		{
			emit updateTextBrowser(QString::fromLocal8Bit("上传成功"));
		}
		if (logFile != NULL)
		{
			fclose(logFile);
		}
	}
	else if (tftpClient->getRequestCode() == RRQ)
	{
		FILE* logFile = fopen("log/download.log", "a");
		tftpClient->setLogFilePointer(logFile);
		result = tftpClient->downloadFile();
		if (result == false)
		{
			errorMessage += QString::fromLocal8Bit("传输失败\n");
			errorMessage = QString::fromLocal8Bit("错误信息为 ") + QString(tftpClient->getErrorMessage());
			emit updateTextBrowser(errorMessage);
		}
		else
		{
			QString message = QString::fromLocal8Bit("下载成功!\n");
			message += QString::fromLocal8Bit("文件的保存路径为");
			message += QString(tftpClient->getSavePath());
			message += QString(tftpClient->getDownloadFileName());
			emit updateTextBrowser(message);
		}
		tftpClient->logWriteTail(logFile);
		if (logFile != NULL)
		{
			fclose(logFile);
		}
	}

}