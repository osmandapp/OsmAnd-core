#ifndef _OSMAND_CORE_MAP_STYLE_CONFIGURABLE_INPUT_VALUE_H_
#define _OSMAND_CORE_MAP_STYLE_CONFIGURABLE_INPUT_VALUE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyleValueDefinition.h>

namespace OsmAnd
{
    class MapStyle_P;

    class OSMAND_CORE_API MapStyleConfigurableInputValue : public MapStyleValueDefinition
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyleConfigurableInputValue);
    private:
    protected:
        MapStyleConfigurableInputValue(const MapStyleValueDataType dataType, const QString& name, const QString& title, const QString& description, const QStringList& possibleValues);
    public:
        virtual ~MapStyleConfigurableInputValue();

        const QString title;
        const QString description;
        const QStringList possibleValues;

    friend class OsmAnd::MapStyle_P;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_STYLE_CONFIGURABLE_INPUT_VALUE_H_)
