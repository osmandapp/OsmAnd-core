#ifndef _OSMAND_CORE_MAP_SYMBOLS_GROUP_H_
#define _OSMAND_CORE_MAP_SYMBOLS_GROUP_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Bitmask.h>
#include <OsmAndCore/Map/MapSymbol.h>
#include <OsmAndCore/Map/IBillboardMapSymbol.h>
#include <OsmAndCore/Map/IOnSurfaceMapSymbol.h>
#include <OsmAndCore/Map/IOnPathMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapSymbolsGroup
    {
        Q_DISABLE_COPY_AND_MOVE(MapSymbolsGroup);

    public:
        enum class PresentationModeFlag : unsigned int
        {
            ShowAnything = 0,
            ShowAllOrNothing,
            ShowAllCaptionsOrNoCaptions,
            ShowNoneIfIconIsNotShown,
        };
        typedef Bitmask<PresentationModeFlag> PresentationMode;

        // Additional instance shares all resources and settings. It's lifetime ends with original group.
        // Additional instance allows to customize:
        //  - IBillboardMapSymbol: position31, offset
        //  - IOnSurfaceMapSymbol: position31, direction
        //  - IOnPathMapSymbol: pinPointOnPath
        // Symbols are still processed in order of original group
        struct AdditionalSymbolInstanceParameters;
        struct OSMAND_CORE_API AdditionalInstance
        {
            AdditionalInstance(
                const std::shared_ptr<MapSymbolsGroup>& originalGroup);
            virtual ~AdditionalInstance();

            const std::weak_ptr<MapSymbolsGroup> originalGroup;

            QHash< std::shared_ptr<MapSymbol>, std::shared_ptr<AdditionalSymbolInstanceParameters> > symbols;
            std::shared_ptr<MapSymbol> getFirstSymbolWithContentClass(const MapSymbol::ContentClass contentClass) const;
            unsigned int numberOfSymbolsWithContentClass(const MapSymbol::ContentClass contentClass) const;

        private:
            Q_DISABLE_COPY(AdditionalInstance);
        };

        struct OSMAND_CORE_API AdditionalSymbolInstanceParameters
        {
            AdditionalSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr);
            virtual ~AdditionalSymbolInstanceParameters();

            const AdditionalInstance* const groupInstancePtr;
        
        private:
            Q_DISABLE_COPY(AdditionalSymbolInstanceParameters);
        };

        struct OSMAND_CORE_API AdditionalBillboardSymbolInstanceParameters : public AdditionalSymbolInstanceParameters
        {
            AdditionalBillboardSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr);
            virtual ~AdditionalBillboardSymbolInstanceParameters();

            bool overridesPosition31;
            PointI position31;

            bool overridesOffset;
            PointI offset;

        private:
            Q_DISABLE_COPY(AdditionalBillboardSymbolInstanceParameters);
        };

        struct OSMAND_CORE_API AdditionalOnSurfaceSymbolInstanceParameters : public AdditionalSymbolInstanceParameters
        {
            AdditionalOnSurfaceSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr);
            virtual ~AdditionalOnSurfaceSymbolInstanceParameters();

            bool overridesPosition31;
            PointI position31;

            bool overridesDirection;
            float direction;

        private:
            Q_DISABLE_COPY(AdditionalOnSurfaceSymbolInstanceParameters);
        };

        struct OSMAND_CORE_API AdditionalOnPathSymbolInstanceParameters : public AdditionalSymbolInstanceParameters
        {
            AdditionalOnPathSymbolInstanceParameters(const AdditionalInstance* const groupInstancePtr);
            virtual ~AdditionalOnPathSymbolInstanceParameters();

            bool overridesPinPointOnPath;
            IOnPathMapSymbol::PinPoint pinPointOnPath;

        private:
            Q_DISABLE_COPY(AdditionalOnPathSymbolInstanceParameters);
        };

    private:
    protected:
    public:
        MapSymbolsGroup();
        virtual ~MapSymbolsGroup();

        PresentationMode presentationMode;

        QList< std::shared_ptr<MapSymbol> > symbols;
        std::shared_ptr<MapSymbol> getFirstSymbolWithContentClass(const MapSymbol::ContentClass contentClass) const;
        unsigned int numberOfSymbolsWithContentClass(const MapSymbol::ContentClass contentClass) const;

        virtual QString getDebugTitle() const;

        bool additionalInstancesDiscardOriginal;
        QList< std::shared_ptr<AdditionalInstance> > additionalInstances;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOLS_GROUP_H_)
