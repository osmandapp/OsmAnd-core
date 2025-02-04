#ifndef _OSMAND_CORE_ARABIC_NORMALIZER_H
#define _OSMAND_CORE_ARABIC_NORMALIZER_H

#include <QString>

namespace OsmAnd
{
    class ArabicNormalizer {
    public:
        static bool isSpecialArabic(const QString& text);
        static QString normalize(const QString& text);

    private:
        static QString replaceDigits(const QString& text);
        static bool isDiacritic(QChar c);
        static bool isArabicDigit(QChar c);
        static bool isKashida(QChar c);
        static bool isArabicCharacter(QChar c);
    };
}

#endif // _OSMAND_CORE_ARABIC_NORMALIZER_H