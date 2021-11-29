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

    _title = attribs.value(QStringLiteral("name")).toString();
    _parentName = attribs.value(QStringLiteral("depends")).toString();

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
        if (tagName == QStringLiteral("renderingStyle"))
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
            "XML error: %s (%" PRIi64 ", %" PRIi64 ")",
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



bool OsmAnd::UnresolvedMapStyle_P::processStartElement(OsmAnd::MapStyleRulesetType &currentRulesetType,
                                                QStack<std::shared_ptr<RuleNode> > &ruleNodesStack,
                                                const QString &tagName,
                                                const QXmlStreamAttributes &attribs,
                                                qint64 lineNum, qint64 columnNum
                                                ) {
    
//    const auto& attribs = xmlReader.attributes();
    
    if (tagName == QStringLiteral("renderingConstant"))
    {
        const auto nameAttribValue = attribs.value(QStringLiteral("name")).toString();
        const auto valueAttribValue = attribs.value(QStringLiteral("value")).toString();
        OsmAnd::UnresolvedMapStyle_P::constants.insert(nameAttribValue, valueAttribValue);
    }
    else if (tagName == QStringLiteral("renderingProperty"))
    {
        const auto title = attribs.value(QStringLiteral("name")).toString();
        const auto description = attribs.value(QStringLiteral("description")).toString();
        const auto category = attribs.value(QStringLiteral("category")).toString();
        const auto name = attribs.value(QStringLiteral("attr")).toString();
        const auto valueType = attribs.value(QStringLiteral("type")).toString();
        const auto possibleValues = attribs.value(QStringLiteral("possibleValues")).toString()
        .split(QLatin1Char(','), Qt::SkipEmptyParts);
        const auto defaultValueDescription = attribs.value(QStringLiteral("defaultValueDescription")).toString();
        
        MapStyleValueDataType dataType;
        if (valueType == QStringLiteral("string"))
            dataType = MapStyleValueDataType::String;
        else if (valueType == QStringLiteral("boolean"))
            dataType = MapStyleValueDataType::Boolean;
        else
        {
            LogPrintf(LogSeverityLevel::Error,
                      "'%s' type is not supported (%s) - skip it",
                      qPrintable(valueType),
                      qPrintable(name));
            return false;
        }
        
        const std::shared_ptr<const Parameter> newParameter(new Parameter(
                                                                          title,
                                                                          description,
                                                                          category,
                                                                          name,
                                                                          dataType,
                                                                          possibleValues,
                                                                          defaultValueDescription));
        parameters.push_back(qMove(newParameter));
    }
    else if (tagName == QStringLiteral("renderingAttribute"))
    {
        const auto nameAttribValue = attribs.value(QStringLiteral("name")).toString();
        
        const std::shared_ptr<Attribute> newAttribute(new Attribute(nameAttribValue));
        
        if (!ruleNodesStack.isEmpty())
        {
            LogPrintf(LogSeverityLevel::Error, "Previous rule node was closed before opening <renderingAttribute>");
            return false;
        }
        ruleNodesStack.push(newAttribute->rootNode);
        
        attributes.push_back(qMove(newAttribute));
    }
    else if (tagName == QStringLiteral("switch") || tagName == QStringLiteral("group"))
    {
        const std::shared_ptr<RuleNode> newSwitchNode(new RuleNode(true));
        
        for (const auto& xmlAttrib : constOf(attribs))
        {
            const auto attribName = xmlAttrib.name().toString();
            const auto attribValue = xmlAttrib.value().toString();
            newSwitchNode->values[attribName] = attribValue;
        }
        ruleNodesStack.push(newSwitchNode);
    }
    else if (tagName == QStringLiteral("case") || tagName == QStringLiteral("filter"))
    {
        const std::shared_ptr<RuleNode> newCaseNode(new RuleNode(false));
        
        for (const auto& xmlAttrib : constOf(attribs))
        {
            const auto attribName = xmlAttrib.name().toString();
            const auto attribValue = xmlAttrib.value().toString();
            
            newCaseNode->values[attribName] = attribValue;
        }
        
        if (ruleNodesStack.isEmpty() &&
            (!newCaseNode->values.contains(QStringLiteral("tag")) || !newCaseNode->values.contains(QStringLiteral("value"))))
        {
            LogPrintf(LogSeverityLevel::Error,
                      "Top-level <case> must have 'tag' and 'value' attributes at %" PRIi64 ":%" PRIi64,
                      lineNum,
                      columnNum);
            return false;
        }
        ruleNodesStack.push(newCaseNode);
    }
    else if (tagName == QStringLiteral("apply") || tagName == QStringLiteral("apply_if") || tagName == QStringLiteral("groupFilter"))
    {
        const std::shared_ptr<RuleNode> newApplyNode(new RuleNode(false));
        
        for (const auto& xmlAttrib : constOf(attribs))
        {
            const auto attribName = xmlAttrib.name().toString();
            const auto attribValue = xmlAttrib.value().toString();
            
            newApplyNode->values[attribName] = attribValue;
        }
        
        if (ruleNodesStack.isEmpty())
        {
            LogPrintf(LogSeverityLevel::Error,
                      "<apply> must be inside <switch>, <case> or <renderingAttribute> at %" PRIi64 ":%" PRIi64,
                      lineNum,
                      columnNum);
            return false;
        }
        ruleNodesStack.push(newApplyNode);
    }
    else if (tagName == QStringLiteral("order"))
    {
        currentRulesetType = MapStyleRulesetType::Order;
    }
    else if (tagName == QStringLiteral("text"))
    {
        currentRulesetType = MapStyleRulesetType::Text;
    }
    else if (tagName == QStringLiteral("point"))
    {
        currentRulesetType = MapStyleRulesetType::Point;
    }
    else if (tagName == QStringLiteral("line"))
    {
        currentRulesetType = MapStyleRulesetType::Polyline;
    }
    else if (tagName == QStringLiteral("polygon"))
    {
        currentRulesetType = MapStyleRulesetType::Polygon;
    }
    return true; //test
}

