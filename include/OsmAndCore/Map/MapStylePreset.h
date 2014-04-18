#ifndef _OSMAND_CORE_MAP_STYLE_PRESET_H_
#define _OSMAND_CORE_MAP_STYLE_PRESET_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API MapStylePreset
    {
        Q_DISABLE_COPY(MapStylePreset);
    private:
    protected:
    public:
        MapStylePreset(const QString& name, const QString& styleName);
        virtual ~MapStylePreset();

        const QString name;
        const QString styleName;
        QHash<QString, QString> attributes;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_PRESET_H_)
