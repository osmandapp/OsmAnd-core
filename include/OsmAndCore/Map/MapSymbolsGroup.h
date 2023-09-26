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
            ShowAnythingUntilFirstGap,
        };
        typedef Bitmask<PresentationModeFlag> PresentationMode;

        enum class IntersectionProcessingModeFlag : unsigned int
        {
            CheckIntersectionsWithinGroup = 0,
        };
        typedef Bitmask<IntersectionProcessingModeFlag> IntersectionProcessingMode;

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

            QHash< std::shared_ptr<MapSymbol>, std::shared_ptr<const AdditionalSymbolInstanceParameters> > symbols;
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
            bool discardableByAnotherInstances;         
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

        typedef uint64_t SharingKey;

        // Map symbols groups comparator using sorting key:
        // - If key is available, sort by key in ascending order in case keys differ. Otherwise sort by pointer
        // - If key is unavailable, sort by pointer
        // - If key vs no-key is sorted, no-key is always goes "greater" than key
        //NOTE: Symbol ordering should take into account ordering of primitives actually (in cases that apply)
        typedef uint64_t SortingKey;
        struct OSMAND_CORE_API Comparator Q_DECL_FINAL
        {
            Comparator();

            bool operator()(
                const std::shared_ptr<const MapSymbolsGroup>& l,
                const std::shared_ptr<const MapSymbolsGroup>& r) const;
        };
    private:
    protected:
    public:
        MapSymbolsGroup();
        virtual ~MapSymbolsGroup();

        virtual bool obtainSharingKey(SharingKey& outKey) const;
        virtual bool obtainSortingKey(SortingKey& outKey) const;
        virtual QString toString() const;

        PresentationMode presentationMode;
        IntersectionProcessingMode intersectionProcessingMode;

        QList< std::shared_ptr<MapSymbol> > symbols;
        std::shared_ptr<MapSymbol> getFirstSymbolWithContentClass(const MapSymbol::ContentClass contentClass) const;
        unsigned int numberOfSymbolsWithContentClass(const MapSymbol::ContentClass contentClass) const;

        bool additionalInstancesDiscardOriginal;
        QList< std::shared_ptr<AdditionalInstance> > additionalInstances;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOLS_GROUP_H_)
