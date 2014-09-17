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
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Data/Model/ObjectWithId.h>

namespace OsmAnd
{
    class ObfMapSectionInfo;
    class ObfMapSectionLevel;
    class ObfMapSectionReader_P;
    class Primitiviser_P;

    namespace Model
    {
        class OSMAND_CORE_API BinaryMapObject Q_DECL_FINAL : public ObjectWithId
        {
            Q_DISABLE_COPY_AND_MOVE(BinaryMapObject);
        private:
        protected:
            BinaryMapObject(const std::shared_ptr<const ObfMapSectionInfo>& section, const std::shared_ptr<const ObfMapSectionLevel>& level);

            bool _isArea;
            QVector< PointI > _points31;
            QList< QVector< PointI > > _innerPolygonsPoints31;
            QVector< uint32_t > _typesRuleIds;
            QVector< uint32_t > _extraTypesRuleIds;
            QHash< uint32_t, QString > _captions;
            QList<uint32_t> _captionsOrder;
            AreaI _bbox31;
        public:
            virtual ~BinaryMapObject();

            const std::shared_ptr<const ObfMapSectionInfo> section;
            const std::shared_ptr<const ObfMapSectionLevel> level;

            const bool& isArea;
            const QVector< PointI >& points31;
            const QList< QVector< PointI > >& innerPolygonsPoints31;
            const QVector< uint32_t >& typesRuleIds;
            const QVector< uint32_t >& extraTypesRuleIds;
            const QHash<uint32_t, QString>& captions;
            const QList<uint32_t>& captionsOrder;
            const AreaI& bbox31;

            int getSimpleLayerValue() const;
            bool isClosedFigure(bool checkInner = false) const;

            bool containsType(const uint32_t typeRuleId, bool checkAdditional = false) const;
            bool containsTypeSlow(const QString& tag, const QString& value, bool checkAdditional = false) const;

            bool intersects(const AreaI& area) const;

            QString getNameInNativeLanguage() const;
            QString getNameInLanguage(const QString& lang) const;

            static uint64_t getUniqueId(const std::shared_ptr<const BinaryMapObject>& mapObject);
            static uint64_t getUniqueId(const uint64_t id, const std::shared_ptr<const ObfMapSectionInfo>& section);

        friend class OsmAnd::ObfMapSectionReader_P;
        friend class OsmAnd::Primitiviser_P;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_OBJECT_H_)
