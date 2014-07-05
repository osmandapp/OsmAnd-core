#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_READER_P_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_READER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QList>
#include <QSet>
#include <QMap>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "ObfRoutingSectionInfo_P.h"

namespace OsmAnd {

    class ObfReader_P;
    class ObfRoutingSectionInfo;
    class ObfRoutingEncodingRule;
    class ObfRoutingSubsectionInfo;
    class ObfRoutingBorderLineHeader;
    class ObfRoutingBorderLinePoint;
    namespace Model {
        class Road;
    } // namespace Model
    class IQueryController;
    class IQueryFilter;

    class ObfRoutingSectionReader;
    class ObfRoutingSectionReader_P Q_DECL_FINAL
    {
    private:
        ObfRoutingSectionReader_P();
        ~ObfRoutingSectionReader_P();
    protected:
    
        enum {
            ShiftCoordinates = 4,
        };

        static void read(const ObfReader_P& reader, const std::shared_ptr<ObfRoutingSectionInfo>& section);

        static void readEncodingRule(const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            const std::shared_ptr<ObfRoutingSectionInfo_P::EncodingRule>& rule);
        static void readSubsectionHeader(const ObfReader_P& reader, const std::shared_ptr<ObfRoutingSubsectionInfo>& subsection,
            const std::shared_ptr<const ObfRoutingSubsectionInfo>& parent, const unsigned int depth = std::numeric_limits<unsigned int>::max());
        static void readSubsectionChildrenHeaders(const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            const unsigned int depth = std::numeric_limits<unsigned int>::max());
        static void readSubsectionData(const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList< std::shared_ptr<const Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const Model::Road>&)> visitor = nullptr);
        static void readSubsectionRoadsIds(const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList<uint64_t>& ids);
        static void readSubsectionRestriction(
            const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            const QMap< uint32_t, std::shared_ptr<Model::Road> >& roads, const QList<uint64_t>& roadsInternalIdToGlobalIdMap);
        static void readRoad(const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            const QList<uint64_t>& idsTable, uint32_t& internalId, const std::shared_ptr<Model::Road>& road );

        static void readBorderBoxLinesHeaders(const ObfReader_P& reader,
            QList< std::shared_ptr<const ObfRoutingBorderLineHeader> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitor = nullptr);
        static void readBorderLineHeader(const ObfReader_P& reader,
            const std::shared_ptr<ObfRoutingBorderLineHeader>& borderLine, uint32_t outerOffset);
        static void readBorderLinePoints(const ObfReader_P& reader,
            QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitor = nullptr);
        static void readBorderLinePoint(const ObfReader_P& reader,
            const std::shared_ptr<ObfRoutingBorderLinePoint>& point);

        static void querySubsections(const ObfReader_P& reader, const QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >& in,
            QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingSubsectionInfo>&)> visitor = nullptr);

        static void loadSubsectionData(const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList< std::shared_ptr<const Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Road>&)> visitor = nullptr);

        static void loadSubsectionBorderBoxLinesPoints(const ObfReader_P& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitorLine = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitorPoint = nullptr);

    friend class OsmAnd::ObfRoutingSectionReader;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_READER_P_H_)
