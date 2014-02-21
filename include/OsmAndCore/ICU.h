#ifndef _OSMAND_CORE_ICU_H_
#define _OSMAND_CORE_ICU_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>
#include <QVector>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace ICU
    {
        OSMAND_CORE_API QString OSMAND_CORE_CALL convertToVisualOrder(const QString& input);
        OSMAND_CORE_API QString OSMAND_CORE_CALL transliterateToLatin(const QString& input);
        OSMAND_CORE_API QVector<int> OSMAND_CORE_CALL getTextWrapping(const QString& input, const int maxCharsPerLine);
        OSMAND_CORE_API QVector<QStringRef> OSMAND_CORE_CALL getTextWrappingRefs(const QString& input, const int maxCharsPerLine);
        OSMAND_CORE_API QStringList OSMAND_CORE_CALL wrapText(const QString& input, const int maxCharsPerLine);
    }
}

#endif // !defined(_OSMAND_CORE_ICU_H_)
