#ifndef _OSMAND_CORE_ICONS_CACHE_H_
#define _OSMAND_CORE_ICONS_CACHE_H_

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMutex>
#include <QReadWriteLock>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImage.h>
#include "restore_internal_warnings.h"

namespace OsmAnd
{
    class IconsProvider Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(IconsProvider);    

    private:
        mutable QMutex _mutex;
        mutable QHash< QString, sk_sp<const SkImage> > _cache;

        bool obtainIcon(const QString& name, const float scale, const bool colorable, sk_sp<const SkImage>& outIcon) const;
        bool obtainResourceByPath(const QString& path, QByteArray& outResource) const;
        bool containsResource(const QString& name, bool colorable) const;
        QString makeIconKey(const QString& name, const float scale, const bool colorable) const;
        QString makeIconPath(const QString& name, const bool colorable) const;

    public:
        IconsProvider(
            const QString& pathFormat_,
            const std::shared_ptr<const ICoreResourcesProvider>& externalResourcesProvider_,
            const float displayDensityFactor_);
        virtual ~IconsProvider();

        const QString pathFormat;
        const std::shared_ptr<const ICoreResourcesProvider> externalResourcesProvider;
        const float displayDensityFactor;

        bool obtainIcon(
            const QString& name,
            const float scale,
            sk_sp<const SkImage>& outIcon,
            bool* const outColorable = nullptr) const;
        bool containsResource(const QString& name) const;
    };
}

#endif // !defined(_OSMAND_CORE_ICONS_CACHE_H_)