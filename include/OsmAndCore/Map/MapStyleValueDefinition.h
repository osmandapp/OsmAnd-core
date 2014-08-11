#ifndef _OSMAND_CORE_MAP_STYLE_VALUE_DEFINITION_H_
#define _OSMAND_CORE_MAP_STYLE_VALUE_DEFINITION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QAtomicInt>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyle_P;
    class MapStyleBuiltinValueDefinitions;

    enum class MapStyleValueDataType
    {
        Boolean,
        Integer,
        Float,
        String,
        Color,
    };

    enum class MapStyleValueClass
    {
        Input,
        Output,
    };

    class OSMAND_CORE_API MapStyleValueDefinition
    {
        Q_DISABLE_COPY_AND_MOVE(MapStyleValueDefinition);
    private:
        static QAtomicInt _nextId;
    protected:
        MapStyleValueDefinition(const MapStyleValueClass valueClass, const MapStyleValueDataType dataType, const QString& name, const bool isComplex);
    public:
        virtual ~MapStyleValueDefinition();

        // This id is generated in runtime
        const int id;

        const MapStyleValueClass valueClass;
        const MapStyleValueDataType dataType;
        const QString name;
        const bool isComplex;

    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleBuiltinValueDefinitions;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_STYLE_VALUE_DEFINITION_H_)
