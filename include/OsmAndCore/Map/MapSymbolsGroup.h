#ifndef _OSMAND_CORE_MAP_SYMBOL_GROUP_H_
#define _OSMAND_CORE_MAP_SYMBOL_GROUP_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapSymbolsGroup
    {
        Q_DISABLE_COPY(MapSymbolsGroup);

    public:
        enum class PresentationMode : unsigned int
        {
            ShowAnything = 0,
            ShowAllOrNothing,
            ShowAllCaptionsOrNoCaptions,
            ShowNoneIfIconIsNotShown,
        };

    private:
    protected:
    public:
        MapSymbolsGroup();
        virtual ~MapSymbolsGroup();

        unsigned int presentationModesMask;
        void setPresentationMode(const PresentationMode mode);
        bool hasPresentationMode(const PresentationMode mode) const;

        QList< std::shared_ptr<MapSymbol> > symbols;
        std::shared_ptr<MapSymbol> getFirstSymbolWithContentClass(const MapSymbol::ContentClass contentClass) const;
        unsigned int numberOfSymbolsWithContentClass(const MapSymbol::ContentClass contentClass) const;

        virtual QString getDebugTitle() const;
    };

    class OSMAND_CORE_API IUpdatableMapSymbolsGroup
    {
    private:
    protected:
        IUpdatableMapSymbolsGroup();
    public:
        virtual ~IUpdatableMapSymbolsGroup();

        virtual bool update() = 0;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_GROUP_H_)
