#include "UnresolvedMapStyle.h"
#include "UnresolvedMapStyle_P.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QFileInfo>
#include "restore_internal_warnings.h"

#include "QKeyValueIterator.h"
#include "Logging.h"

OsmAnd::UnresolvedMapStyle::UnresolvedMapStyle(const std::shared_ptr<QIODevice>& source_, const QString& name_)
    : _p(new UnresolvedMapStyle_P(this, source_, name_))
    , title(_p->_title)
    , name(_p->_name)
    , parentName(_p->_parentName)
    , constants(_p->_constants)
    , parameters(_p->_parameters)
    , attributes(_p->_attributes)
    , rulesets(_p->_rulesets)
{
}

OsmAnd::UnresolvedMapStyle::UnresolvedMapStyle(const QString& fileName_, const QString& name_)
    : UnresolvedMapStyle(
    std::shared_ptr<QIODevice>(new QFile(fileName_)),
    name_.isNull() ? QFileInfo(fileName_).fileName().remove(QLatin1String(".render.xml")) : name_)
{
}

OsmAnd::UnresolvedMapStyle::~UnresolvedMapStyle()
{
}

bool OsmAnd::UnresolvedMapStyle::isStandalone() const
{
    return _p->isStandalone();
}

bool OsmAnd::UnresolvedMapStyle::isAddon() const
{
    return _p->isAddon();
}

bool OsmAnd::UnresolvedMapStyle::isMetadataLoaded() const
{
    return _p->isMetadataLoaded();
}

bool OsmAnd::UnresolvedMapStyle::loadMetadata()
{
    return _p->loadMetadata();
}

bool OsmAnd::UnresolvedMapStyle::isLoaded() const
{
    return _p->isLoaded();
}

bool OsmAnd::UnresolvedMapStyle::load()
{
    return _p->load();
}

OsmAnd::UnresolvedMapStyle::RuleNode::RuleNode(const bool isSwitch_)
    : isSwitch(isSwitch_)
{
}

OsmAnd::UnresolvedMapStyle::RuleNode::~RuleNode()
{
}

OsmAnd::UnresolvedMapStyle::BaseRule::BaseRule(RuleNode* const ruleNode_)
    : rootNode(ruleNode_)
{
}

OsmAnd::UnresolvedMapStyle::BaseRule::~BaseRule()
{
}

OsmAnd::UnresolvedMapStyle::Rule::Rule(const MapStyleRulesetType rulesetType_)
    : BaseRule(new RuleNode(true))
    , rulesetType(rulesetType_)
{
}

OsmAnd::UnresolvedMapStyle::Rule::~Rule()
{
}

OsmAnd::UnresolvedMapStyle::Attribute::Attribute(const QString& name_)
    : BaseRule(new RuleNode(false))
    , name(name_)
{
}

OsmAnd::UnresolvedMapStyle::Attribute::~Attribute()
{
}

OsmAnd::UnresolvedMapStyle::Parameter::Parameter(
    const QString& title_,
    const QString& description_,
    const QString& category_,
    const QString& name_,
    const MapStyleValueDataType dataType_,
    const QStringList& possibleValues_,
    const QString& defaultValueDescription_)
    : title(title_)
    , description(description_)
    , category(category_)
    , name(name_)
    , dataType(dataType_)
    , possibleValues(possibleValues_)
    , defaultValueDescription(defaultValueDescription_)
{
}

OsmAnd::UnresolvedMapStyle::Parameter::~Parameter()
{
}
