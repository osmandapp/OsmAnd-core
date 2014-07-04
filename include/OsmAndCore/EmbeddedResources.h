#ifndef _OSMAND_CORE_EMBEDDED_RESOURCES_H_
#define _OSMAND_CORE_EMBEDDED_RESOURCES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QByteArray>

#include <SkBitmap.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API EmbeddedResources
    {
    private:
        EmbeddedResources();
    protected:
    public:
        virtual ~EmbeddedResources();

        static QByteArray decompressResource(const QString& name, bool* ok = nullptr);
        static QByteArray getRawResource(const QString& name, bool* ok = nullptr);
        static std::shared_ptr<SkBitmap> getBitmapResource(const QString& name, bool* ok = nullptr);
        static bool containsResource(const QString& name);
    };
}

#endif // !defined(_OSMAND_CORE_EMBEDDED_RESOURCES_H_)
