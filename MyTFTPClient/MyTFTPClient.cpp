#include "MyTFTPClient.h"
#include "qfiledialog.h"
#include "qwidget.h"
#include "qcoreapplication.h"
#pragma comment(lib, "ws2_32.lib") 


MyTFTPClient::MyTFTPClient(QWidget* parent)
	: QMainWindow(parent)
{
	ui = new Ui::MyTFTPClientClass();
	ui->setupUi(this);

	TftpClient* tftpClient = new TftpClient();
	Worker* worker = new Worker();
	this->setWorker(worker);
	this->setTftpClient(tftpClient);


	QString ipString = QString("tftp://10.12.172.172");
	QString clientIPString = QString("tftp://10.12.172.172");
	QString saveDirPath = QCoreApplication::applicationDirPath() + "/DownloadFile/";
	QString downloadFile = QString("test.txt");

	ui->serverIPLine->clear();
	ui->serverIPLine->setText(ipString);
	ui->clientIPLine->setText(clientIPString);
	ui->savePathLine->setText(saveDirPath);
	//ui->downloadPathLine->setText(QString::fromLocal8Bit("下载测试.docx"));
	ui->downloadPathLine->setText(downloadFile);
	ui->netasciiCheckBox->setChecked(false);
	ui->octetCheckBox->setChecked(true);



	this->setIPLocation(ipString.remove("tftp://"));
	this->setclientIPLocation(clientIPString.remove("tftp://"));
	this->setSavePath(saveDirPath);
	this->setDownloadFilePath(downloadFile);
	//tftpClient.setServerIP(ipString.remove("http://").toLatin1().data());
	worker->setTftpClient(tftpClient);


	connect(worker, SIGNAL(updateTextBrowser(QString)), this, SLOT(on_updateTextBrowser(QString)));
	connect(tftpClient, SIGNAL(updateTextBrowser(const char*)), this, SLOT(on_updateTextBrowser(const char*)));
	connect(tftpClient, SIGNAL(updateTextBrowser(QString)), this, SLOT(on_updateTextBrowser(QString)));
	connect(tftpClient, SIGNAL(updateAverageSpeed(QString)), this, SLOT(on_updateAverageSpeed(QString)));
	connect(tftpClient, SIGNAL(updateAverageSpeed(QString)), this, SLOT(on_clearButton_clicked()));
}


void MyTFTPClient::on_setIPButton_clicked()
{
	QString server = ui->serverIPLine->text();
	QString client = ui->clientIPLine->text();
	QString ip;
	if (server.contains("tftp://"))server.remove("tftp://");
	if (client.contains("tftp://"))client.remove("tftp://");

	QByteArray qb = server.toLatin1();
	if (true == MyTFTPClient::isIPAddressValid(qb.data()))
	{
		this->setIPLocation(server);
		server = "tftp://" + server;
	}
	else
	{
		server = QString::fromLocal8Bit("不合法ip地址字符串，请重新输入！");
	}

	qb = client.toLatin1();
	if (true == MyTFTPClient::isIPAddressValid(qb.data()))
	{
		this->setIPLocation(client);
		client = "tftp://" + client;
	}
	else
	{
		client = QString::fromLocal8Bit("不合法ip地址字符串，请重新输入！");
	}

	ui->serverIPLine->clear();
	ui->clientIPLine->clear();
	ui->serverIPLine->setText(server);
	ui->clientIPLine->setText(client);
}




void MyTFTPClient::on_selectFileButton_clicked()
{
	QString path;
	path = QFileDialog::getOpenFileName(this, "Please choose a file", ".");
	if (!path.isEmpty())
	{
		ui->uploadPathLine->clear();
		ui->uploadPathLine->setText(path);
		this->setUploadFilePath(path);
	}
	else
	{
		ui->uploadPathLine->clear();
		ui->uploadPathLine->setText(QString::fromLocal8Bit("未选择文件"));
	}
}

void MyTFTPClient::on_downloadButton_clicked()
{
	QString path = ui->downloadPathLine->text();
	if (!path.isEmpty())
	{
		ui->textBrowser->clear();
		this->setDownloadFilePath(path);

		TftpClient* client = this->getTftpClient();
		client->setRequestCode(RRQ);
		//client->setUploadFilePath(this->getUploadFilePath().toLatin1().data());
		client->setDownloadFileName(this->getDownloadFilePath().toLatin1().data());
		client->setSavePath(this->getSavePath().toLatin1().data());
		client->setServerIP(this->getIPLocation().toLatin1().data());
		client->setClientIP(this->getClientIPLocation().toLatin1().data());
		client->setTransferMode(this->getTransferMode().toLatin1().data());
		Worker* worker = this->getWorker();
		worker->startWork();
		//QString string = QString::fromLocal8Bit("下载文件保存路径为");
		//string += this->getSavePath() + path;
		//ui->textBrowser->append(string);
	}
}

