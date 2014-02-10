#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataTypes.h>

namespace OsmAnd
{
    class ObfReader;
    class ObfAddressSectionInfo;
    namespace Model
    {
        class StreetGroup;
        class Street;
        class Building;
        class StreetIntersection;
    } // namespace Model
    class IQueryController;

    class OSMAND_CORE_API ObfAddressSectionReader
    {
    private:
        ObfAddressSectionReader();
        ~ObfAddressSectionReader();
    protected:
    public:
        static void loadStreetGroups(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfAddressSectionInfo>& section,
            QList< std::shared_ptr<const Model::StreetGroup> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetGroup>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr,
            QSet<ObfAddressBlockType>* blockTypeFilter = nullptr);

        static void loadStreetsFromGroup(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::StreetGroup>& group,
            QList< std::shared_ptr<const Model::Street> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Street>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadBuildingsFromStreet(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::Building> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Building>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);

        static void loadIntersectionsFromStreet(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const Model::Street>& street,
            QList< std::shared_ptr<const Model::StreetIntersection> >* resultOut = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::StreetIntersection>&)> visitor = nullptr,
            const IQueryController* const controller = nullptr);
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_)
