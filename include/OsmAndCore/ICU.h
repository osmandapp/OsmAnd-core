#ifndef _OSMAND_CORE_ICU_H_
#define _OSMAND_CORE_ICU_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QStringList>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    namespace ICU
    {
        enum TextDirection {
            LTR,
            RTL,
            MIXED,
            NEUTRAL
        };
        OSMAND_CORE_API QString OSMAND_CORE_CALL convertToVisualOrder(const QString& input);
        OSMAND_CORE_API TextDirection OSMAND_CORE_CALL getTextDirection(const QString& input);
        OSMAND_CORE_API QString OSMAND_CORE_CALL transliterateToLatin(
            const QString& input,
            const bool keepAccentsAndDiacriticsInInput = true,
            const bool keepAccentsAndDiacriticsInOutput = true);
        OSMAND_CORE_API QVector<int> OSMAND_CORE_CALL getTextWrapping(const QString& input, const int maxCharsPerLine, const int maxLines = 0);
        OSMAND_CORE_API QVector<QStringRef> OSMAND_CORE_CALL getTextWrappingRefs(const QString& input, const int maxCharsPerLine, const int maxLines = 0);
        OSMAND_CORE_API QStringList OSMAND_CORE_CALL wrapText(const QString& input, const int maxCharsPerLine, const int maxLines = 0);
        OSMAND_CORE_API QString OSMAND_CORE_CALL stripAccentsAndDiacritics(const QString& input);
        OSMAND_CORE_API bool OSMAND_CORE_CALL cmatches(const QString& _fullName, const QString& _part, StringMatcherMode _mode);
        OSMAND_CORE_API bool OSMAND_CORE_CALL ccontains(const QString& _base, const QString& _part);
        OSMAND_CORE_API bool OSMAND_CORE_CALL cstartsWith(const QString& _searchInParam, const QString& _theStart,
                                bool checkBeginning, bool checkSpaces, bool equals);
        OSMAND_CORE_API int OSMAND_CORE_CALL ccompare(const QString& _base, const QString& _part);
    }
}

#endif // !defined(_OSMAND_CORE_ICU_H_)
