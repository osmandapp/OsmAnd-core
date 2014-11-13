#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfAddressSectionInfo;
    class StreetGroup;
    class Street;
    class Building;
    class StreetIntersection;
    class IQueryController;

    class OSMAND_CORE_API ObfAddressSectionReader
    {
    private:
        ObfAddressSectionReader();
        ~ObfAddressSectionReader();
    protected:
    public:
        static void loadStreetGroups(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const StreetGroup> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::StreetGroup>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);

        static void loadStreetsFromGroup(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const StreetGroup>& group,
            QList< std::shared_ptr<const Street> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Street>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadBuildingsFromStreet(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const Building> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Building>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadIntersectionsFromStreet(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Street>& street,
            QList< std::shared_ptr<const StreetIntersection> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::StreetIntersection>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_)
