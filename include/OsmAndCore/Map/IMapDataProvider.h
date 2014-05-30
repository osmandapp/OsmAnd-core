#ifndef _OSMAND_CORE_I_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    // IMapDataProvider describes minimal interface that providers of any type of data must implement and support
    class OSMAND_CORE_API IMapDataProvider
    {
        Q_DISABLE_COPY(IMapDataProvider);

    public:
        enum class DataType
        {
            Unknown = -1,

            // Static tiled data:
            BinaryMapDataTile,
            //FUTURE:VectorMapTile
            RasterBitmapTile,
            ElevationDataTile,
            SymbolsTile,

            // Dynamic keyed data:
            Symbols,
        };

    private:
    protected:
        IMapDataProvider(const DataType dataType);
    public:
        virtual ~IMapDataProvider();

        const DataType dataType;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_DATA_PROVIDER_H_)