bool OsmAnd::UnresolvedMapStyle_P::processEndElement(OsmAnd::MapStyleRulesetType &currentRulesetType,
                                                QStack<std::shared_ptr<RuleNode> > &ruleNodesStack,
                                                const QString &tagName,
                                                qint64 lineNum, qint64 columnNum) {
    
    if (tagName == QStringLiteral("renderingAttribute"))
    {
        const auto attribute = ruleNodesStack.pop();
        if (!ruleNodesStack.isEmpty())
        {
            LogPrintf(LogSeverityLevel::Error, "<renderingAttribute> was not complete before </renderingAttribute>");
            return false;
        }
    }
    else if (tagName == QStringLiteral("switch") || tagName == QStringLiteral("group"))
    {
        const auto switchNode = ruleNodesStack.pop();
        if (!ruleNodesStack.isEmpty())
            ruleNodesStack.top()->oneOfConditionalSubnodes.push_back(switchNode);
        else
        {
            // Several cases:
            //  - Top-level <switch> may not have 'tag' and 'value' attributes pair, then it has to be copied inside every <case>-child
            //  - Otherwise, process as case
            const auto ok = insertNodeIntoTopLevelTagValueRule(
                                                               rulesets[static_cast<unsigned int>(currentRulesetType)],
                                                               currentRulesetType,
                                                               switchNode);
            if (!ok)
            {
                LogPrintf(LogSeverityLevel::Error,
                          "Failed to find 'tag' and 'value' attributes in nodes tree ending at %" PRIi64 ":%" PRIi64,
                          lineNum,
                          columnNum);
                return false;
            }
        }
    }
    else if (tagName == QStringLiteral("case") || tagName == QStringLiteral("filter"))
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
                          "Failed to find 'tag' and 'value' attributes in nodes tree ending at %" PRIi64 ":" PRIi64,
                          lineNum,
                          columnNum);
                return false;
            }
        }
    }
    else if (tagName == QStringLiteral("apply") || tagName == QStringLiteral("apply_if") || tagName == QStringLiteral("groupFilter"))
    {
        const auto applyNode = ruleNodesStack.pop();
        ruleNodesStack.top()->applySubnodes.push_back(applyNode);
    }
    else
    {
        currentRulesetType = MapStyleRulesetType::Invalid;
    }
    return true;
}

