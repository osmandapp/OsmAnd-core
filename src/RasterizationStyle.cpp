#include "RasterizationStyle.h"

#include <iostream>
#include <QByteArray>
#include <QBuffer>

#include "RasterizationStyles.h"
#include "EmbeddedResources.h"

OsmAnd::RasterizationStyle::RasterizationStyle( RasterizationStyles* owner, const QString& embeddedResourceName )
    : owner(owner)
    , _resourceName(embeddedResourceName)
    , resourceName(_resourceName)
    , externalFileName(_externalFileName)
    , name(_name)
    , parentName(_parentName)
    , options(_options)
{
}

OsmAnd::RasterizationStyle::RasterizationStyle( RasterizationStyles* owner, const QFile& externalStyleFile )
    : owner(owner)
    , resourceName(_resourceName)
    , _externalFileName(externalStyleFile.fileName())
    , externalFileName(_externalFileName)
    , name(_name)
    , parentName(_parentName)
    , options(_options)
{
}

OsmAnd::RasterizationStyle::~RasterizationStyle()
{
}

bool OsmAnd::RasterizationStyle::parseMetadata()
{
    if(!_resourceName.isEmpty())
    {
        QXmlStreamReader data(EmbeddedResources::decompressResource(_resourceName));
        return parseMetadata(data);
    }
    else if(!_externalFileName.isEmpty())
    {
        QFile styleFile(_externalFileName);
        if(!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QXmlStreamReader data(&styleFile);
        bool ok = parseMetadata(data);
        styleFile.close();
        if(!ok)
            return false;
    }

    return false;
}

bool OsmAnd::RasterizationStyle::parseMetadata( QXmlStreamReader& xmlReader )
{
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (tagName == "renderingStyle")
            {
                _name = xmlReader.attributes().value("name").toString();
                auto attrDepends = xmlReader.attributes().value("depends");
                if(!attrDepends.isNull())
                    _parentName = attrDepends.toString();
            }
        }
        else if (xmlReader.isEndElement())
        {
        }
    }
    if(xmlReader.hasError())
    {
        std::cerr << qPrintable(xmlReader.errorString()) << "(" << xmlReader.lineNumber() << ", " << xmlReader.columnNumber() << ")" << std::endl;
        return false;
    }

    return true;
}

bool OsmAnd::RasterizationStyle::isStandalone() const
{
    return _parentName.isEmpty();
}

bool OsmAnd::RasterizationStyle::areDependenciesResolved() const
{
    if(isStandalone())
        return true;

    return _parent && _parent->areDependenciesResolved();
}

bool OsmAnd::RasterizationStyle::resolveDependencies()
{
    // Make sure parent is resolved before this style (if present)
    if(!_parentName.isEmpty() && !_parent)
    {
        if(!owner->obtainStyle(_parentName, _parent))
            return false;
    }

    return true;
}

bool OsmAnd::RasterizationStyle::parse()
{
    if(!_resourceName.isEmpty())
    {
        QXmlStreamReader data(EmbeddedResources::decompressResource(_resourceName));
        return parse(data);
    }
    else if(!_externalFileName.isEmpty())
    {
        QFile styleFile(_externalFileName);
        if(!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QXmlStreamReader data(&styleFile);
        bool ok = parse(data);
        styleFile.close();
        if(!ok)
            return false;
    }

    return false;
}

bool OsmAnd::RasterizationStyle::parse( QXmlStreamReader& xmlReader )
{
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (tagName == "renderingStyle")
            {
            }
            else if(tagName == "renderingProperty")
            {
                std::shared_ptr<Option> option;

                const auto& attribs = xmlReader.attributes();
                auto name = attribs.value("attr").toString();
                auto type = attribs.value("type");
                auto title = attribs.value("name").toString();
                auto description = attribs.value("description").toString();
                auto encodedPossibleValues = attribs.value("possibleValues");
                QStringList possibleValues;
                if(!encodedPossibleValues.isEmpty())
                    possibleValues = encodedPossibleValues.toString().split(",", QString::SkipEmptyParts);
                if(type == "string")
                {
                    option.reset(new Option(
                        RasterizationStyleExpression::String,
                        name,
                        title,
                        description,
                        possibleValues));
                }
                else if(type == "boolean")
                {
                    option.reset(new Option(
                        RasterizationStyleExpression::Boolean,
                        name,
                        title,
                        description,
                        possibleValues));
                }
                else // treat as integer
                {
                    option.reset(new Option(
                        RasterizationStyleExpression::Integer,
                        name,
                        title,
                        description,
                        possibleValues));
                }

                if(option)
                    _options.insert(option->name, option);
            }
            else if(tagName == "renderingAttribute")
            {
            }
        }
        else if (xmlReader.isEndElement())
        {
        }
    }
    if(xmlReader.hasError())
    {
        std::cerr << qPrintable(xmlReader.errorString()) << "(" << xmlReader.lineNumber() << ", " << xmlReader.columnNumber() << ")" << std::endl;
        return false;
    }

    return true;
}

OsmAnd::RasterizationStyle::Option::Option( Type type_, const QString& name_, const QString& title_, const QString& description_, const QStringList& possibleValues_ )
    : RasterizationStyleExpression(type_, name_)
    , title(title_)
    , description(description_)
{
}

OsmAnd::RasterizationStyle::Option::~Option()
{
}

bool OsmAnd::RasterizationStyle::Option::evaluate() const
{
return true;
}
