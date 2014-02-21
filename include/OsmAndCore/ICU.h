#ifndef _OSMAND_CORE_ICU_H_
#define _OSMAND_CORE_ICU_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace ICU
    {
        OSMAND_CORE_API QString OSMAND_CORE_CALL convertToVisualOrder(const QString& input);
        OSMAND_CORE_API QString OSMAND_CORE_CALL transliterateToLatin(const QString& input);
    }
}

#endif // !defined(_OSMAND_CORE_ICU_H_)
