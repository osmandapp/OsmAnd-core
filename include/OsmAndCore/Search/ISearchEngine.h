#ifndef _OSMAND_CORE_I_SEARCH_ENGINE_H_
#define _OSMAND_CORE_I_SEARCH_ENGINE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QLinkedList>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>

namespace OsmAnd
{
    class ISearchSession;

    class OSMAND_CORE_API ISearchEngine
    {
        Q_DISABLE_COPY_AND_MOVE(ISearchEngine);

    public:
        class OSMAND_CORE_API IDataSource
        {
            Q_DISABLE_COPY_AND_MOVE(IDataSource);

        private:
        protected:
            IDataSource();
        public:
            virtual ~IDataSource();
        };

        class OSMAND_CORE_API ISearchResult
        {
            Q_DISABLE_COPY_AND_MOVE(ISearchResult);

        private:
        protected:
            ISearchResult();
        public:
            virtual ~ISearchResult();

            virtual float getMatchFactor() const = 0;
            virtual QString getMatchString() const = 0;
            virtual QList<QStringRef> getMatchedSubstrings() const = 0;
        };

        class OSMAND_CORE_API ISearchResults
        {
            Q_DISABLE_COPY_AND_MOVE(ISearchResults);

        private:
        protected:
            ISearchResults();
        public:
            virtual ~ISearchResults();

            virtual QString getQuery() const = 0;
            virtual QLinkedList< std::shared_ptr<const ISearchResult> > getResults() const = 0;
        };

    private:
    protected:
        ISearchEngine();
    public:
        virtual ~ISearchEngine();

        virtual void addDataSource(const std::shared_ptr<const IDataSource>& dataSource) = 0;
        virtual bool removeDataSource(const std::shared_ptr<const IDataSource>& dataSource) = 0;
        virtual unsigned int removeAllDataSources() = 0;
        virtual QList< std::shared_ptr<const IDataSource> > getDataSources() const = 0;

        virtual std::shared_ptr<ISearchSession> createSession() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_SEARCH_ENGINE_H_)
