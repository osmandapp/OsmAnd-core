#ifndef _OSMAND_CORE_MAP_SYMBOL_INTERSECTION_CLASSES_REGISTRY_P_H_
#define _OSMAND_CORE_MAP_SYMBOL_INTERSECTION_CLASSES_REGISTRY_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QReadWriteLock>
#include <QHash>
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapSymbolIntersectionClassesRegistry.h"

namespace OsmAnd
{
    class MapSymbolIntersectionClassesRegistry_P Q_DECL_FINAL
    {
    public:
        typedef MapSymbolIntersectionClassesRegistry::ClassId ClassId;

    private:
        mutable QReadWriteLock _lock;
        QHash<QString, int> _idByName;
        QList<QString> _nameById;

        ClassId unsafeRegisterClassIdByName(const QString& className);
    protected:
        MapSymbolIntersectionClassesRegistry_P(MapSymbolIntersectionClassesRegistry* const owner);
    public:
        ~MapSymbolIntersectionClassesRegistry_P();

        ImplementationInterface<MapSymbolIntersectionClassesRegistry> owner;

        QString getClassNameById(const ClassId classId) const;
        ClassId getOrRegisterClassIdByName(const QString& className);

        // Predefined 'any' class
        const ClassId anyClass;

        // Predefined 'anyExceptOwnGroup' class
        const ClassId anyExceptOwnGroupClass;

        // Predefined 'ownGroup' class
        const ClassId ownGroupClass;

    friend class OsmAnd::MapSymbolIntersectionClassesRegistry;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_INTERSECTION_CLASSES_REGISTRY_P_H_)
