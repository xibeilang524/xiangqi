﻿#ifndef XYUDPBROADCAST_H
#define XYUDPBROADCAST_H

#include <QUdpSocket>

class XYUdpbroadcast : public QUdpSocket
{
    Q_OBJECT
public:
    enum DATATYPE {ONLINE = 45953, OFFLINE = 45954, USERDATA = 45955};
    explicit XYUdpbroadcast(QObject *parent = 0);
    ~XYUdpbroadcast();
    static QString getUserName();
    void writeUplineDatagram();
    void writeOfflineDatagram();
    void writeUserDatagram(const QHostAddress &address, const QByteArray &data, int type);

signals:
    void peopleUpline(const QString &name, const QHostAddress &address);
    void peopleOffline(const QString &name, const QHostAddress &address);
    void receiveUserData(const QString &from, const QByteArray &data, int type);

public slots:
    bool isLocalHostAddress(const QHostAddress &address);
    void receiveBroadcast();

private:
    static QString      userName;
    QList<QHostAddress> ipAddresses;
    int      port;
};

#endif // XYUDPBROADCAST_H
