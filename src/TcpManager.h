#pragma once

#include <QObject>
#include <QTcpSocket>

class TcpManager : public QObject {
    Q_OBJECT

    QTcpSocket *socket_;
    QByteArray buffer_;

public:
    explicit TcpManager(QObject *parent = nullptr);

    void connectToHost(const QString &ip, quint16 port);
    void disconnect();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void newLineParsed(const QString &line);
    void errorOccurred(const QString &message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
};
