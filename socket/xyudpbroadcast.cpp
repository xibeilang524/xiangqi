﻿#include "xyudpbroadcast.h"
#include <QProcess>
#include <QNetworkInterface>
#include <QDataStream>

QString XYUdpbroadcast::userName;
XYUdpbroadcast::XYUdpbroadcast(QObject *parent)
    : QUdpSocket(parent), port(45954)
{
    // 绑定端口
#if (QT_VERSION >= 0x050000)
    bind(port, QAbstractSocket::ShareAddress);
#else
    bind(port, QUdpSocket::ShareAddress);
#endif
    connect(this, SIGNAL(readyRead()), this, SLOT(receiveBroadcast()));

    // 获取用户名
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";

    QStringList environment = QProcess::systemEnvironment();

    foreach (QString string, envVariables) {
        int index = environment.indexOf(QRegExp(string));
        if (index != -1) {
            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2) {
                userName = stringList.at(1).toUtf8();
                break;
            }
        }
    }

    // 获取本地IP
    ipAddresses.clear();
    foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces()) {
        foreach (QNetworkAddressEntry entry, interface.addressEntries()) {
            QHostAddress broadcastAddress = entry.broadcast();
            if (broadcastAddress != QHostAddress::Null && entry.ip() != QHostAddress::LocalHost) {
                ipAddresses << entry.ip();
            }
        }
    }

    if (userName.isEmpty())
    {
        userName = "Unknown";
    }
}

XYUdpbroadcast::~XYUdpbroadcast()
{

}

QString XYUdpbroadcast::getUserName()
{
    // 获取用户名
    if (userName.isEmpty())
    {
        QStringList envVariables;
        envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                     << "HOSTNAME.*" << "DOMAINNAME.*";

        QStringList environment = QProcess::systemEnvironment();

        foreach (QString string, envVariables) {
            int index = environment.indexOf(QRegExp(string));
            if (index != -1) {
                QStringList stringList = environment.at(index).split('=');
                if (stringList.size() == 2) {
                    userName = stringList.at(1).toUtf8();
                    break;
                }
            }
        }
    }
    return userName;
}

void XYUdpbroadcast::writeUplineDatagram()
{
    QByteArray uplineDatagram;
    QDataStream out(&uplineDatagram, QIODevice::WriteOnly);

    out << qint64(ONLINE)
        << qint64(userName.toUtf8().size())
        << userName.toUtf8();

    writeDatagram(uplineDatagram, QHostAddress(QHostAddress::Broadcast), port);

}

void XYUdpbroadcast::writeOfflineDatagram()
{
    QByteArray offlineDatagram;
    QDataStream out(&offlineDatagram, QIODevice::WriteOnly);

    out << qint64(OFFLINE)
        << qint64(userName.toUtf8().size())
        << userName.toUtf8();

    writeDatagram(offlineDatagram, QHostAddress(QHostAddress::Broadcast), port);
}

void XYUdpbroadcast::writeUserDatagram(const QHostAddress &address, const QByteArray &data, int type)
{
    QByteArray userDataDatagram;
    QDataStream out(&userDataDatagram, QIODevice::WriteOnly);

    out << qint64(USERDATA + type)
        << qint64(userName.toUtf8().size())
        << userName.toUtf8()
        << qint64(data.size())
        << data;

    writeDatagram(userDataDatagram, address, port);
}

bool XYUdpbroadcast::isLocalHostAddress(const QHostAddress &address)
{
    foreach (QHostAddress localAddress, ipAddresses) {
        if (address.toIPv4Address() == localAddress.toIPv4Address())
            return true;
    }
    return false;
}

void XYUdpbroadcast::receiveBroadcast()
{
    while (hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        readDatagram(datagram.data(), datagram.size(),
                     &sender, &senderPort);

        QDataStream in(datagram);

        qint64 type = 0;
        qint64 size = 0;
        QByteArray validData;

        in >> type >> size >> validData;

        // 大小不符合，数据接收错误
        if (validData.size() != size)
        {
            return;
        }
        switch (type)
        {
        case ONLINE:
            if (!isLocalHostAddress(sender))
            {
                emit peopleUpline(QString::fromUtf8(validData), sender);
            }
            break;
        case OFFLINE:
            emit peopleOffline(QString::fromUtf8(validData), sender);
            break;
        default:
            if (!isLocalHostAddress(sender))
            {
                QByteArray userData;
                in >> size >> userData;
                if (userData.size() != size)
                {
                    return;
                }
                emit receiveUserData(QString::fromUtf8(validData), userData, type - USERDATA);
            }
            break;
        }
    }
}

