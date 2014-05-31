#ifndef _OSMAND_CORE_MAP_SYMBOL_GROUP_H_
#define _OSMAND_CORE_MAP_SYMBOL_GROUP_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class MapSymbol;

    class OSMAND_CORE_API MapSymbolsGroup
    {
        Q_DISABLE_COPY(MapSymbolsGroup);
    private:
    protected:
    public:
        MapSymbolsGroup();
        virtual ~MapSymbolsGroup();

        QList< std::shared_ptr<const MapSymbol> > symbols;

        virtual QString getDebugTitle() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_GROUP_H_)
