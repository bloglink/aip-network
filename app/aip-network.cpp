#include <QCoreApplication>
#include "CTcpNetwork.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CTcpNetwork Network;

    Network.Connect();

    return a.exec();
}

