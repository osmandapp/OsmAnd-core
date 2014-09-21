#include "UnresolvedMapStyle_P.h"
#include "UnresolvedMapStyle.h"

#include "stdlib_common.h"
#include <iostream>
#include <array>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QStringList>
#include <QByteArray>
#include <QFileInfo>
#include <QStack>
#include "restore_internal_warnings.h"
#include "QtCommon.h"

#include "IMapStylesCollection.h"
#include "MapStyleValueDefinition.h"
#include "MapStyleConstantValue.h"
#include "CoreResourcesEmbeddedBundle.h"
#include "Logging.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"

OsmAnd::UnresolvedMapStyle_P::UnresolvedMapStyle_P(UnresolvedMapStyle* owner_, const std::shared_ptr<QIODevice>& source_, const QString& name_)
    : _source(source_)
    , _name(name_)
    , owner(owner_)
{
}

OsmAnd::UnresolvedMapStyle_P::~UnresolvedMapStyle_P()
{
}

bool OsmAnd::UnresolvedMapStyle_P::parseTagStart_RenderingStyle(QXmlStreamReader& xmlReader)
{
    const auto& attribs = xmlReader.attributes();

    _title = attribs.value(QLatin1String("name")).toString();
    _parentName = attribs.value(QLatin1String("depends")).toString();

    return true;
}

bool OsmAnd::UnresolvedMapStyle_P::parseMetadata()
{
    if (!_source->open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QXmlStreamReader data(_source.get());
    const bool ok = parseMetadata(data);
    _source->close();

    return ok;
}

bool OsmAnd::UnresolvedMapStyle_P::parseMetadata(QXmlStreamReader& xmlReader)
{
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (tagName == QLatin1String("renderingStyle"))
        {
            if (xmlReader.isStartElement())
            {
                if (!parseTagStart_RenderingStyle(xmlReader))
                    return false;
            }
            else if (xmlReader.isEndElement())
            {
            }
        }
    }
    if (xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning,
            "XML error: %s (%d, %d)",
            qPrintable(xmlReader.errorString()),
            xmlReader.lineNumber(),
            xmlReader.columnNumber());
        return false;
    }

    return true;
}

