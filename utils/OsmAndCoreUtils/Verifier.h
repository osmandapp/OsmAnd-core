#ifndef _OSMAND_CORE_UTILS_VERIFIER_H_
#define _OSMAND_CORE_UTILS_VERIFIER_H_

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <OsmAndCoreUtils.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace Verifier
    {
        struct OSMAND_CORE_UTILS_API Configuration
        {
            Configuration();

            enum class Action
            {
                Invalid = -1,

                UniqueMapObjectIds
            };

            QStringList obfDirs;
            QStringList obfFiles;
            Action action;
        };
        OSMAND_CORE_UTILS_API bool OSMAND_CORE_UTILS_CALL parseCommandLineArguments(const QStringList& cmdLineArgs, Configuration& cfg, QString& error);
        OSMAND_CORE_UTILS_API void OSMAND_CORE_UTILS_CALL dumpToStdOut(const Configuration& cfg);
        OSMAND_CORE_UTILS_API QString OSMAND_CORE_UTILS_CALL dumpToString(const Configuration& cfg);
    }
}

#endif // !defined(_OSMAND_CORE_UTILS_VERIFIER_H_)
