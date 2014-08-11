#ifndef _OSMAND_CORE_MAP_SYMBOL_GROUP_SHAREABLE_BY_ID_H_
#define _OSMAND_CORE_MAP_SYMBOL_GROUP_SHAREABLE_BY_ID_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapSymbolsGroupShareableById : public MapSymbolsGroup
    {
        Q_DISABLE_COPY_AND_MOVE(MapSymbolsGroupShareableById);
    private:
    protected:
    public:
        MapSymbolsGroupShareableById(const uint64_t id);
        virtual ~MapSymbolsGroupShareableById();

        const uint64_t id;

        virtual QString getDebugTitle() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_GROUP_SHAREABLE_BY_ID_H_)
