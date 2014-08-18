#include "MapSymbolIntersectionClassesRegistry.h"
#include "MapSymbolIntersectionClassesRegistry_P.h"

#include "MapSymbolIntersectionClassesRegistry_private.h"

OsmAnd::MapSymbolIntersectionClassesRegistry::MapSymbolIntersectionClassesRegistry()
    : _p(new MapSymbolIntersectionClassesRegistry_P(this))
    , anyClass(_p->anyClass)
{
}

OsmAnd::MapSymbolIntersectionClassesRegistry::~MapSymbolIntersectionClassesRegistry()
{
}

static std::shared_ptr<OsmAnd::MapSymbolIntersectionClassesRegistry> s_globalMapSymbolIntersectionClassesRegistry;
OsmAnd::MapSymbolIntersectionClassesRegistry& OsmAnd::MapSymbolIntersectionClassesRegistry::globalInstance()
{
    return *s_globalMapSymbolIntersectionClassesRegistry;
}

QString OsmAnd::MapSymbolIntersectionClassesRegistry::getClassNameById(const ClassId classId) const
{
    return _p->getClassNameById(classId);
}

OsmAnd::MapSymbolIntersectionClassesRegistry::ClassId OsmAnd::MapSymbolIntersectionClassesRegistry::getOrRegisterClassIdByName(const QString& className)
{
    return _p->getOrRegisterClassIdByName(className);
}

void OsmAnd::MapSymbolIntersectionClassesRegistry_initializeGlobalInstance()
{
    s_globalMapSymbolIntersectionClassesRegistry.reset(new MapSymbolIntersectionClassesRegistry());
}

void OsmAnd::MapSymbolIntersectionClassesRegistry_releaseGlobalInstance()
{
    s_globalMapSymbolIntersectionClassesRegistry.reset();
}
