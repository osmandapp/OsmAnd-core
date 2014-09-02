#ifndef _OSMAND_CORE_TOOLS_UTILITIES_H_
#define _OSMAND_CORE_TOOLS_UTILITIES_H_

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

namespace OsmAndTools
{
    struct OSMAND_CORE_API Utilities Q_DECL_FINAL
    {
        static QString resolvePath(const QString& input);
        static QString purifyArgumentValue(const QString& input);
    };
}

#endif // !defined(_OSMAND_CORE_TOOLS_UTILITIES_H_)
