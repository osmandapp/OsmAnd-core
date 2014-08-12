#ifndef _OSMAND_CORE_MAP_SYMBOLS_GROUP_WITH_ID_H_
#define _OSMAND_CORE_MAP_SYMBOLS_GROUP_WITH_ID_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapSymbolsGroupWithId : public MapSymbolsGroup
    {
        Q_DISABLE_COPY_AND_MOVE(MapSymbolsGroupWithId);
    private:
    protected:
    public:
        MapSymbolsGroupWithId(const uint64_t id, const bool sharableById);
        virtual ~MapSymbolsGroupWithId();

        const uint64_t id;
        const bool sharableById;

        virtual QString getDebugTitle() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOLS_GROUP_WITH_ID_H_)
