#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_P_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_INFO_P_H_

#include "stdlib_common.h"
#include <array>

#include "QtExtensions.h"
#include <QString>
#include <QMutex>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "ObfRoutingSectionInfo.h"

namespace OsmAnd
{
    class ObfRoutingSectionReader_P;
    class ObfRoutingSectionLevel;

    class ObfRoutingSectionInfo;
    class ObfRoutingSectionInfo_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY(ObfRoutingSectionInfo_P)
    private:
    protected:
        ObfRoutingSectionInfo_P(ObfRoutingSectionInfo* owner);

        mutable QMutex _decodingRulesMutex;
        mutable std::shared_ptr<const ObfRoutingSectionDecodingRules> _decodingRules;

        struct LevelContainer
        {
            QMutex mutex;
            std::shared_ptr<const ObfRoutingSectionLevel> level;
        };
        mutable std::array<LevelContainer, RoutingDataLevelsCount> _levelContainers;
    public:
        ~ObfRoutingSectionInfo_P();

        ImplementationInterface<ObfRoutingSectionInfo> owner;

    friend class OsmAnd::ObfRoutingSectionInfo;
    friend class OsmAnd::ObfRoutingSectionReader_P;
    };

    class ObfRoutingSectionLevelTreeNode;
    class ObfRoutingSectionLevel;
    class ObfRoutingSectionLevel_P Q_DECL_FINAL
    {
    private:
    protected:
        ObfRoutingSectionLevel_P(ObfRoutingSectionLevel* const owner);

        QList< std::shared_ptr<const ObfRoutingSectionLevelTreeNode> > _rootNodes;
    public:
        ~ObfRoutingSectionLevel_P();

        ImplementationInterface<ObfRoutingSectionLevel> owner;

    friend class OsmAnd::ObfRoutingSectionLevel;
    friend class OsmAnd::ObfRoutingSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_INFO_P_H_)