bool OsmAnd::UnresolvedMapStyle_P::parse()
{
    if (!_source->open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QXmlStreamReader data(_source.get());
    bool ok = parse(data);
    _source->close();
    return ok;
}

bool OsmAnd::UnresolvedMapStyle_P::parse(QXmlStreamReader& xmlReader)
{
    QHash<QString, QString> constants;
    QList< std::shared_ptr<const Parameter> > parameters;
    QList< std::shared_ptr<const Attribute> > attributes;
    std::array<EditableRulesByTagValueCollection, MapStyleRulesetTypesCount> rulesets;

    QStack< std::shared_ptr<RuleNode> > ruleNodesStack;
    auto currentRulesetType = MapStyleRulesetType::Invalid;

    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (tagName == QLatin1String("renderingStyle"))
        {
            if (xmlReader.isStartElement())
            {
                if (!isMetadataLoaded())
                {
                    if (!parseTagStart_RenderingStyle(xmlReader))
                        return false;
                }
            }
            else if (xmlReader.isEndElement())
            {
            }
        }
        else if (tagName == QLatin1String("renderingConstant"))
        {
            if (xmlReader.isStartElement())
            {
                const auto& attribs = xmlReader.attributes();

                const auto nameAttribValue = attribs.value(QLatin1String("name")).toString();
                const auto valueAttribValue = attribs.value(QLatin1String("value")).toString();
                constants.insert(nameAttribValue, valueAttribValue);
            }
            else if (xmlReader.isEndElement())
            {
            }
        }
        else if (tagName == QLatin1String("renderingProperty"))
        {
            if (xmlReader.isStartElement())
            {
                const auto& attribs = xmlReader.attributes();

                const auto nameAttribValue = attribs.value(QLatin1String("attr")).toString();
                const auto typeAttribValue = attribs.value(QLatin1String("type")).toString();
                const auto titleAttribValue = attribs.value(QLatin1String("name")).toString();
                const auto descriptionAttribValue = attribs.value(QLatin1String("description")).toString();
                const auto possibleValues = attribs.value(QLatin1String("possibleValues")).toString().split(QLatin1Char(','), QString::SkipEmptyParts);

                MapStyleValueDataType dataType;
                if (typeAttribValue == QLatin1String("string"))
                    dataType = MapStyleValueDataType::String;
                else if (typeAttribValue == QLatin1String("boolean"))
                    dataType = MapStyleValueDataType::Boolean;
                else
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "'%s' type is not supported (%s)",
                        qPrintable(typeAttribValue),
                        qPrintable(nameAttribValue));
                    return false;
                }

                const std::shared_ptr<const Parameter> newParameter(new Parameter(
                    titleAttribValue,
                    descriptionAttribValue,
                    nameAttribValue,
                    dataType,
                    possibleValues));
                parameters.push_back(qMove(newParameter));
            }
            else if (xmlReader.isEndElement())
            {
            }
        }
        else if (tagName == QLatin1String("renderingAttribute"))
        {
            if (xmlReader.isStartElement())
            {
                const auto& attribs = xmlReader.attributes();

                const auto nameAttribValue = attribs.value(QLatin1String("name")).toString();

                const std::shared_ptr<Attribute> newAttribute(new Attribute(
                    nameAttribValue));

                if (!ruleNodesStack.isEmpty())
                {
                    LogPrintf(LogSeverityLevel::Error, "Previous rule node was closed before opening <renderingAttribute>");
                    return false;
                }
                ruleNodesStack.push(newAttribute->rootNode);

                attributes.push_back(qMove(newAttribute));
            }
            else if (xmlReader.isEndElement())
            {
                const auto attribute = ruleNodesStack.pop();
                if (!ruleNodesStack.isEmpty())
                {
                    LogPrintf(LogSeverityLevel::Error, "<renderingAttribute> was not complete before </renderingAttribute>");
                    return false;
                }
            }
        }
        else if (tagName == QLatin1String("switch") || tagName == QLatin1String("group"))
        {
            if (xmlReader.isStartElement())
            {
                const std::shared_ptr<RuleNode> newSwitchNode(new RuleNode(true));

                const auto& attribs = xmlReader.attributes();
                for (const auto& xmlAttrib : constOf(attribs))
                {
                    const auto attribName = xmlAttrib.name().toString();
                    const auto attribValue = xmlAttrib.value().toString();

                    newSwitchNode->values[attribName] = attribValue;
                }

                ruleNodesStack.push(newSwitchNode);
            }
            else if (xmlReader.isEndElement())
            {
                const auto switchNode = ruleNodesStack.pop();
                if (!ruleNodesStack.isEmpty())
                    ruleNodesStack.top()->oneOfConditionalSubnodes.push_back(switchNode);
                else
                {
                    // Several cases:
                    //  - Top-level <switch> may not have 'tag' and 'value' attributes pair, then it has to be flattened
                    //  - Otherwise, process as case
                    const auto ok = insertNodeIntoTopLevelTagValueRule(
                        rulesets[static_cast<unsigned int>(currentRulesetType)],
                        currentRulesetType,
                        switchNode);
                    if (!ok)
                    {
                        LogPrintf(LogSeverityLevel::Error,
                            "Failed to find 'tag' and 'value' attributes in nodes tree ending at %d:%d",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        return false;
                    }
                }
            }
        }
        else if (tagName == QLatin1String("case") || tagName == QLatin1String("filter"))
        {
            if (xmlReader.isStartElement())
            {
                const std::shared_ptr<RuleNode> newCaseNode(new RuleNode(false));

                const auto& attribs = xmlReader.attributes();
                for (const auto& xmlAttrib : constOf(attribs))
                {
                    const auto attribName = xmlAttrib.name().toString();
                    const auto attribValue = xmlAttrib.value().toString();

                    newCaseNode->values[attribName] = attribValue;
                }

                if (ruleNodesStack.isEmpty() &&
                    (!newCaseNode->values.contains(QLatin1String("tag")) || !newCaseNode->values.contains(QLatin1String("value"))))
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "Top-level <case> must have 'tag' and 'value' attributes at %d:%d",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    return false;
                }
                ruleNodesStack.push(newCaseNode);
            }
            else if (xmlReader.isEndElement())
            {
                const auto caseNode = ruleNodesStack.pop();
                if (!ruleNodesStack.isEmpty())
                    ruleNodesStack.top()->oneOfConditionalSubnodes.push_back(caseNode);
                else
                {
                    // In case there's no <switch> parent, create or obtain top-level one with 'tag' and 'value' attributes pair of this case
                    const auto ok = insertNodeIntoTopLevelTagValueRule(
                        rulesets[static_cast<unsigned int>(currentRulesetType)],
                        currentRulesetType,
                        caseNode);
                    if (!ok)
                    {
                        LogPrintf(LogSeverityLevel::Error,
                            "Failed to find 'tag' and 'value' attributes in nodes tree ending at %d:%d",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        return false;
                    }
                }
            }
        }
        else if (tagName == QLatin1String("apply") || tagName == QLatin1String("apply_if") || tagName == QLatin1String("groupFilter"))
        {
            if (xmlReader.isStartElement())
            {
                const std::shared_ptr<RuleNode> newApplyNode(new RuleNode(false));

                const auto& attribs = xmlReader.attributes();
                for (const auto& xmlAttrib : constOf(attribs))
                {
                    const auto attribName = xmlAttrib.name().toString();
                    const auto attribValue = xmlAttrib.value().toString();

                    newApplyNode->values[attribName] = attribValue;
                }

                if (ruleNodesStack.isEmpty())
                {
                    LogPrintf(LogSeverityLevel::Error,
                        "<apply> must be inside <switch>, <case> or <renderingAttribute> at %d:%d",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    return false;
                }
                ruleNodesStack.push(newApplyNode);
            }
            else if (xmlReader.isEndElement())
            {
                const auto applyNode = ruleNodesStack.pop();
                ruleNodesStack.top()->applySubnodes.push_back(applyNode);
            }
        }
        else if (tagName == QLatin1String("order"))
        {
            if (xmlReader.isStartElement())
                currentRulesetType = MapStyleRulesetType::Order;
            else if (xmlReader.isEndElement())
                currentRulesetType = MapStyleRulesetType::Invalid;
        }
        else if (tagName == QLatin1String("text"))
        {
            if (xmlReader.isStartElement())
                currentRulesetType = MapStyleRulesetType::Text;
            else if (xmlReader.isEndElement())
                currentRulesetType = MapStyleRulesetType::Invalid;
        }
        else if (tagName == QLatin1String("point"))
        {
            if (xmlReader.isStartElement())
                currentRulesetType = MapStyleRulesetType::Point;
            else if (xmlReader.isEndElement())
                currentRulesetType = MapStyleRulesetType::Invalid;
        }
        else if (tagName == QLatin1String("line"))
        {
            if (xmlReader.isStartElement())
                currentRulesetType = MapStyleRulesetType::Polyline;
            else if (xmlReader.isEndElement())
                currentRulesetType = MapStyleRulesetType::Invalid;
        }
        else if (tagName == QLatin1String("polygon"))
        {
            if (xmlReader.isStartElement())
                currentRulesetType = MapStyleRulesetType::Polygon;
            else if (xmlReader.isEndElement())
                currentRulesetType = MapStyleRulesetType::Invalid;
        }
    }
    if (xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning,
            "XML error: %s (%d, %d)",
            qPrintable(xmlReader.errorString()),
            xmlReader.lineNumber(),
            xmlReader.columnNumber());
        return false;
    }

    // Save loaded rulesets
    _constants = constants;
    _parameters = parameters;
    _attributes = attributes;
    for (auto rulesetTypeIdx = 0u; rulesetTypeIdx < MapStyleRulesetTypesCount; rulesetTypeIdx++)
    {
        const auto& src = rulesets[rulesetTypeIdx];
        auto& dst = _rulesets[rulesetTypeIdx];

        auto itSrcByTag = iteratorOf(src);
        while (itSrcByTag.hasNext())
        {
            const auto& srcByTagEntry = itSrcByTag.next();
            const auto& srcByTag = srcByTagEntry.value();
            auto& dstByTag = dst[srcByTagEntry.key()];

            auto itSrcByValue = iteratorOf(srcByTag);
            while (itSrcByValue.hasNext())
            {
                const auto& srcByValueEntry = itSrcByValue.next();
                const auto& srcByValue = srcByValueEntry.value();
                auto& dstByValue = dstByTag[srcByValueEntry.key()];

                dstByValue = srcByValue;
            }
        }
    }

    return true;
}

