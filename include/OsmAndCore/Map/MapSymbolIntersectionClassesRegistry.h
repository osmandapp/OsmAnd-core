#ifndef _OSMAND_CORE_MAP_SYMBOL_INTERSECTION_CLASSES_REGISTRY_H_
#define _OSMAND_CORE_MAP_SYMBOL_INTERSECTION_CLASSES_REGISTRY_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    class MapSymbolIntersectionClassesRegistry_P;
    class OSMAND_CORE_API MapSymbolIntersectionClassesRegistry
    {
        Q_DISABLE_COPY_AND_MOVE(MapSymbolIntersectionClassesRegistry)
    public:
        typedef MapSymbolIntersectionClassId ClassId;

    private:
        PrivateImplementation<MapSymbolIntersectionClassesRegistry_P> _p;
    protected:
    public:
        MapSymbolIntersectionClassesRegistry();
        virtual ~MapSymbolIntersectionClassesRegistry();

        static MapSymbolIntersectionClassesRegistry& globalInstance();

        QString getClassNameById(const ClassId classId) const;
        ClassId getOrRegisterClassIdByName(const QString& className);

        // Predefined 'any' class
        const ClassId& anyClass;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_INTERSECTION_CLASSES_REGISTRY_H_)
