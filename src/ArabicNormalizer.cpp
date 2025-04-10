#include "ArabicNormalizer.h"
#include <QMap>
#include <QRegularExpression>

// Arabic-specific character ranges
static const QString ARABIC_DIGITS = QString::fromUtf16(u"٠١٢٣٤٥٦٧٨٩");
static const QString DIGITS_REPLACEMENT = QStringLiteral("0123456789");

static const QVector<QString> ARABIC_DIACRITIC_REGEX = {
    QString::fromUtf8("[\u064B-\u065F]"),
    QString::fromUtf8("[\u0610-\u061A]"),
    QString::fromUtf8("[\u06D6-\u06ED]"),
    QString::fromUtf8("\u0640"),
    QString::fromUtf8("\u0670")
};

static const QMap<QString, QString> ARABIC_DIACRITIC_REPLACE = {
    {QString::fromUtf8("\u0624"), QString::fromUtf8("\u0648")}, // Replace Waw Hamza Above by Waw
    {QString::fromUtf8("\u0629"), QString::fromUtf8("\u0647")}, // Replace Ta Marbuta by Ha
    {QString::fromUtf8("\u064A"), QString::fromUtf8("\u0649")}, // Replace Ya by Alif Maksura
    {QString::fromUtf8("\u0626"), QString::fromUtf8("\u0649")}, // Replace Ya Hamza Above by Alif Maksura
    {QString::fromUtf8("\u0622"), QString::fromUtf8("\u0627")}, // Replace Alifs with Hamza Above
    {QString::fromUtf8("\u0623"), QString::fromUtf8("\u0627")}, // Replace Alifs with Hamza Below
    {QString::fromUtf8("\u0625"), QString::fromUtf8("\u0627")}  // Replace with Madda Above by Alif
};

bool OsmAnd::ArabicNormalizer::isSpecialArabic(const QString& text) {
    if (text.isEmpty()) {
        return false;
    }

    QChar firstChar = text.at(0);
    if (isArabicCharacter(firstChar)) {
        for (const QChar& c : text) {
            if (isDiacritic(c) || isArabicDigit(c) || isNeedReplace(c)) {
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

    // Remove diacritics using regex
    for (const QString& regexStr : ARABIC_DIACRITIC_REGEX) {
        QRegularExpression regex(regexStr);
        result.replace(regex, "");
    }

    // Replace characters
    QMap<QString, QString>::const_iterator it;
    for (it = ARABIC_DIACRITIC_REPLACE.constBegin(); it != ARABIC_DIACRITIC_REPLACE.constEnd(); ++it) {
        result.replace(it.key(), it.value());
    }
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
    return (c >= QChar(0x064B) && c <= QChar(0x065F)) ||
           (c >= QChar(0x0610) && c <= QChar(0x061A)) ||
           (c >= QChar(0x06D6) && c <= QChar(0x06ED)) ||
           c == QChar(0x0640) || c == QChar(0x0670);
}

bool OsmAnd::ArabicNormalizer::isArabicDigit(QChar c) {
    return (c.unicode() >= 0x0660 && c.unicode() <= 0x0669);// Arabic-Indic digits
}

bool OsmAnd::ArabicNormalizer::isArabicCharacter(QChar c) {
    return (c.unicode() >= 0x0600 && c.unicode() <= 0x06FF);  // Arabic Unicode block
}

bool OsmAnd::ArabicNormalizer::isNeedReplace(QChar c) {
    return ARABIC_DIACRITIC_REPLACE.contains(QString(c));
}
