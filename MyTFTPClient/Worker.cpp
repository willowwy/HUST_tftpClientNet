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
			errorMessage += QString::fromLocal8Bit("����ʧ��\n");
			errorMessage = QString::fromLocal8Bit("������ϢΪ ") + QString(tftpClient->getErrorMessage());
			emit updateTextBrowser(errorMessage);
		}
		else
		{
			emit updateTextBrowser(QString::fromLocal8Bit("�ϴ��ɹ�"));
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
			errorMessage += QString::fromLocal8Bit("����ʧ��\n");
			errorMessage = QString::fromLocal8Bit("������ϢΪ ") + QString(tftpClient->getErrorMessage());
			emit updateTextBrowser(errorMessage);
		}
		else
		{
			QString message = QString::fromLocal8Bit("���سɹ�!\n");
			message += QString::fromLocal8Bit("�ļ��ı���·��Ϊ");
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