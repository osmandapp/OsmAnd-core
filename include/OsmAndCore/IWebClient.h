#ifndef _OSMAND_CORE_I_WEB_CLIENT_H_
#define _OSMAND_CORE_I_WEB_CLIENT_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class IQueryController;

    class OSMAND_CORE_API IWebClient
    {
        Q_DISABLE_COPY_AND_MOVE(IWebClient);

    public:
        typedef std::function<void(
            const uint64_t transferredBytes,
            const uint64_t totalBytes)> RequestProgressCallbackSignature;

        class OSMAND_CORE_API IRequestResult
        {
            Q_DISABLE_COPY_AND_MOVE(IRequestResult);

        private:
        protected:
            IRequestResult();
        public:
            virtual ~IRequestResult();

            virtual bool isSuccessful() const = 0;
        };

        class OSMAND_CORE_API IHttpRequestResult : public IRequestResult
        {
            Q_DISABLE_COPY_AND_MOVE(IHttpRequestResult);

        private:
        protected:
            IHttpRequestResult();
        public:
            virtual ~IHttpRequestResult();

            virtual unsigned int getHttpStatusCode() const = 0;
        };

        struct OSMAND_CORE_API DataRequest
        {
            Q_DISABLE_COPY_AND_MOVE(DataRequest);

            DataRequest();
            DataRequest(std::nullptr_t);
            virtual ~DataRequest();

            std::shared_ptr<const IRequestResult> requestResult;
            RequestProgressCallbackSignature progressCallback;
            std::shared_ptr<const IQueryController> queryController;
        };

    private:
    protected:
        IWebClient();
    public:
        virtual ~IWebClient();

        virtual QByteArray downloadData(
            const QString& url,
            SWIG_CLARIFY(IWebClient, DataRequest)& dataRequest,
            const QString& userAgent = QString()) const = 0;
        virtual QString downloadString(
            const QString& url,
            SWIG_CLARIFY(IWebClient, DataRequest)& dataRequest) const = 0;
        virtual long long downloadFile(
            const QString& url,
            const QString& fileName,
            const long long lastTime,
            SWIG_CLARIFY(IWebClient, DataRequest)& dataRequest) const = 0;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(IWebClient);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QByteArray,
            downloadData,
            const QString& url,
            SWIG_CLARIFY(IWebClient, DataRequest)& dataRequest,
            const QString& userAgent);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QString,
            downloadString,
            const QString& url,
            SWIG_CLARIFY(IWebClient, DataRequest)& dataRequest);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            long long,
            downloadFile,
            const QString& url,
            const QString& fileName,
            const long long lastTime,
            SWIG_CLARIFY(IWebClient, DataRequest)& dataRequest);
    SWIG_EMIT_DIRECTOR_END(IWebClient);
}

#endif // !defined(_OSMAND_CORE_I_WEB_CLIENT_H_)

