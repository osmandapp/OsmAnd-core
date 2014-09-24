#ifndef _OSMAND_CORE_BASE_SEARCH_ENGINE_H_
#define _OSMAND_CORE_BASE_SEARCH_ENGINE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Search/ISearchEngine.h>

namespace OsmAnd
{
    class BaseSearchEngine_P;
    class OSMAND_CORE_API BaseSearchEngine : public ISearchEngine
    {
        Q_DISABLE_COPY_AND_MOVE(BaseSearchEngine);

    public:
        /*class OSMAND_CORE_API ISearchResult
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

        class OSMAND_CORE_API BaseSearchResults : public ISearchResults
        {
            Q_DISABLE_COPY_AND_MOVE(BaseSearchResults);

        private:
        protected:
            BaseSearchResults();
        public:
            virtual ~BaseSearchResults();

            virtual QLinkedList< std::shared_ptr<const ISearchResult> > getResults() const;
        };*/
    private:
    protected:
        PrivateImplementation<BaseSearchEngine_P> _p;

        BaseSearchEngine(BaseSearchEngine_P* const p);
    public:
        virtual ~BaseSearchEngine();

        virtual void addDataSource(const std::shared_ptr<const IDataSource>& dataSource);
        virtual bool removeDataSource(const std::shared_ptr<const IDataSource>& dataSource);
        virtual unsigned int removeAllDataSources();
        virtual QList< std::shared_ptr<const IDataSource> > getDataSources() const;
    };
}

#endif // !defined(_OSMAND_CORE_BASE_SEARCH_ENGINE_H_)
