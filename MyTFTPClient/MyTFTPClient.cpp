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
	//ui->downloadPathLine->setText(QString::fromLocal8Bit("���ز���.docx"));
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
		server = QString::fromLocal8Bit("���Ϸ�ip��ַ�ַ��������������룡");
	}

	qb = client.toLatin1();
	if (true == MyTFTPClient::isIPAddressValid(qb.data()))
	{
		this->setIPLocation(client);
		client = "tftp://" + client;
	}
	else
	{
		client = QString::fromLocal8Bit("���Ϸ�ip��ַ�ַ��������������룡");
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
		ui->uploadPathLine->setText(QString::fromLocal8Bit("δѡ���ļ�"));
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
		//QString string = QString::fromLocal8Bit("�����ļ�����·��Ϊ");
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
���ܣ�
	�ж�IP��ַ�Ƿ���Ч
�ӿں�����
	booli isIPAddressValid ( const char *  pszIPAddr )
���룺
	pszIPAddr  �ַ���
�����
	true ��Ч��IP��ַ��false����Ч��IP��ַ
Լ����
	1.����IPΪXXX.XXX.XXX.XXX��ʽ
	2.�ַ������˺��пո���Ϊ�ǺϷ�IP
	3.�ַ����м京�пո���Ϊ�ǲ��Ϸ�IP
	4.������ 01.1.1.1�� 1.02.3.4  IP�Ӷ���0��ͷΪ���Ϸ�IP
	5.�Ӷ�Ϊ����0 ��Ϊ�ǺϷ�IP��0.0.0.0Ҳ��Ϸ�I
*/
bool MyTFTPClient::isIPAddressValid(const char* pszIPAddr)
{
	if (!pszIPAddr) return false; //��pszIPAddrΪ��  
	char IP1[100], cIP[4];
	int len = strlen(pszIPAddr);
	int i = 0, j = len - 1;
	int k, m = 0, n = 0, num = 0;
	//ȥ����β�ո�(ȡ����i-1��j+1֮����ַ�):  
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
		cIP[n++] = *p; //����ÿ���Ӷεĵ�һ���ַ�������֮���жϸ��Ӷ��Ƿ�Ϊ0��ͷ  

		int sum = 0;  //sumΪÿһ�Ӷε���ֵ��Ӧ��0��255֮��  
		while (*p != '.' && *p != '\0')
		{
			if (*p == ' ' || *p < '0' || *p>'9') return false;
			sum = sum * 10 + *p - 48;  //ÿһ�Ӷ��ַ���ת��Ϊ����  
			p++;
		}
		if (*p == '.') {
			if ((*(p - 1) >= '0' && *(p - 1) <= '9') && (*(p + 1) >= '0' && *(p + 1) <= '9'))//�ж�"."ǰ���Ƿ������֣����ޣ���Ϊ��ЧIP���硰1.1.127.��  
				num++;  //��¼��.�����ֵĴ��������ܴ���3  
			else
				return false;
		};
		if ((sum > 255) || (sum > 0 && cIP[0] == '0') || num > 3) return false;//���Ӷε�ֵ>255��Ϊ0��ͷ�ķ�0�Ӷλ�.������Ŀ>3����Ϊ��ЧIP  

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