#ifndef _OSMAND_CORE_OBF_MAP_SECTION_INFO_P_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_INFO_P_H_

#include "stdlib_common.h"
#include <atomic>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMutex>
#include <QReadWriteLock>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QString>
#include <QAtomicInt>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapCommonTypes.h"

namespace OsmAnd
{
    class ObfMapSectionLevel;
    class ObfMapSectionAttributeMapping;
    class ObfMapSectionReader_P;

    class ObfMapSectionLevelTreeNode
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapSectionLevelTreeNode);
    private:
    protected:
        ObfMapSectionLevelTreeNode(const std::shared_ptr<const ObfMapSectionLevel>& level);
    public:
        ~ObfMapSectionLevelTreeNode();

        const std::shared_ptr<const ObfMapSectionLevel> level;

        uint32_t offset;
        uint32_t length;
        uint32_t dataOffset;
        uint32_t depth;
        MapSurfaceType surfaceType;
        AreaI area31;

        bool hasChildrenDataBoxes;
        mutable QList<std::shared_ptr<const ObfMapSectionLevelTreeNode>> children;
        uint32_t firstDataBoxInnerOffset;

    friend class OsmAnd::ObfMapSectionReader_P;
    };

    class ObfMapSectionLevel_P Q_DECL_FINAL
    {
    private:
    protected:
        ObfMapSectionLevel_P(ObfMapSectionLevel* owner);

        mutable std::shared_ptr< const QList< std::shared_ptr<const ObfMapSectionLevelTreeNode> > > _rootNodes;
        mutable QAtomicInt _rootNodesLoaded;
        mutable QMutex _rootNodesLoadMutex;
        mutable QReadWriteLock _childNodeAccessMutex;
    public:
        virtual ~ObfMapSectionLevel_P();

        ImplementationInterface<ObfMapSectionLevel> owner;

    friend class OsmAnd::ObfMapSectionLevel;
    friend class OsmAnd::ObfMapSectionReader_P;
    };

    class ObfMapSectionInfo;
    class ObfMapSectionInfo_P Q_DECL_FINAL
    {
    private:
    protected:
        ObfMapSectionInfo_P(ObfMapSectionInfo* owner);

        mutable std::shared_ptr<ObfMapSectionAttributeMapping> _attributeMapping;
        mutable QAtomicInt _attributeMappingLoaded;
        mutable QMutex _attributeMappingLoadMutex;
    public:
        virtual ~ObfMapSectionInfo_P();

        ImplementationInterface<ObfMapSectionInfo> owner;

        std::shared_ptr<const ObfMapSectionAttributeMapping> getAttributeMapping() const;

    friend class OsmAnd::ObfMapSectionInfo;
    friend class OsmAnd::ObfMapSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_INFO_P_H_)
