#ifndef _OSMAND_CORE_GEO_BAND_SETTINGS_H_
#define _OSMAND_CORE_GEO_BAND_SETTINGS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QList>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API GeoBandSettings
    {
        Q_DISABLE_COPY_AND_MOVE(GeoBandSettings);
    private:
    protected:
    public:
        GeoBandSettings(
            const QString& unit,
            const QString& unitFormatGeneral,
            const QString& unitFormatPrecise,
            const QString& internalUnit,
            const float opacity,
            const QString& colorProfilePath,
            const QString& contourStyleName,
            const QHash<ZoomLevel, QList<double>>& contourLevels,
            const QHash<ZoomLevel, QStringList>& contourTypes);
        virtual ~GeoBandSettings();

        const QString unit;
        const QString unitFormatGeneral;
        const QString unitFormatPrecise;
        const QString internalUnit;
        const float opacity;
        const QString colorProfilePath;
        const QString contourStyleName;
        const QHash<ZoomLevel, QList<double>> contourLevels;
        const QHash<ZoomLevel, QStringList> contourTypes;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_BAND_SETTINGS_H_)