void MyTFTPClient::on_uploadButton_clicked()
{
	ui->textBrowser->clear();
	TftpClient* client = this->getTftpClient();
	client->setRequestCode(WRQ);
	client->setUploadFilePath(this->getUploadFilePath().toLatin1().data());
	client->setServerIP(this->getIPLocation().toLatin1().data());
	client->setClientIP(this->getClientIPLocation().toLatin1().data());
	client->setTransferMode(this->getTransferMode().toLatin1().data());
	Worker* worker = this->getWorker();
	worker->startWork();

}

void MyTFTPClient::on_selectPathButton_clicked()
{
	QString path;
	QFileDialog* dialog = new QFileDialog(this);
	dialog->setFileMode(QFileDialog::Directory);
	//path = dialog->getOpenFileName(this, "Please choose a directory", ".",QFileDialog::Directory);
	path = QFileDialog::getExistingDirectory(this, "Please choose a directory", ".");
	if (!path.isEmpty())
	{
		ui->savePathLine->clear();
		ui->savePathLine->setText(path);
		this->setSavePath(path);
	}
}

void MyTFTPClient::on_savePathButton_clicked()
{
	QString fileName = ui->savePathLine->text();
	//QString fileName = "test.txt";
	if (!fileName.isEmpty())
	{
		this->setDownloadFilePath(fileName);

	}
}


/*
功能：
	判断IP地址是否有效
接口函数：
	booli isIPAddressValid ( const char *  pszIPAddr )
输入：
	pszIPAddr  字符串
输出：
	true 有效的IP地址，false，无效的IP地址
约束：
	1.输入IP为XXX.XXX.XXX.XXX格式
	2.字符串两端含有空格认为是合法IP
	3.字符串中间含有空格认为是不合法IP
	4.类似于 01.1.1.1， 1.02.3.4  IP子段以0开头为不合法IP
	5.子段为单个0 认为是合法IP，0.0.0.0也算合法I
*/
bool MyTFTPClient::isIPAddressValid(const char* pszIPAddr)
{
	if (!pszIPAddr) return false; //若pszIPAddr为空  
	char IP1[100], cIP[4];
	int len = strlen(pszIPAddr);
	int i = 0, j = len - 1;
	int k, m = 0, n = 0, num = 0;
	//去除首尾空格(取出从i-1到j+1之间的字符):  
	while (pszIPAddr[i++] == ' ');
	while (pszIPAddr[j--] == ' ');

	for (k = i - 1; k <= j + 1; k++)
	{
		IP1[m++] = *(pszIPAddr + k);
	}
	IP1[m] = '\0';

	char* p = IP1;

	while (*p != '\0')
	{
		if (*p == ' ' || *p < '0' || *p>'9') return false;
		cIP[n++] = *p; //保存每个子段的第一个字符，用于之后判断该子段是否为0开头  

		int sum = 0;  //sum为每一子段的数值，应在0到255之间  
		while (*p != '.' && *p != '\0')
		{
			if (*p == ' ' || *p < '0' || *p>'9') return false;
			sum = sum * 10 + *p - 48;  //每一子段字符串转化为整数  
			p++;
		}
		if (*p == '.') {
			if ((*(p - 1) >= '0' && *(p - 1) <= '9') && (*(p + 1) >= '0' && *(p + 1) <= '9'))//判断"."前后是否有数字，若无，则为无效IP，如“1.1.127.”  
				num++;  //记录“.”出现的次数，不能大于3  
			else
				return false;
		};
		if ((sum > 255) || (sum > 0 && cIP[0] == '0') || num > 3) return false;//若子段的值>255或为0开头的非0子段或“.”的数目>3，则为无效IP  

		if (*p != '\0') p++;
		n = 0;
	}
	if (num != 3) return false;
	return true;
}

void MyTFTPClient::on_netasciiCheckBox_stateChanged(int a)
{
	bool state = ui->netasciiCheckBox->isChecked();
	if (state == true)
	{
		ui->octetCheckBox->setChecked(false);
		this->setTransferMode("netascii");
	}
	else
	{
		ui->octetCheckBox->setChecked(true);
		this->setTransferMode("octet");
	}
}

void MyTFTPClient::on_octetCheckBox_stateChanged(int a)
{
	bool state = ui->octetCheckBox->isChecked();
	if (state == true)
	{
		ui->netasciiCheckBox->setChecked(false);
		this->setTransferMode("octet");
	}
	else
	{
		ui->netasciiCheckBox->setChecked(true);
		this->setTransferMode("netascii");
	}
}

void MyTFTPClient::on_updateTextBrowser(const char* message)
{
	ui->textBrowser->append(QString(message) + "\n");
}

void MyTFTPClient::on_updateTextBrowser(QString message)
{
	ui->textBrowser->append(message);
}


void MyTFTPClient::on_updateAverageSpeed(QString message)
{
	ui->averageSpeedPathLine->clear();
	ui->averageSpeedPathLine->setText(message);
}

void MyTFTPClient::on_clearButton_clicked()
{
	ui->textBrowser->clear();
}