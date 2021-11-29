#include "MapSymbolIntersectionClassesRegistry_P.h"
#include "MapSymbolIntersectionClassesRegistry.h"

OsmAnd::MapSymbolIntersectionClassesRegistry_P::MapSymbolIntersectionClassesRegistry_P(MapSymbolIntersectionClassesRegistry* const owner_)
    : owner(owner_)
    , anyClass(unsafeRegisterClassIdByName(QLatin1String("any")))
{
}

OsmAnd::MapSymbolIntersectionClassesRegistry_P::~MapSymbolIntersectionClassesRegistry_P()
{
}

QString OsmAnd::MapSymbolIntersectionClassesRegistry_P::getClassNameById(const ClassId classId) const
{
    QReadLocker scopedLocker(&_lock);

    if (_nameById.size() <= classId)
        return {};
    return _nameById[classId];
}

OsmAnd::MapSymbolIntersectionClassesRegistry_P::ClassId OsmAnd::MapSymbolIntersectionClassesRegistry_P::getOrRegisterClassIdByName(const QString& className)
{
    // First try to find already-registered class
    {
        QReadLocker scopedLocker(&_lock);

        const auto citId = _idByName.constFind(className);
        if (citId != _idByName.cend())
            return *citId;
    }

    // Otherwise, create (but also perform second check)
    {
        QWriteLocker scopedLocker(&_lock);

        const auto citId = _idByName.constFind(className);
        if (citId != _idByName.cend())
            return *citId;

        return unsafeRegisterClassIdByName(className);
    }
}

OsmAnd::MapSymbolIntersectionClassesRegistry_P::ClassId OsmAnd::MapSymbolIntersectionClassesRegistry_P::unsafeRegisterClassIdByName(const QString& className)
{
    const ClassId newId = _nameById.size();
    _nameById.push_back(className);
    _idByName[className] = newId;
    return newId;
}
