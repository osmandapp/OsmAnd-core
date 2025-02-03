#include "ArabicNormalizer.h"

// Arabic-specific character ranges
static const QString ARABIC_DIACRITICS = QString::fromUtf16(u"\u064B\u064C\u064D\u064E\u064F\u0650\u0651\u0652");
static const QString ARABIC_DIGITS = QString::fromUtf16(u"٠١٢٣٤٥٦٧٨٩");
static const QString DIGITS_REPLACEMENT = QStringLiteral("0123456789");
static const QChar KASHIDA = QChar(0x0640);  // Arabic Tatweel (Kashida)

bool OsmAnd::ArabicNormalizer::isSpecialArabic(const QString& text) {
    if (text.isEmpty()) {
        return false;
    }

    QChar firstChar = text.at(0);
    if (isArabicCharacter(firstChar)) {
        for (const QChar& c : text) {
            if (isDiacritic(c) || isArabicDigit(c) || isKashida(c)) {
                return true;
            }
        }
    }
    
    return false;
}

QString OsmAnd::ArabicNormalizer::normalize(const QString& text) {
    if (text.isEmpty()) {
        return text;
    }

    QString result = text;
    
    // Remove diacritics
    for (const QChar& diacritic : ARABIC_DIACRITICS) {
        result.remove(diacritic);
    }

    // Remove Kashida
    result.remove(KASHIDA);

    return replaceDigits(result);
}

QString OsmAnd::ArabicNormalizer::replaceDigits(const QString& text) {
    if (text.isEmpty()) {
        return text;
    }

    QChar firstChar = text.at(0);
    if (!isArabicCharacter(firstChar)) {
        return text;
    }

    QString result = text;
    for (int i = 0; i < ARABIC_DIGITS.length(); ++i) {
        result.replace(ARABIC_DIGITS.at(i), DIGITS_REPLACEMENT.at(i));
    }

    return result;
}

bool OsmAnd::ArabicNormalizer::isDiacritic(QChar c) {
    return ARABIC_DIACRITICS.contains(c);
}

bool OsmAnd::ArabicNormalizer::isArabicDigit(QChar c) {
    return (c >= QChar(0x0660) && c <= QChar(0x0669));  // Arabic-Indic digits
}

bool OsmAnd::ArabicNormalizer::isKashida(QChar c) {
    return (c == KASHIDA);
}

bool OsmAnd::ArabicNormalizer::isArabicCharacter(QChar c) {
    return (c.unicode() >= 0x0600 && c.unicode() <= 0x06FF);  // Arabic Unicode block
}