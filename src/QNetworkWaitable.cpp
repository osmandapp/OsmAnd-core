#include "QNetworkWaitable.h"

OsmAnd::QNetworkWaitable::QNetworkWaitable(QNetworkAccessManager* const networkAccessManager_, QEventLoop& eventLoop_)
    : networkAccessManager(networkAccessManager_)
    , eventLoop(eventLoop_)
{
    _finishedConnection = QObject::connect(
        networkAccessManager, &QNetworkAccessManager::finished,
        (std::function<void(QNetworkReply*)>)std::bind(&QNetworkWaitable::onFinished, this, std::placeholders::_1));
}

OsmAnd::QNetworkWaitable::~QNetworkWaitable()
{
    QObject::disconnect(_finishedConnection);
}

void OsmAnd::QNetworkWaitable::waitForMetaDataOf(QNetworkReply* const networkReply, const bool autoReset /*= true*/)
{
    QMutexLocker scopedLocker(&_mutex);

    volatile bool readyRead = false;
    const auto readyReadConnection = QObject::connect(
        networkReply, &QNetworkReply::readyRead,
        [this, &readyRead]
        ()
        {
            QMutexLocker scopedLocker(&_mutex);
            readyRead = true;
            eventLoop.exit();
        });

    while(!_finishedReplies.contains(networkReply) && !readyRead)
    {
        _mutex.unlock();
        eventLoop.exec();
        _mutex.lock();
    }

    if (autoReset)
        _finishedReplies.remove(networkReply);
    QObject::disconnect(readyReadConnection);
}

void OsmAnd::QNetworkWaitable::waitForFinishOf(QNetworkReply* const networkReply, const bool autoReset /*= true*/)
{
    QMutexLocker scopedLocker(&_mutex);

    while(!_finishedReplies.contains(networkReply))
    {
        _mutex.unlock();
        eventLoop.exec();
        _mutex.lock();
    }

    if (autoReset)
        _finishedReplies.remove(networkReply);
}

void OsmAnd::QNetworkWaitable::reset()
{
    QMutexLocker scopedLocker(&_mutex);

    _finishedReplies.clear();
}

void OsmAnd::QNetworkWaitable::reset(QNetworkReply* const networkReply)
{
    QMutexLocker scopedLocker(&_mutex);

    _finishedReplies.remove(networkReply);
}

void OsmAnd::QNetworkWaitable::onFinished(QNetworkReply *networkReply)
{
    QMutexLocker scopedLocker(&_mutex);
    _finishedReplies.insert(networkReply);
    eventLoop.exit();
}
