#ifndef _OSMAND_CORE_DOWNLOAD_MANAGER_P_H_
#define _OSMAND_CORE_DOWNLOAD_MANAGER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QString>
#include <QIODevice>
#include <QUrl>
#include <QReadWriteLock>
#include <QMutex>
#include <QWaitCondition>
#include <QThreadPool>
#include <QRunnable>
#include <QAtomicInt>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "WebClient.h"

namespace OsmAnd
{
    class WebClient;
    class WebClient_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY(WebClient_P);
    public:
        typedef WebClient::RequestProgressCallbackSignature RequestProgressCallbackSignature;
        typedef WebClient::RequestResult RequestResult;
        typedef WebClient::HttpRequestResult HttpRequestResult;

    private:
    protected:
        WebClient_P(
            WebClient* const owner,
            const QString& userAgent,
            const unsigned int concurrentRequestsLimit,
            const unsigned int retriesLimit,
            const bool followRedirects);

        static QReadWriteLock _defaultUserAgentLock;
        static QString _defaultUserAgent;
        mutable QReadWriteLock _userAgentLock;
        QString _userAgent;

        mutable QAtomicInt _retriesLimit;
        mutable QAtomicInt _followRedirects;

        QThreadPool _threadPool;

        class Request : QRunnable
        {
        public:
            typedef std::function<uint64_t(QNetworkReply& networkReply)> DataConsumer;
            typedef std::function<void(qint64 transferredBytes, qint64 totalBytes)> TransferProgressCallback;

        private:
        protected:
            Request(
                const QNetworkRequest& networkRequest,
                const QString& userAgent,
                const unsigned int retriesLimit,
                const bool followRedirects,
                const DataConsumer dataConsumer,
                const TransferProgressCallback downloadProgressCallback,
                const TransferProgressCallback uploadProgressCallback);

            virtual void run();

            void onReadyRead();
            void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
            void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

            mutable QWaitCondition _finishedCondition;
            mutable QMutex _finishedConditionMutex;

            QNetworkReply* _lastNetworkReply;
            uint64_t _totalBytesConsumed;
        public:
            virtual ~Request();

            const QNetworkRequest originalNetworkRequest;
            const QString userAgent;
            const unsigned int retriesLimit;
            const bool followRedirects;
            const DataConsumer dataConsumer;
            const TransferProgressCallback downloadProgressCallback;
            const TransferProgressCallback uploadProgressCallback;

            void waitUntilFinished() const;

        friend class OsmAnd::WebClient_P;
        };
    public:
        virtual ~WebClient_P();

        ImplementationInterface<WebClient> owner;

        // "User-Agent" setting:
        static QString getDefaultUserAgent();
        static void setDefaultUserAgent(const QString& userAgent);
        QString getUserAgent() const;
        void setUserAgent(const QString& userAgent);

        // Concurrent requests limit:
        unsigned int getConcurrentRequestsLimit() const;
        void setConcurrentRequestsLimit(const unsigned int newLimit);

        // Retries limit:
        unsigned int getRetriesLimit() const;
        void setRetriesLimit(const unsigned int newLimit);

        // Redirects following:
        bool getFollowRedirects() const;
        void setFollowRedirects(const bool followRedirects);

        // Operations:
        QByteArray downloadData(
            const QNetworkRequest& networkRequest,
            std::shared_ptr<const RequestResult>* const requestResult,
            const RequestProgressCallbackSignature progressCallback);
        QString downloadString(
            const QNetworkRequest& networkRequest,
            std::shared_ptr<const RequestResult>* const requestResult,
            const RequestProgressCallbackSignature progressCallback);
        bool downloadFile(
            const QNetworkRequest& networkRequest,
            const QString& fileName,
            std::shared_ptr<const RequestResult>* const requestResult,
            const RequestProgressCallbackSignature progressCallback);

    friend class OsmAnd::WebClient;
    };
}

#endif // !defined(_OSMAND_CORE_DOWNLOAD_MANAGER_P_H_)