bool OsmAnd::UnresolvedMapStyle_P::parse(QXmlStreamReader& xmlReader)
{
    QList<std::shared_ptr<XmlTreeSequence>> sequenceHolder;
    std::shared_ptr<XmlTreeSequence> currentSeqElement = nullptr;
    QStack< std::shared_ptr<RuleNode> > ruleNodesStack;
    auto currentRulesetType = MapStyleRulesetType::Invalid;
    
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        QString tagName;
        tagName += xmlReader.name();
        if (xmlReader.isStartElement()) {
            const auto &attribs = xmlReader.attributes();
            if (tagName == QStringLiteral("renderingStyle"))
            {
                if (!isMetadataLoaded())
                {
                    if (!parseTagStart_RenderingStyle(xmlReader))
                        continue;
                }
            }
            else if (attribs.hasAttribute(SEQ_ATTR) || currentSeqElement != nullptr)
            {
                std::shared_ptr<XmlTreeSequence> seq = std::make_shared<XmlTreeSequence>();
                sequenceHolder.append(seq);
                seq->name += tagName;
                seq->attrsMap = attribs;
                seq->parent = currentSeqElement;
                if (currentSeqElement == nullptr) {
                    seq->seqOrder += attribs.value(SEQ_ATTR);
                    
                }
                else
                {
                    currentSeqElement->children.push_back(seq);
                    seq->seqOrder = currentSeqElement->seqOrder;
                }
                currentSeqElement = seq;
            }
            
            else
            {
                processStartElement(currentRulesetType, ruleNodesStack, tagName, attribs, xmlReader.lineNumber(), xmlReader.columnNumber());
            }
        }
        else if (xmlReader.isEndElement())
        {
            if (currentSeqElement == nullptr)
            {
                processEndElement(currentRulesetType, ruleNodesStack, tagName, xmlReader.lineNumber(), xmlReader.columnNumber());
            }
            else
            {
                auto process = currentSeqElement;
                currentSeqElement = currentSeqElement->parent.lock();
                if (currentSeqElement == nullptr)
                {
                    int seqEnd = process->seqOrder.split(QLatin1Char(':'))[1].toInt();
                    for (int i = 1; i < seqEnd; i++)
                    {
                        process->process(i,
                                         this,
                                         currentRulesetType,
                                         ruleNodesStack);
                    }
                }
            }
        }
    }
    if (xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning,
            "XML error: %s (%" PRIi64 ", %" PRIi64 ")",
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
    const auto itTagAttrib = ruleNode->values.find(QStringLiteral("tag"));
    const auto itValueAttrib = ruleNode->values.find(QStringLiteral("value"));
    const auto itAttribNotFound = ruleNode->values.end();

    if (ruleNode->isSwitch && (itTagAttrib == itAttribNotFound || itValueAttrib == itAttribNotFound))
    {
        // In case current <switch> has no 'tag' and 'value' attributes pair, it needs to be merged inside it's children
        for (const auto& oneOfConditionalSubnode : constOf(ruleNode->oneOfConditionalSubnodes))
        {
            // Merge only values from current <switch> node into its <case> node that are not yet set in the <case>
            mergeNonExistent(oneOfConditionalSubnode->values, ruleNode->values);

            // Merge all <apply> nodes from current <switch> node into its <case>-subnode, keeping following order:
            //  - <apply> nodes from <case>
            //  - <apply> nodes from <switch>
            oneOfConditionalSubnode->applySubnodes = oneOfConditionalSubnode->applySubnodes + ruleNode->applySubnodes;
            
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
            topLevelRule->rootNode->values[QStringLiteral("tag")] = tagAttrib;
            topLevelRule->rootNode->values[QStringLiteral("value")] = valueAttrib;
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

void OsmAnd::XmlTreeSequence::process(
                                    int index,
                                    OsmAnd::UnresolvedMapStyle_P *parserObj,
                                    OsmAnd::MapStyleRulesetType &currentRulesetType,
                                    QStack<std::shared_ptr<UnresolvedMapStyle::RuleNode> > &ruleNodesStack) {
    
    for (int i = 0; i < attrsMap.size(); i++)
    {
        if (attrsMap[i].value().contains(SEQ_PLACEHOLDER))
        {
            QString seqVal = attrsMap[i].value().toString();
            QString key = attrsMap[i].name().toString();
            seqVal.replace(SEQ_PLACEHOLDER, QString::number(index));
            const QXmlStreamAttribute seqAttr(key, seqVal);
            attrsMap.replace(i, seqAttr);
        }
    };

    parserObj->processStartElement(currentRulesetType, ruleNodesStack, name, attrsMap, lineNum, columnNum);
    for (const auto& child : children) {
        child.lock()->process(index, parserObj, currentRulesetType, ruleNodesStack);
        
    };
    parserObj->processEndElement(currentRulesetType, ruleNodesStack, name, lineNum, columnNum);
}
