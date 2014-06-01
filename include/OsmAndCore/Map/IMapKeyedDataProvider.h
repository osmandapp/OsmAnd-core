#ifndef _OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/Map/IMapDataProvider.h>

namespace OsmAnd
{
    class MapKeyedData;

    // IMapKeyedDataProvider describes minimal interface that providers with keyed-based access must implement
    class OSMAND_CORE_API IMapKeyedDataProvider : public IMapDataProvider
    {
        Q_DISABLE_COPY(IMapKeyedDataProvider);

    public:
        typedef const void* Key;

    private:
    protected:
        IMapKeyedDataProvider(const DataType dataType);
    public:
        virtual ~IMapKeyedDataProvider();

        virtual QList<Key> getProvidedDataKeys() const = 0;
        virtual bool obtainData(
            const Key key,
            std::shared_ptr<const MapKeyedData>& outKeyedData,
            const IQueryController* const queryController = nullptr) = 0;
    };

    class OSMAND_CORE_API MapKeyedData : public MapData
    {
        Q_DISABLE_COPY(MapKeyedData);

    public:
        typedef IMapKeyedDataProvider::Key Key;

    private:
    protected:
        MapKeyedData(const DataType dataType, const Key key);
    public:
        virtual ~MapKeyedData();

        const Key key;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_)
