#ifndef _OSMAND_CORE_OBF_MAP_SECTION_INFO_P_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_INFO_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QString>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <MapTypes.h>

namespace OsmAnd {

    class ObfMapSectionLevel;
    struct ObfMapSectionDecodingEncodingRules;
    class ObfMapSectionReader_P;

    class ObfMapSectionLevelTreeNode
    {
    private:
    protected:
        ObfMapSectionLevelTreeNode(const std::shared_ptr<const ObfMapSectionLevel>& level);

        uint32_t _offset;
        uint32_t _length;
        uint32_t _childrenInnerOffset;
        uint32_t _dataOffset;
        MapFoundationType _foundation;
        AreaI _area31;
    public:
        ~ObfMapSectionLevelTreeNode();

        const std::shared_ptr<const ObfMapSectionLevel> level;

    friend class OsmAnd::ObfMapSectionReader_P;
    };

    class ObfMapSectionLevel_P
    {
    private:
    protected:
        ObfMapSectionLevel_P(ObfMapSectionLevel* owner);

        ObfMapSectionLevel* const owner;

        mutable QMutex _rootNodesMutex;
        struct RootNodes
        {
            QList< std::shared_ptr<ObfMapSectionLevelTreeNode> > nodes;
        };
        std::shared_ptr<RootNodes> _rootNodes;
    public:
        virtual ~ObfMapSectionLevel_P();

    friend class OsmAnd::ObfMapSectionLevel;
    friend class OsmAnd::ObfMapSectionReader_P;
    };

    class ObfMapSectionInfo;
    class ObfMapSectionInfo_P
    {
    private:
    protected:
        ObfMapSectionInfo_P(ObfMapSectionInfo* owner);

        ObfMapSectionInfo* const owner;

        mutable QMutex _encodingDecodingDataMutex;
        std::shared_ptr<const ObfMapSectionDecodingEncodingRules> _encodingDecodingRules;
    public:
        virtual ~ObfMapSectionInfo_P();

    friend class OsmAnd::ObfMapSectionInfo;
    friend class OsmAnd::ObfMapSectionReader_P;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_INFO_P_H_)
