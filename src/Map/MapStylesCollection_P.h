#ifndef _OSMAND_CORE_MAP_STYLES_COLLECTION_P_H_
#define _OSMAND_CORE_MAP_STYLES_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QString>
#include <QHash>
#include <QList>
#include <QReadWriteLock>
#include <QMutex>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class UnresolvedMapStyle;
    class ResolvedMapStyle;

    class MapStylesCollection;
    class MapStylesCollection_P Q_DECL_FINAL
    {
    private:
        static QString getFullyQualifiedStyleName(const QString& name);

        QHash< QString, std::shared_ptr<UnresolvedMapStyle> > _styles;
        mutable QReadWriteLock _stylesLock;

        std::shared_ptr<UnresolvedMapStyle> getEditableStyleByName_noSync(const QString& name) const;
        QList<std::shared_ptr<UnresolvedMapStyle>> getEditableStyleAddons_noSync() const;

        mutable QHash< QString, std::shared_ptr<const ResolvedMapStyle> > _resolvedStyles;
        mutable QMutex _resolvedStylesLock;

        bool addStyleFromCoreResource(const QString& resourceName);
    protected:
        MapStylesCollection_P(MapStylesCollection* owner);
    public:
        virtual ~MapStylesCollection_P();

        ImplementationInterface<MapStylesCollection> owner;

        bool addStyleFromFile(const QString& filePath, const bool doNotReplace);
        bool addStyleFromByteArray(const QByteArray& data, const QString& name, const bool doNotReplace);

        QList< std::shared_ptr<const UnresolvedMapStyle> > getCollection() const;
        std::shared_ptr<const UnresolvedMapStyle> getStyleByName(const QString& name) const;
        virtual std::shared_ptr<const ResolvedMapStyle> getResolvedStyleByName(const QString& name) const;

    friend class OsmAnd::MapStylesCollection;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_COLLECTION_P_H_)