bool OsmAnd::UnresolvedMapStyle_P::insertNodeIntoTopLevelTagValueRule(
    EditableRulesByTagValueCollection& rules,
    const MapStyleRulesetType rulesetType,
    const std::shared_ptr<RuleNode>& ruleNode)
{
    const auto itTagAttrib = ruleNode->values.find(QLatin1String("tag"));
    const auto itValueAttrib = ruleNode->values.find(QLatin1String("value"));
    const auto itAttribNotFound = ruleNode->values.end();

    if (!ruleNode->oneOfConditionalSubnodes.isEmpty() && (itTagAttrib == itAttribNotFound || itValueAttrib == itAttribNotFound))
    {
        // In case current <switch> has no 'tag' and 'value' attributes pair, it needs to be merged inside it's children
        for (const auto& oneOfConditionalSubnode : constOf(ruleNode->oneOfConditionalSubnodes))
        {
            // Merge only values from current <switch> node into its <case> node that are not yet set
            mergeNonExistent(oneOfConditionalSubnode->values, ruleNode->values);

            // Merge all <apply> nodes from current <switch> node into its <case> node before own <apply> nodes
            // to keep proper apply order
            oneOfConditionalSubnode->applySubnodes = ruleNode->applySubnodes + oneOfConditionalSubnode->applySubnodes;
            
            if (!insertNodeIntoTopLevelTagValueRule(rules, rulesetType, oneOfConditionalSubnode))
                return false;
        }
    }
    else
    {
        // If there's nothing to merge, but 'tag' and 'value' attributes not available - fail
        if (itTagAttrib == itAttribNotFound || itValueAttrib == itAttribNotFound)
            return false;

        const auto tagAttrib = *itTagAttrib;
        ruleNode->values.erase(itTagAttrib);

        const auto valueAttrib = *itValueAttrib;
        ruleNode->values.erase(itValueAttrib);

        auto& topLevelRule = rules[tagAttrib][valueAttrib];
        if (!topLevelRule)
        {
            // If there's no rule with corresponding "tag-value", create one
            topLevelRule.reset(new Rule(rulesetType));
            topLevelRule->rootNode->values[QLatin1String("tag")] = tagAttrib;
            topLevelRule->rootNode->values[QLatin1String("value")] = valueAttrib;
        }

        topLevelRule->rootNode->oneOfConditionalSubnodes.push_back(ruleNode);
    }

    return true;
}

bool OsmAnd::UnresolvedMapStyle_P::isStandalone() const
{
    return _parentName.isEmpty();
}

bool OsmAnd::UnresolvedMapStyle_P::isMetadataLoaded() const
{
    return _isMetadataLoaded.loadAcquire() != 0;
}

bool OsmAnd::UnresolvedMapStyle_P::loadMetadata()
{
    if (!isMetadataLoaded())
    {
        QMutexLocker scopedLocker(&_metadataLoadMutex);

        if (isMetadataLoaded())
            return true;

        if (!parseMetadata())
            return false;

        _isMetadataLoaded.storeRelease(1);
    }

    return true;
}

bool OsmAnd::UnresolvedMapStyle_P::isLoaded() const
{
    return _isLoaded.loadAcquire() != 0;
}

bool OsmAnd::UnresolvedMapStyle_P::load()
{
    if (!loadMetadata())
        return false;

    if (!isLoaded())
    {
        QMutexLocker scopedLocker(&_loadMutex);

        if (isLoaded())
            return true;

        if (!parse())
            return false;

        _isLoaded.storeRelease(1);
    }

    return true;
}
