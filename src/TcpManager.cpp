#include "TcpManager.h"

TcpManager::TcpManager(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
{
    connect(socket_, &QTcpSocket::connected, this, &TcpManager::onSocketConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &TcpManager::onSocketDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &TcpManager::onReadyRead);
    connect(socket_, &QTcpSocket::errorOccurred, this, &TcpManager::onSocketError);
}

void TcpManager::connectToHost(const QString &ip, quint16 port)
{
    socket_->connectToHost(ip, port);
}

void TcpManager::disconnect()
{
    if (socket_->state() == QAbstractSocket::ConnectedState)
        socket_->disconnectFromHost();
}

bool TcpManager::isConnected() const
{
    return socket_->state() == QAbstractSocket::ConnectedState;
}

void TcpManager::onSocketConnected()
{
    buffer_.clear();
    emit connected();
}

void TcpManager::onSocketDisconnected()
{
    emit disconnected();
}

void TcpManager::onReadyRead()
{
    buffer_.append(socket_->readAll());

    while (true) {
        int idx = buffer_.indexOf('\n');
        if (idx < 0)
            break;

        QByteArray line = buffer_.left(idx).trimmed();
        buffer_.remove(0, idx + 1);

        if (!line.isEmpty())
            emit newLineParsed(QString::fromLatin1(line));
    }
}

void TcpManager::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(socket_->errorString());
}
