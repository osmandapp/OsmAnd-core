#ifndef _OSMAND_CORE_I_MAP_STYLES_COLLECTION_H_
#define _OSMAND_CORE_I_MAP_STYLES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class UnresolvedMapStyle;
    class ResolvedMapStyle;

    class OSMAND_CORE_API IMapStylesCollection
    {
        Q_DISABLE_COPY_AND_MOVE(IMapStylesCollection);
    private:
    protected:
        IMapStylesCollection();
    public:
        virtual ~IMapStylesCollection();

        virtual QList< std::shared_ptr<const UnresolvedMapStyle> > getCollection() const = 0;
        virtual std::shared_ptr<const UnresolvedMapStyle> getStyleByName(const QString& name) const = 0;
        virtual std::shared_ptr<const ResolvedMapStyle> getResolvedStyleByName(const QString& name) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_STYLES_COLLECTION_H_)
