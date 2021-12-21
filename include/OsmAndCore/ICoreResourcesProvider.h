#ifndef _OSMAND_CORE_I_CORE_RESOURCES_PROVIDER_H_
#define _OSMAND_CORE_I_CORE_RESOURCES_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/MemoryCommon.h>

namespace OsmAnd
{
	class OSMAND_CORE_API ICoreResourcesProvider
    {
        Q_DISABLE_COPY_AND_MOVE(ICoreResourcesProvider);

    protected:
        ICoreResourcesProvider();
    public:
        virtual ~ICoreResourcesProvider();

        virtual QByteArray getResource(
            const QString& name,
            const float displayDensityFactor,
            bool* ok = nullptr) const = 0;
        virtual QByteArray getResource(
            const QString& name,
            bool* ok = nullptr) const = 0;

        virtual sk_sp<SkImage> getResourceAsImage(
            const QString& name,
            const float displayDensityFactor) const;
        virtual sk_sp<SkImage> getResourceAsImage(
            const QString& name) const;

        virtual bool containsResource(
            const QString& name,
            const float displayDensityFactor) const = 0;
        virtual bool containsResource(
            const QString& name) const = 0;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(ICoreResourcesProvider);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QByteArray,
            getResource,
            const QString& name,
            const float displayDensityFactor,
            bool* ok);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            QByteArray,
            getResource,
            const QString& name,
            bool* ok);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            bool,
            containsResource,
            const QString& name,
            const float displayDensityFactor);
        SWIG_EMIT_DIRECTOR_CONST_METHOD(
            bool,
            containsResource,
            const QString& name);
    SWIG_EMIT_DIRECTOR_END(ICoreResourcesProvider);
}

#endif // !defined(_OSMAND_CORE_I_CORE_RESOURCES_PROVIDER_H_)
