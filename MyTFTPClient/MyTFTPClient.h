#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MyTFTPClient.h"
#include <WinSock2.h>
#include <iostream>  
#include <cstring>  
#include "FileUpDown.h"
#include "Worker.h"
#include "qwidget.h"


class MyTFTPClient : public QMainWindow
{
    Q_OBJECT

public:
    MyTFTPClient(QWidget* parent = Q_NULLPTR); /*{
        tftpClient = new TftpClient();
        worker = new Worker();
    };*/

    //用于检查ip地址字符串是否合法
    static bool isIPAddressValid(const char* pszIPAddr);

    void setIPLocation(QString ip) { ipLocation = ip; }
    QString getIPLocation() { return ipLocation; }

    void setclientIPLocation(QString ip) { clientIPLocation = ip; }
    QString getClientIPLocation() { return clientIPLocation; }

    void setUploadFilePath(QString str) { uploadFilePath = str; }
    QString getUploadFilePath() { return uploadFilePath; }

    void setDownloadFilePath(QString str) { downloadFilePath = str; }
    QString getDownloadFilePath() { return downloadFilePath; }

    void setSavePath(QString str) { savePath = str; }
    QString getSavePath() { return savePath; }

    void setTftpClient(TftpClient* client) { tftpClient = client; }
    TftpClient* getTftpClient() { return tftpClient; }

    void setTransferMode(QString mode) { transferMode = mode; }
    QString getTransferMode() { return transferMode; }

    void setWorker(Worker* er) { worker = er; }
    Worker* getWorker() { return worker; }
private:
    Ui::MyTFTPClientClass* ui;
    QString clientIPLocation;
    QString ipLocation;
    QString uploadFilePath;
    QString savePath;
    QString downloadFilePath;
    QString transferMode;
    TftpClient* tftpClient;
    Worker* worker;

private slots:
    void on_setIPButton_clicked();

    void on_selectFileButton_clicked();

    void on_downloadButton_clicked();

    void on_uploadButton_clicked();

    void on_netasciiCheckBox_stateChanged(int a);

    void on_octetCheckBox_stateChanged(int a);

    void on_updateTextBrowser(const char* message);

    void on_updateTextBrowser(QString message);

    void on_selectPathButton_clicked();

    void on_savePathButton_clicked();

    void on_updateAverageSpeed(QString);

    void on_clearButton_clicked();
};
