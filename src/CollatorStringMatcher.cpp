#include "CollatorStringMatcher.h"
#include "CollatorStringMatcher_P.h"
#include "ArabicNormalizer.h"
#include <ICU.h>

OsmAnd::CollatorStringMatcher::CollatorStringMatcher(const QString& part, const StringMatcherMode mode)
    : _p(new CollatorStringMatcher_P(this))
{
    QString part_ = CollatorStringMatcher_P::simplifyStringAndAlignChars(part);
    StringMatcherMode mode_ = mode;
    if (part_.length() > 0 && part_.at(part_.length() - 1) == L'.') {
        part_ = part_.mid(0, part.length() - 1);
        if (mode == StringMatcherMode::CHECK_EQUALS_FROM_SPACE) {
            mode_ = StringMatcherMode::CHECK_STARTS_FROM_SPACE;
        } else if (mode == StringMatcherMode::CHECK_EQUALS) {
            mode_ = StringMatcherMode::CHECK_ONLY_STARTS_WITH;
        }
    }
    if (OsmAnd::ArabicNormalizer::isSpecialArabic(part_)) {
        part_ = OsmAnd::ArabicNormalizer::normalize(part_);
    }
    _part = part_;
    _mode = mode_;
}

OsmAnd::CollatorStringMatcher::~CollatorStringMatcher()
{
}

bool OsmAnd::CollatorStringMatcher::matches(const QString& name) const
{
    QString normalized = name;
    if (OsmAnd::ArabicNormalizer::isSpecialArabic(name)) {
        normalized = OsmAnd::ArabicNormalizer::normalize(name);
    }
    return _p->CollatorStringMatcher_P::matches(normalized, _part, _mode);
}

bool OsmAnd::CollatorStringMatcher::cmatches(const QString& _base, const QString& _part, StringMatcherMode _mode)
{
    QString base = _base;
    QString part = _part;
    if (OsmAnd::ArabicNormalizer::isSpecialArabic(_base)) {
        base = OsmAnd::ArabicNormalizer::normalize(_base);
    }
    if (OsmAnd::ArabicNormalizer::isSpecialArabic(_part)) {
        part = OsmAnd::ArabicNormalizer::normalize(_part);
    }
    return OsmAnd::ICU::cmatches(base, part, _mode);
}

bool OsmAnd::CollatorStringMatcher::ccontains(const QString& _base, const QString& _part)
{
    return OsmAnd::ICU::ccontains(_base, _part);
}

bool OsmAnd::CollatorStringMatcher::cstartsWith(const QString& _searchInParam, const QString& _theStart,
                                                bool checkBeginning, bool checkSpaces, bool equals)
{
    return OsmAnd::ICU::cstartsWith(_searchInParam, _theStart, checkBeginning, checkSpaces, equals);
}

QString OsmAnd::CollatorStringMatcher::simplifyStringAndAlignChars(const QString& fullText)
{
    return CollatorStringMatcher_P::simplifyStringAndAlignChars(fullText);
}

QString OsmAnd::CollatorStringMatcher::alignChars(const QString& fullText)
{
    return CollatorStringMatcher_P::alignChars(fullText);
}
