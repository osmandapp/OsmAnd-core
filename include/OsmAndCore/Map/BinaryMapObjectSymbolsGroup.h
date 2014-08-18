#ifndef _OSMAND_CORE_BINARY_MAP_OBJECT_SYMBOLS_GROUP_H_
#define _OSMAND_CORE_BINARY_MAP_OBJECT_SYMBOLS_GROUP_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IMapSymbolsGroupWithUniqueId.h>

namespace OsmAnd
{
    namespace Model
    {
        class BinaryMapObject;
    }

    class OSMAND_CORE_API BinaryMapObjectSymbolsGroup
        : public MapSymbolsGroup
        , public IMapSymbolsGroupWithUniqueId
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapObjectSymbolsGroup);
    private:
    protected:
    public:
        BinaryMapObjectSymbolsGroup(
            const std::shared_ptr<const Model::BinaryMapObject>& mapObject,
            const bool sharableById);
        virtual ~BinaryMapObjectSymbolsGroup();

        const std::shared_ptr<const Model::BinaryMapObject> mapObject;
        
        const uint64_t id;
        virtual uint64_t getId() const;

        const bool sharableById;
        virtual bool isSharableById() const;

        virtual QString getDebugTitle() const;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_OBJECT_SYMBOLS_GROUP_H_)
