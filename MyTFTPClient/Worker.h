#pragma once

#include "qthread.h"
#include "FileUpDown.h"

class Worker :public QThread
{
	Q_OBJECT
		QThread workThread;

public:
	void setTftpClient(TftpClient* client) { tftpClient = client; }
	TftpClient* getTftpClient() { return tftpClient; }

	void startWork() { this->start(); }
private:
	TftpClient* tftpClient;

protected:
	void run() override;

signals:
	void updateTextBrowser(QString);
};