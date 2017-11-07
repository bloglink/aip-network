/*******************************************************************************
 * Copyright (c) 2016,青岛艾普智能仪器有限公司
 * All rights reserved.
 *
 * version:     0.1
 * author:      link
 * date:        2016.12.17
 * brief:       二代客户端网络后台
*******************************************************************************/
#include "CTcpNetwork.h"
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      初始化
******************************************************************************/
CTcpNetwork::CTcpNetwork(QObject *parent) : QTcpSocket(parent)
{
    file         = NULL;
    count        = 0;
    LoadSize     = 4*1024;;
    BlockSize    = 0;
    BytesRead    = 0;
    BytesToRead  = 0;
    BytesToWrite = 0;
    BytesWritten = 0;

    QDir *dir = new QDir;
    if (!dir->exists(QString(TMP)))
        dir->mkdir(QString(TMP));
    if (!dir->exists(QString(NET)))
        dir->mkdir(QString(NET));

    proc = new QProcess(this);
    set = new QSettings(GLOBAL_SET,QSettings::IniFormat);
    set->setIniCodec("GB18030");

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(KeepAlive()));
    connect(this,SIGNAL(connected()),this,SLOT(Connected()));
    connect(this,SIGNAL(readyRead()),this,SLOT(GetBlock()));
    connect(this,SIGNAL(bytesWritten(qint64)),this,SLOT(PutFileData(qint64)));
    connect(this,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(Error(QAbstractSocket::SocketError)));
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      网络初始化
******************************************************************************/
void CTcpNetwork::Connect()
{
    QFileInfo fi("net");
    QString tt;
    if (fi.isFile())
        tt = QString("AIP-%1").arg(fi.created().toString("yyyyMMdd"));
    else
        tt = QString("AIP-00000001");
    count = 0;
    HostName = set->value("/GLOBAL/TCP_HOST","s.aipuo.com").toString();
    Version = set->value("/Version/text","V0.1").toString();
    User = set->value("/GLOBAL/TCP_USER","sa@@1234").toString();
    Number = set->value("/Factory/text",tt).toString();

    this->connectToHost(HostName, 6000);
    timer->start(10000);
    Display("连接服务器...\n");
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      网络退出
******************************************************************************/
void CTcpNetwork::Disconnect()
{
    this->close();
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      连接成功
******************************************************************************/
void CTcpNetwork::Connected()
{
#ifdef __arm__
    ExcuteCmd(quint16(ADDR),quint16(TCP_CMD_CLIENT_LOGIN),NULL);
#else
    ExcuteCmd(quint16(ADDR),quint16(TCP_CMD_LOCAL_LOGIN),NULL);
#endif
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      心跳
******************************************************************************/
void CTcpNetwork::KeepAlive()
{
    if (this->state() != QAbstractSocket::ConnectedState) {
        Connect();
    } else {
        PutBlock(ADDR,TCP_CMD_HEART,"NULL");
        count++;
        if (count >6) {
            Display("服务器无反应，断开连接\n");
            this->abort();
        }
    }
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      读取数据
******************************************************************************/
void CTcpNetwork::GetBlock()
{
    quint16 addr;
    quint16 cmd;
    QByteArray msg;
    QDataStream in(this);
    in.setVersion(QDataStream::Qt_4_8);
    while (1) {
        if (BlockSize == 0) {
            if (this->bytesAvailable() < qint64(sizeof(qint64)))
                return;
            in >> BlockSize;
        }
        if (BlockSize > 8*1024) {
            qDebug() << "block size overflow";
            this->abort();
            return;
        }
        if (this->bytesAvailable() < BlockSize)
            return;
        in >> addr >> cmd >> msg;
        ExcuteCmd(addr,cmd,msg);
        BlockSize = 0;
    }
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      读取文件头
******************************************************************************/
void CTcpNetwork::GetFileHead(quint16 addr,QByteArray msg)
{
    SendVerify = msg.mid(0,16);
    QString temp = msg.remove(0,17);
    QStringList detail = temp.split(" ");
    BytesToRead = detail.at(0).toInt();
    BytesRead = 0;
    file = new QFile(QString(TMP).append(detail.at(1)));
    if(!file->open(QFile::ReadWrite)) {
        PutBlock(addr,TCP_CMD_HEAD_ERROR,file->errorString().toUtf8());
    }
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      读取文件
******************************************************************************/
void CTcpNetwork::GetFileData(quint16 addr,QByteArray msg)
{
    int ret;
    if (file == NULL)
        return;
    if (file->isOpen())
        ret = file->write(msg);
    else
        return;
    BytesRead += ret;
    if (BytesRead == BytesToRead) {
        file->seek(0);
        RecvVerify = QCryptographicHash::hash(file->readAll(),QCryptographicHash::Md5);
        file->close();
        BytesRead = 0;
        BytesToRead = 0;
        if (SendVerify == RecvVerify) {
            QProcess::execute("sync");
            QString cmd = QString("mv %1 %2").arg(file->fileName()).arg(NET);
            QProcess::execute(cmd);
            PutBlock(addr,TCP_CMD_FILE_SUCCESS,"NULL");
            file = NULL;
            qDebug() << "Put Data Success";
        } else {
            QString cmd = QString("rm %1").arg(file->fileName());
            QProcess::execute(cmd);
            PutBlock(addr,TCP_CMD_FILE_ERROR,"NULL");
            qDebug() << "Put Data Error";
            file = NULL;
        }
        return;
    }
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      写数据
******************************************************************************/
void CTcpNetwork::PutBlock(quint16 addr, quint16 cmd, QByteArray data)
{
    QByteArray msg;
    QDataStream out(&msg, QIODevice::ReadWrite);
    out.setVersion(QDataStream::Qt_4_8);
    out<<(qint64)0 << quint16(addr)<<quint16(cmd)<<data;
    out.device()->seek(0);
    out<<(qint64)(msg.size()-sizeof(qint64));
    this->write(msg);
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      发送文件头
******************************************************************************/
void CTcpNetwork::PutFileHead(QByteArray data)
{
    QString name = data;
    file = new QFile(name);
    if (!file->open(QFile::ReadOnly)) {
        PutBlock(ADDR,TCP_CMD_HEAD_ERROR,file->errorString().toUtf8());
        qDebug() << "open file error!" << file->errorString();
        return;
    }
    QByteArray msg;
    msg.append(QCryptographicHash::hash(file->readAll(),QCryptographicHash::Md5));
    file->seek(0);
    msg.append(QString(" %1 ").arg(file->size()));
#ifdef __linux__
    msg.append(name.right(name.size()- name.lastIndexOf('/')-1));
#else
    msg.append(name.right(name.size()- name.lastIndexOf('\\')-1));
#endif
    PutBlock(ADDR,TCP_CMD_HEAD_DATA,msg);
    BytesToWrite = file->size();
    BytesWritten = 0;
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      发送文件
******************************************************************************/
void CTcpNetwork::PutFileData(qint64 numBytes)
{
    if (BytesToWrite == 0)
        return;
    BytesWritten += (int)numBytes;
    PutBlock(quint16(ADDR),quint16(TCP_CMD_FILE_DATA),file->read(LoadSize));
    BytesToWrite -= (int)qMin(BytesToWrite,LoadSize);
    if (BytesToWrite == 0)
        file->close();
    count = 0;
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      命令执行
******************************************************************************/
void CTcpNetwork::ExcuteCmd(quint16 addr, quint16 cmd, QByteArray data)
{
    timer->stop();
    count = 0;
    switch (cmd) {
    case TCP_CMD_CLIENT_LOGIN:
        InitString.clear();
        InitString.append(Number);
        InitString.append(" ");
        InitString.append(HardwareAddress());
        InitString.append(" ");
        InitString.append(Version);
        PutBlock(ADDR,TCP_CMD_CLIENT_LOGIN,InitString.toUtf8());
        Display("连接服务器成功\n");
        break;
    case TCP_CMD_CLIENT_LOGIN_ERROR:
        Display(data);
        break;
    case TCP_CMD_LOCAL_LOGIN:
        InitString.clear();
        InitString.append(User);
        InitString.append(" ");
        InitString.append(HardwareAddress());
        InitString.append(" ");
        InitString.append(Version);
        qDebug()<<InitString;
        PutBlock(ADDR,TCP_CMD_LOCAL_LOGIN,InitString.toUtf8());
        Display("连接服务器成功\n");
        Display("验证用户名和密码...\n");
        break;
    case TCP_CMD_LOGIN_ERROR:
        emit TransformCmd(addr,cmd,data);
        break;
    case TCP_CMD_LOGIN_SUCCESS:
        emit TransformCmd(addr,cmd,data);
        break;
    case TCP_CMD_SERVER_GET_HEAD:
        PutFileHead(data);
        break;
    case TCP_CMD_CLIENT_GET_HEAD:
        qDebug()<<"ClientGetHead"<<data;
        this->PutBlock(ADDR,TCP_CMD_CLIENT_GET_HEAD,data);
        break;
    case TCP_CMD_HEAD_DATA:
        GetFileHead(addr,data);
        break;
    case TCP_CMD_HEAD_ERROR:
        Display(data);
        break;
    case TCP_CMD_FILE_DATA:
        GetFileData(addr,data);
        break;
    case TCP_CMD_FILE_ERROR:
        Display(data);
        break;
    case TCP_CMD_SHEEL_CMD:
        proc->start(data);
        proc->waitForFinished();
        PutBlock(quint16(addr),TCP_CMD_SHEEL_RETURN,proc->readAll());
        break;
    case TCP_CMD_SHEEL_RETURN:
        Display(data);
        break;
    case TCP_CMD_LIST_GET:
        emit TransformCmd(addr,cmd,data);
        break;
    case TCP_CMD_LIST_ERROR:
        break;
    case TCP_CMD_SOCKET_DISPLAY:
        emit TransformCmd(addr,cmd,data);
        break;
    default:
        //        qDebug()<<addr<<cmd<<data;
        break;
    }
    timer->start(10000);
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.12.14
  * brief:      获取网卡硬件地址
******************************************************************************/
QString CTcpNetwork::HardwareAddress()
{
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    for (int i=0; i<list.size(); i++) {
        if (list[i].hardwareAddress().size() != 17)
            continue;
        if (list[i].hardwareAddress() == "00:00:00:00:00:00")
            continue;
        return list[i].hardwareAddress();
    }
    return NULL;
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      输出错误信息
******************************************************************************/
void CTcpNetwork::Error(QAbstractSocket::SocketError socketError)
{
    Display(this->errorString().toUtf8());
    if (socketError == QAbstractSocket::RemoteHostClosedError)
        return;
    qDebug()<<"error:"<<this->errorString(); //输出错误信息
    this->close();
}
/******************************************************************************
  * version:    1.0
  * author:     link
  * date:       2016.07.16
  * brief:      显示
******************************************************************************/
void CTcpNetwork::Display(QByteArray msg)
{
    if (TramsmitAddr != this->peerPort())
        emit TransformCmd(TramsmitAddr,TCP_CMD_SOCKET_DISPLAY,msg);
    else
        emit TransformCmd(ADDR,TCP_CMD_SOCKET_DISPLAY,msg);
}
