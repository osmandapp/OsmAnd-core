#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_READER_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfReader;
    class ObfRoutingSectionInfo;
    class ObfRoutingSubsectionInfo;
    class ObfRoutingBorderLineHeader;
    class ObfRoutingBorderLinePoint;
    namespace Model {
        class Road;
    } // namespace Model
    class IQueryFilter;

    class OSMAND_CORE_API ObfRoutingSectionReader
    {
    private:
        ObfRoutingSectionReader();
        ~ObfRoutingSectionReader();
    protected:
    public:
        static void querySubsections(const std::shared_ptr<ObfReader>& reader, const QList< std::shared_ptr<ObfRoutingSubsectionInfo> >& in,
            QList< std::shared_ptr<const ObfRoutingSubsectionInfo> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingSubsectionInfo>&)> visitor = nullptr);

        static void loadSubsectionData(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSubsectionInfo>& subsection,
            QList< std::shared_ptr<const Model::Road> >* resultOut = nullptr,
            QMap< uint64_t, std::shared_ptr<const Model::Road> >* resultMapOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const OsmAnd::Model::Road>&)> visitor = nullptr);

        static void loadSubsectionBorderBoxLinesPoints(const std::shared_ptr<ObfReader>& reader, const std::shared_ptr<const ObfRoutingSectionInfo>& section,
            QList< std::shared_ptr<const ObfRoutingBorderLinePoint> >* resultOut = nullptr,
            IQueryFilter* filter = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLineHeader>&)> visitorLine = nullptr,
            std::function<bool (const std::shared_ptr<const ObfRoutingBorderLinePoint>&)> visitorPoint = nullptr);
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_READER_H_)
