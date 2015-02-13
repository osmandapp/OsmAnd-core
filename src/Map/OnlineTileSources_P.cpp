#include "OnlineTileSources_P.h"
#include "OnlineTileSources.h"

#include "Logging.h"

OsmAnd::OnlineTileSources_P::OnlineTileSources_P(OnlineTileSources* owner_)
    : owner(owner_)
{
}

OsmAnd::OnlineTileSources_P::~OnlineTileSources_P()
{
}

bool OsmAnd::OnlineTileSources_P::deserializeFrom(QXmlStreamReader& xmlReader)
{
    QHash< QString, std::shared_ptr<const Source> > collection;

    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        if (!xmlReader.isStartElement())
            continue;
        const auto tagName = xmlReader.name();
        if (tagName == QLatin1String("tile_source"))
        {
            // Original format of the tile sources, used in the Android application
            if (xmlReader.attributes().hasAttribute("rule"))
                continue; // Rules are not supported

            const auto name = xmlReader.attributes().value(QLatin1String("name")).toString();
            if (collection.contains(name))
            {
                LogPrintf(LogSeverityLevel::Warning, "Ignored duplicate online tile source with name '%s'", qPrintable(name));
                continue;
            }
            const auto originalUrlTemplate = xmlReader.attributes().value(QLatin1String("url_template")).toString();
            auto urlPattern = originalUrlTemplate;
            urlPattern = urlPattern.replace(QLatin1String("{0}"), QLatin1String("${osm_zoom}"));
            urlPattern = urlPattern.replace(QLatin1String("{1}"), QLatin1String("${osm_x}"));
            urlPattern = urlPattern.replace(QLatin1String("{2}"), QLatin1String("${osm_y}"));
            const auto minZoom = static_cast<ZoomLevel>(xmlReader.attributes().value(QLatin1String("min_zoom")).toUInt());
            const auto maxZoom = static_cast<ZoomLevel>(xmlReader.attributes().value(QLatin1String("max_zoom")).toUInt());
            const auto tileSize = xmlReader.attributes().value(QLatin1String("tile_size")).toUInt();
            
            std::shared_ptr<Source> newSource(new Source(name));
            newSource->urlPattern = urlPattern;
            newSource->minZoom = minZoom;
            newSource->maxZoom = maxZoom;
            newSource->maxConcurrentDownloads = 1;
            newSource->tileSize = tileSize;
            newSource->alphaChannelPresence = AlphaChannelPresence::Unknown;
            collection.insert(name, newSource);
        }
        else if (tagName == QLatin1String("tileSource"))
        {
            //TODO: parse new format, but create it first :)
        }
    }
    if (xmlReader.hasError())
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML error: %s (%" PRIi64 ", %" PRIi64 ")",
            qPrintable(xmlReader.errorString()),
            xmlReader.lineNumber(),
            xmlReader.columnNumber());
        return false;
    }

    _collection = collection;

    return true;
}

bool OsmAnd::OnlineTileSources_P::serializeTo(QXmlStreamWriter& xmlWriter) const
{
    assert(false);
    return false;
}

bool OsmAnd::OnlineTileSources_P::loadFrom(const QByteArray& content)
{
    QXmlStreamReader xmlReader(content);
    return deserializeFrom(xmlReader);
}

bool OsmAnd::OnlineTileSources_P::loadFrom(QIODevice& ioDevice)
{
    QXmlStreamReader xmlReader(&ioDevice);
    return deserializeFrom(xmlReader);
}

bool OsmAnd::OnlineTileSources_P::saveTo(QIODevice& ioDevice) const
{
    QXmlStreamWriter xmlWriter(&ioDevice);
    return serializeTo(xmlWriter);
}

QHash< QString, std::shared_ptr<const OsmAnd::OnlineTileSources_P::Source> > OsmAnd::OnlineTileSources_P::getCollection() const
{
    return _collection;
}

std::shared_ptr<const OsmAnd::OnlineTileSources_P::Source> OsmAnd::OnlineTileSources_P::getSourceByName(const QString& sourceName) const
{
    const auto citSource = _collection.constFind(sourceName);
    if (citSource == _collection.cend())
        return nullptr;
    return *citSource;
}

bool OsmAnd::OnlineTileSources_P::addSource(const std::shared_ptr<Source>& source)
{
    if (_collection.constFind(source->name) != _collection.cend())
        return false;

    _collection.insert(source->name, source);

    return true;
}

bool OsmAnd::OnlineTileSources_P::removeSource(const QString& sourceName)
{
    return (_collection.remove(sourceName) > 0);
}

std::shared_ptr<OsmAnd::OnlineTileSources> OsmAnd::OnlineTileSources_P::_builtIn;

std::shared_ptr<const OsmAnd::OnlineTileSources> OsmAnd::OnlineTileSources_P::getBuiltIn()
{
    static QMutex mutex;
    QMutexLocker scopedLocker(&mutex);

    if (!_builtIn)
    {
        _builtIn.reset(new OnlineTileSources());

        std::shared_ptr<Source> osmAndSD(new Source(
            OnlineTileSources::BuiltInOsmAndSD,
            QLatin1String("Mapnik (OsmAnd)")));
        osmAndSD->urlPattern = QLatin1String("http://mapnik.osmand.net/${osm_zoom}/${osm_x}/${osm_y}.png");
        osmAndSD->minZoom = ZoomLevel0;
        osmAndSD->maxZoom = ZoomLevel19;
        osmAndSD->maxConcurrentDownloads = 0;
        osmAndSD->tileSize = 256;
        osmAndSD->alphaChannelPresence = AlphaChannelPresence::NotPresent;
        _builtIn->addSource(osmAndSD);

        std::shared_ptr<Source> osmAndHD(new Source(
            OnlineTileSources::BuiltInOsmAndHD,
            QLatin1String("Mapnik HD (OsmAnd)")));
        osmAndHD->urlPattern = QLatin1String("http://mapnikhd.osmand.net/hd/${osm_zoom}/${osm_x}/${osm_y}.png");
        osmAndHD->minZoom = ZoomLevel0;
        osmAndHD->maxZoom = ZoomLevel19;
        osmAndHD->maxConcurrentDownloads = 0;
        osmAndHD->tileSize = 512;
        osmAndHD->alphaChannelPresence = AlphaChannelPresence::NotPresent;
        _builtIn->addSource(osmAndHD);
    }

    return _builtIn;
}
