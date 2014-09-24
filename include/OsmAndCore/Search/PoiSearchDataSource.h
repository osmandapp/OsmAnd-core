#ifndef _OSMAND_CORE_POI_SEARCH_DATA_SOURCE_H_
#define _OSMAND_CORE_POI_SEARCH_DATA_SOURCE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Search/ISearchEngine.h>

namespace OsmAnd
{
    class PoiSearchDataSource_P;
    class OSMAND_CORE_API PoiSearchDataSource Q_DECL_FINAL : public ISearchEngine::IDataSource
    {
        Q_DISABLE_COPY_AND_MOVE(PoiSearchDataSource);

    private:
        PrivateImplementation<PoiSearchDataSource_P> _p;
    protected:
    public:
        PoiSearchDataSource(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~PoiSearchDataSource();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_POI_SEARCH_DATA_SOURCE_H_)
