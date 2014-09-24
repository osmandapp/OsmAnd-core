#ifndef _OSMAND_CORE_POI_SEARCH_DATA_SOURCE_P_H_
#define _OSMAND_CORE_POI_SEARCH_DATA_SOURCE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "IObfsCollection.h"

namespace OsmAnd
{
    class PoiSearchDataSource;
    class PoiSearchDataSource_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(PoiSearchDataSource_P);

    private:
    protected:
        PoiSearchDataSource_P(PoiSearchDataSource* const owner);
    public:
        ~PoiSearchDataSource_P();

        ImplementationInterface<PoiSearchDataSource> owner;

    friend class OsmAnd::PoiSearchDataSource;
    };
}

#endif // !defined(_OSMAND_CORE_POI_SEARCH_DATA_SOURCE_P_H_)
