#include "RenderStyle.h"

#include <iostream>
#include <QByteArray>
#include <QBuffer>

#include "RenderStyles.h"
#include "EmbeddedResources.h"

OsmAnd::RenderStyle::RenderStyle( RenderStyles* owner, const QString& embeddedResourceName )
    : owner(owner)
    , _resourceName(embeddedResourceName)
    , resourceName(_resourceName)
    , externalFileName(_externalFileName)
    , name(_name)
    , parentName(_parentName)
{
}

OsmAnd::RenderStyle::RenderStyle( RenderStyles* owner, const QFile& externalStyleFile )
    : owner(owner)
    , resourceName(_resourceName)
    , _externalFileName(externalStyleFile.fileName())
    , externalFileName(_externalFileName)
    , name(_name)
    , parentName(_parentName)
{
}

OsmAnd::RenderStyle::~RenderStyle()
{
}

bool OsmAnd::RenderStyle::parseMetadata()
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

bool OsmAnd::RenderStyle::parseMetadata( QXmlStreamReader& xmlReader )
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

bool OsmAnd::RenderStyle::isStandalone() const
{
    return _parentName.isEmpty();
}

bool OsmAnd::RenderStyle::areDependenciesResolved() const
{
    if(isStandalone())
        return true;

    return _parent && _parent->areDependenciesResolved();
}

bool OsmAnd::RenderStyle::resolveDependencies()
{
    // Make sure parent is resolved before this style (if present)
    if(!_parentName.isEmpty() && !_parent)
    {
        if(!owner->obtainCompleteStyle(_parentName, _parent))
            return false;
    }

    return true;
}
