#ifndef _OSMAND_CORE_Q_NETWORK_WAITABLE_H_
#define _OSMAND_CORE_Q_NETWORK_WAITABLE_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QMutex>
#include <QEventLoop>

#include "OsmAndCore.h"

namespace OsmAnd
{
    class QNetworkWaitable
    {
    private:
    protected:
        mutable QMutex _mutex;

        void onFinished(QNetworkReply *networkReply);
        QMetaObject::Connection _finishedConnection;
        QSet<QNetworkReply*> _finishedReplies;
    public:
        QNetworkWaitable(QNetworkAccessManager* const networkAccessManager, QEventLoop& eventLoop);
        virtual ~QNetworkWaitable();

        QNetworkAccessManager* const networkAccessManager;
        QEventLoop& eventLoop;

        void waitForMetaDataOf(QNetworkReply* const networkReply, const bool autoReset = true);
        void waitForFinishOf(QNetworkReply* const networkReply, const bool autoReset = true);
        void reset();
        void reset(QNetworkReply* const networkReply);
    };
}

#endif // !defined(_OSMAND_CORE_Q_NETWORK_WAITABLE_H_)

