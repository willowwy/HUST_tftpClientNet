#include "MyTFTPClient.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyTFTPClient w;
    w.show();
    return a.exec();
}
