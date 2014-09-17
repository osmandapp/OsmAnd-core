#include "MapStyleBuiltinValueDefinitions.h"

#include "QtExtensions.h"

#include "MapStyleValueDefinition.h"

OsmAnd::MapStyleBuiltinValueDefinitions::MapStyleBuiltinValueDefinitions()
    : _runtimeGeneratedId(0)
    // Definitions:
#   define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex)         \
        , varname(new MapStyleBuiltinValueDefinition(                                       \
            _runtimeGeneratedId++,                                                          \
            OsmAnd::MapStyleValueDefinition::Class::valueClass,                             \
            OsmAnd::MapStyleValueDataType::dataType,                                        \
            QLatin1String(name),                                                            \
            isComplex))
#   include "MapStyleBuiltinValueDefinitions_Set.h"
#   undef DECLARE_BUILTIN_VALUEDEF
    // Identifiers of definitions above:
#   define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex)         \
        , id_##varname(varname->id)
#   include "MapStyleBuiltinValueDefinitions_Set.h"
#   undef DECLARE_BUILTIN_VALUEDEF
    , lastBuiltinValueDefinitionId(_runtimeGeneratedId - 1)
{
}

OsmAnd::MapStyleBuiltinValueDefinitions::~MapStyleBuiltinValueDefinitions()
{
}

QAtomicInt OsmAnd::MapStyleBuiltinValueDefinitions::s_instanceFlag;
QMutex OsmAnd::MapStyleBuiltinValueDefinitions::s_instanceMutex;
std::shared_ptr<const OsmAnd::MapStyleBuiltinValueDefinitions> OsmAnd::MapStyleBuiltinValueDefinitions::s_instance;

std::shared_ptr<const OsmAnd::MapStyleBuiltinValueDefinitions> OsmAnd::MapStyleBuiltinValueDefinitions::get()
{
    if (s_instanceFlag.fetchAndAddOrdered(0) != 0)
    {
        return s_instance;
    }
    else
    {
        QMutexLocker scopedLocker(&s_instanceMutex);

        if (!s_instance)
        {
            s_instance.reset(new OsmAnd::MapStyleBuiltinValueDefinitions());
            s_instanceFlag.fetchAndStoreOrdered(1);
        }

        return s_instance;
    }
}

OsmAnd::MapStyleBuiltinValueDefinitions::MapStyleBuiltinValueDefinition::MapStyleBuiltinValueDefinition(
    const int id_,
    const MapStyleValueDefinition::Class valueClass_,
    const MapStyleValueDataType dataType_,
    const QString& name_,
    const bool isComplex_)
    : MapStyleValueDefinition(valueClass_, dataType_, name_, isComplex_)
    , id(id_)
{
}

OsmAnd::MapStyleBuiltinValueDefinitions::MapStyleBuiltinValueDefinition::~MapStyleBuiltinValueDefinition()
{
}
