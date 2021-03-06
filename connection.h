#ifndef CONNECTION_H
#define CONNECTION_H

#include <QUdpSocket>
#include <QQueue>
#include <QSet>
#include <QNetworkInterface>
#include <QDateTime>
#include <QtEndian>

#include "payload.h"

#define PORT 37647
#define LOCAL_IP "192.168.1.116"
#define SATELLITE_IP "192.168.1.255"

#define TELECOMMAND_TOPIC_ID 5555


class Connection : public QObject
{
    Q_OBJECT

    QHostAddress localAddress;
    QHostAddress remoteAddress;
    quint16 port;
    QUdpSocket udpSocket;
    bool bound;
    bool checkChecksum;
    QSet<quint32> topics;
    QQueue<PayloadSatellite> payloads;

signals:
    void readReady();
    void updateConsole();

private slots:
    void connectionReceive();

public:
    QString consoleText;

    explicit Connection(QObject *parent = 0, bool checkChecksum = false);
    void addTopic(PayloadType);
    void connectionSendData(quint32 topicId, const QByteArray &data);
    void connectionSendCommand(quint32 topicID, const Command &telecommand);
    PayloadSatellite read();
    bool isBound();
    bool isReadReady();    
    void bind();

private:
    void console(QString msg);
};

#endif // CONNECTION_H
