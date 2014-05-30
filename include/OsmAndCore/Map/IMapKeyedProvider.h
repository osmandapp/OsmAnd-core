#ifndef _OSMAND_CORE_I_MAP_KEYED_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_KEYED_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/IMapProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapKeyedProvider : public IMapProvider
    {
        Q_DISABLE_COPY(IMapKeyedProvider);

    public:
        typedef const void* Key;

    private:
    protected:
        IMapKeyedProvider();
    public:
        virtual ~IMapKeyedProvider();

        virtual QList<Key> getKeys() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_KEYED_PROVIDER_H_)
