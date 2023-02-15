#ifndef _OSMAND_CORE_ROAD_H_
#define _OSMAND_CORE_ROAD_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfMapObject.h>
#include <OsmAndCore/Data/ObfRoutingSectionInfo.h>

namespace OsmAnd
{
    class ObfRoutingSectionReader_P;

    enum class RoadDirection : int32_t
    {
        OneWayForward = -1,
        TwoWay = 0,
        OneWayReverse = +1
    };

    enum class RoadRestriction : int32_t
    {
        Special_ReverseWayOnly = -1,

        Invalid = 0,

        NoRightTurn = 1,
        NoLeftTurn = 2,
        NoUTurn = 3,
        NoStraightOn = 4,
        OnlyRightTurn = 5,
        OnlyLeftTurn = 6,
        OnlyStraightOn = 7,
    };

    class OSMAND_CORE_API Road Q_DECL_FINAL : public ObfMapObject
    {
        Q_DISABLE_COPY_AND_MOVE(Road);
    private:
        static const int HEIGHT_UNDEFINED = -80000;
        QString getValue(uint32_t pnt, const QString & tag) const;
    protected:
        Road(const std::shared_ptr<const ObfRoutingSectionInfo>& section);
    public:
        virtual ~Road();

        // General information
        const std::shared_ptr<const ObfRoutingSectionInfo> section;

        // Road information
        QHash< uint32_t, QVector<uint32_t> > pointsTypes;
        QHash< ObfObjectId, RoadRestriction > restrictions;

        QString getRefInNativeLanguage() const;
        QString getRefInLanguage(const QString& lang) const;
        QString getRef(const QString lang, bool transliterate) const;
        QString getDestinationRef(bool direction) const;
        QString getDestinationName(const QString lang, bool transliterate, bool direction) const;

        float getMaximumSpeed(bool direction) const;
        bool isDeleted() const;

        double directionRoute(int startPoint, bool plus) const;
        double directionRoute(int startPoint, bool plus, float dist) const;
        bool bearingVsRouteDirection(double bearing) const;

        const bool hasGeocodingAccess() const;
        QVector<double> calculateHeightArray() const;

    friend class OsmAnd::ObfRoutingSectionReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_ROAD_H_)
