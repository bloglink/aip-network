#ifndef CNETWORK_H
#define CNETWORK_H

#include <QDir>
#include <QFile>
#include <QTimer>
#include <QObject>
#include <QProcess>
#include <QSettings>
#include <QHostInfo>
#include <QTcpSocket>
#include <QDataStream>
#include <QNetworkInterface>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDateTime>

#define TCP_CMD_CLIENT_LOGIN        1000
#define TCP_CMD_CLIENT_LOGIN_ERROR  1001
#define TCP_CMD_SERVER_GET_HEAD     1002
#define TCP_CMD_CLIENT_GET_HEAD     1003
#define TCP_CMD_SOCKET_DISPLAY      1004
#define TCP_CMD_CLIENT_DROPPED      1005
#define TCP_CMD_LOCAL_LOGIN         1006
#define TCP_CMD_LOGIN_ERROR         1007
#define TCP_CMD_LOGIN_SUCCESS       1008
#define TCP_CMD_HEAD_DATA           2000
#define TCP_CMD_HEAD_ERROR          2009
#define TCP_CMD_FILE_DATA           2001
#define TCP_CMD_FILE_ERROR          2003
#define TCP_CMD_FILE_SUCCESS        2002
#define TCP_CMD_SHEEL_CMD           2004
#define TCP_CMD_SHEEL_RETURN        2005
#define TCP_CMD_LIST_GET            2006
#define TCP_CMD_LIST_ERROR          2007
#define TCP_CMD_HEART               6000

#define ADDR 6000
#define WIN_CMD_SWITCH  7000

#define CAN_DAT_GET 8000
#define CAN_DAT_PUT 8001


#define NET "./network/"
#define TMP "./temp/"
#define CON "./config/"
#ifdef __arm__
#define GLOBAL_SET "/mnt/nandflash/AIP/Sys.ini"
#else
#define GLOBAL_SET "settings/global.ini"
#endif

class CTcpNetwork : public QTcpSocket
{
    Q_OBJECT
public:
    explicit CTcpNetwork(QObject *parent = 0);

signals:
    void TransformCmd(quint16 addr,quint16 proc,QByteArray data);
public slots:
    void Connect(void);
private slots:
    void Disconnect(void);
    void Connected(void);
    void KeepAlive(void);
    void GetBlock(void);
    void GetFileHead(quint16 addr,QByteArray msg);
    void GetFileData(quint16 addr,QByteArray msg);
    void PutFileData(qint64 numBytes);
    void PutFileHead(QByteArray data);
    void PutBlock(quint16 addr, quint16 cmd, QByteArray data);
    void ExcuteCmd(quint16 addr,quint16 proc,QByteArray data);
    QString HardwareAddress(void);
    void Error(QAbstractSocket::SocketError);
    void Display(QByteArray msg);
private:
    QTimer *timer;
    QProcess *proc;
    QSettings *set;
    QString InitString;
    QString HostName;
    QString Version;
    QString User;
    QString Number;

    QFile *file;
    QString fileName;
    QString fileDetail;

    quint16 TramsmitAddr;

    qint64 count;
    qint64 LoadSize;
    qint64 BlockSize;
    qint64 BytesRead;
    qint64 BytesToRead;
    qint64 BytesToWrite;
    qint64 BytesWritten;
    QByteArray SendVerify;
    QByteArray RecvVerify;
};

#endif // CNETWORK_H
