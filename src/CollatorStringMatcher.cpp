#include "CollatorStringMatcher.h"
#include "CollatorStringMatcher_P.h"
#include "ArabicNormalizer.h"
#include <ICU.h>

OsmAnd::CollatorStringMatcher::CollatorStringMatcher(const QString& part, const StringMatcherMode mode)
    : _p(new CollatorStringMatcher_P(this))
{
    QString part_ = CollatorStringMatcher_P::lowercaseAndAlignChars(part);
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
        QString normalized  = OsmAnd::ArabicNormalizer::normalize(part_);
        part_ = normalized.isEmpty() ? part_ : normalized;
    }
    _part = part_;
    _mode = mode_;
}

OsmAnd::CollatorStringMatcher::~CollatorStringMatcher()
{
}

bool OsmAnd::CollatorStringMatcher::matches(const QString& name) const
{
    QString name_ = name;
    if (OsmAnd::ArabicNormalizer::isSpecialArabic(name)) {
        QString arabic = OsmAnd::ArabicNormalizer::normalize(name);
        name_ = arabic.isEmpty() ? name : arabic;
    }
    return _p->CollatorStringMatcher_P::matches(name_, _part, _mode);
}

bool OsmAnd::CollatorStringMatcher::cmatches(const QString& _base, const QString& _part, StringMatcherMode _mode)
{
    return cmatches(_base, _part, true, _mode);
}

bool OsmAnd::CollatorStringMatcher::cmatches(const QString& _base, const QString& _part, bool alignPart, StringMatcherMode _mode)
{
    
    
    QString base = _base;
    if (!_base.isNull() && _base.indexOf('-') != -1)
    {
        // Test if it matches without the hyphen
        if (cmatches(base.replace("-", ""), _part, _mode))
        {
            return true;
        }
    }
    QString part = _part;
    if (alignPart)
    {
        part = alignChars(part);
    }
    base = lowercaseAndAlignChars(base);
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

QString OsmAnd::CollatorStringMatcher::lowercaseAndAlignChars(const QString& fullText)
{
    return CollatorStringMatcher_P::lowercaseAndAlignChars(fullText);
}

QString OsmAnd::CollatorStringMatcher::alignChars(const QString& fullText)
{
    return CollatorStringMatcher_P::alignChars(fullText);
}
