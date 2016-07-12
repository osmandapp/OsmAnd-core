#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/Utilities.h>

namespace OsmAnd
{
    class Address;
    class Building;
    class IQueryController;
    class ObfAddressSectionInfo;
    class ObfReader;
    class ObfStreet;
    class ObfStreetGroup;
    class Street;
    class StreetGroup;
    class StreetIntersection;

    class OSMAND_CORE_API ObfAddressSectionReader
    {
    public:
        using VisitorFunction = std::function<void(const OsmAnd::Address& address)>;
        using StreetGroupVisitorFunction = std::function<void(const OsmAnd::StreetGroup& streetGroup)>;
        using StreetVisitorFunction = std::function<void(const OsmAnd::Street& street)>;
        using BuildingVisitorFunction = std::function<void(const OsmAnd::Building& building)>;
        using IntersectionVisitorFunction = std::function<void(const OsmAnd::StreetIntersection& streetIntersection)>;
        using StringMatcherFunction = std::function<bool(const QString&, const QString&)>;

        class Filter
        {
        private:
            QString _name;
            Nullable<AreaI> _bbox31;
            Bitmask<AddressType> _addressTypes;
            ObfAddressStreetGroupTypesMask _streetGroupTypes = fullObfAddressStreetGroupTypesMask();
            QVector<const Filter> _filters;

            VisitorFunction _addressVisitor;
            StreetGroupVisitorFunction _streetGroupVisitor;
            StreetVisitorFunction _streetVisitor;
            BuildingVisitorFunction _buildingVisitor;
            IntersectionVisitorFunction _intersectionVisitor;

            const std::function<bool(bool, bool)> _op;

        public:
            Filter(std::function<bool(bool, bool)> op = AND);

            QString name() const;
            Filter& setName(const QString& name);
            Filter& setBbox(const AreaI& bbox31);
            Filter& setAddressStreetGroupTypes(const ObfAddressStreetGroupTypesMask& streetGroupTypes);
            Filter& setFilters(QVector<const Filter&> filters);
            Filter& setVisitor(VisitorFunction visitor);
            Filter& setVisitor(StreetGroupVisitorFunction visitor);
            Filter& setVisitor(StreetVisitorFunction visitor);
            Filter& setVisitor(BuildingVisitorFunction visitor);
            Filter& setVisitor(IntersectionVisitorFunction visitor);

//            bool matches(
//                    const QString& name,
//                    const StringMatcherFunction& matcher = OsmAnd::MATCHES) const;
            bool matches(
                    const QString& name,
                    const QHash<QString, QString>& names = {},
                    const StringMatcherFunction& matcher = OsmAnd::CONTAINS) const;
            bool contains(const AreaI& bbox31) const;
            bool contains(const PointI& point) const;
//            bool filter(const QString& name, const AreaI& bbox) const;
//            bool filter(
//                    const Street& street,
//                    const StringMatcherFunction& matcher = OsmAnd::MATCHES,
//                    bool call v) const;
            bool filter(
                    const Address& address,
                    const StringMatcherFunction& matcher = OsmAnd::MATCHES,
                    bool callVisitor = true) const;
            bool filter(
                    const StreetGroup& streetGroup,
                    const StringMatcherFunction& matcher = OsmAnd::MATCHES,
                    bool callVisitor = true) const;
            bool hasType(
                    const AddressType type) const;
            bool hasStreetGroupType(
                    const ObfAddressStreetGroupType type) const;
        };


    private:
        ObfAddressSectionReader();
        ~ObfAddressSectionReader();
    protected:
    public:
        static void loadStreetGroups(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            const Filter &filter = Filter{},
            QList< std::shared_ptr<const ObfStreetGroup> >* resultOut = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadStreetsFromGroup(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfStreetGroup>& streetGroup,
            const Filter &filter = Filter{},
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadBuildingsFromStreet(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfStreet>& street,
            const Filter &filter = Filter{},
            QList< std::shared_ptr<const Building> >* resultOut = nullptr,
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void loadIntersectionsFromStreet(const std::shared_ptr<const ObfReader>& reader,
            const ObfStreet& street,
            const Filter& filter = Filter{},
            const std::shared_ptr<const IQueryController>& queryController = nullptr);

        static void scanAddressesByName(
            const std::shared_ptr<const ObfReader>& reader,
            const std::shared_ptr<const ObfAddressSectionInfo>& section,
            const Filter& filter = Filter{},
            const std::shared_ptr<const IQueryController>& queryController = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_READER_H_)
