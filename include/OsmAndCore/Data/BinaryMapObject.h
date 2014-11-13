#ifndef _OSMAND_CORE_BINARY_MAP_OBJECT_H_
#define _OSMAND_CORE_BINARY_MAP_OBJECT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QVector>
#include <QString>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/MapObject.h>

namespace OsmAnd
{
    class ObfMapSectionInfo;
    class ObfMapSectionLevel;
    class ObfMapSectionReader_P;

    class OSMAND_CORE_API BinaryMapObject Q_DECL_FINAL : public MapObject
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapObject);
    private:
    protected:
        BinaryMapObject(
            const std::shared_ptr<const ObfMapSectionInfo>& section,
            const std::shared_ptr<const ObfMapSectionLevel>& level);
    public:
        virtual ~BinaryMapObject();

        // General information
        ObfObjectId id;
        const std::shared_ptr<const ObfMapSectionInfo> section;
        const std::shared_ptr<const ObfMapSectionLevel> level;
        virtual bool obtainSharingKey(SharingKey& outKey) const;
        virtual bool obtainSortingKey(SortingKey& outKey) const;
        virtual ZoomLevel getMinZoomLevel() const;
        virtual ZoomLevel getMaxZoomLevel() const;
        virtual QString toString() const;

        // Layers
        virtual LayerType getLayerType() const;

        friend class OsmAnd::ObfMapSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_OBJECT_H_)
