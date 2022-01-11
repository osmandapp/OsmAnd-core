#include "OnlineTileSources_P.h"
#include "OnlineTileSources.h"

#include "ICoreResourcesProvider.h"
#include "Logging.h"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

OsmAnd::OnlineTileSources_P::OnlineTileSources_P(OnlineTileSources* owner_)
    : owner(owner_)
{
}

OsmAnd::OnlineTileSources_P::~OnlineTileSources_P()
{
}

std::shared_ptr<OsmAnd::OnlineTileSources::Source> OsmAnd::OnlineTileSources_P::createTileSourceTemplate(const QXmlStreamAttributes &attributes)
{
    std::shared_ptr<Source> tileSource = nullptr;
    if (attributes.hasAttribute("rule"))
    {
        const QString rule = attributes.value(QStringLiteral("rule")).toString();
        if (rule.isEmpty() || rule == QStringLiteral("template:1"))
            tileSource = createSimpleTileSourceTemplate(attributes, rule);
        else if (rule == QStringLiteral("wms_tile"))
            tileSource = createWmsTileSourceTemplate(attributes);
    }
    else
    {
        tileSource = createSimpleTileSourceTemplate(attributes, QStringLiteral(""));
    }
    return tileSource;
}

std::shared_ptr<OsmAnd::OnlineTileSources::Source> OsmAnd::OnlineTileSources_P::createWmsTileSourceTemplate(const QXmlStreamAttributes &attributes)
{
    const QString name = attributes.value(QStringLiteral("name")).toString();
    const QString layer = attributes.value(QStringLiteral("layer")).toString();
    QString urlTemplate = attributes.value(QStringLiteral("url_template")).toString();
    
    if (name.isEmpty() || urlTemplate.isEmpty() || layer.isEmpty())
        return nullptr;
    
    std::shared_ptr<Source> source(new Source(name));
    
    source->maxZoom = ZoomLevel(parseInt(attributes, QStringLiteral("max_zoom"), 18));
    source->minZoom = ZoomLevel(parseInt(attributes, QStringLiteral("min_zoom"), 5));
    source->tileSize = parseInt(attributes, QStringLiteral("tile_size"), 256);
    const QString ext = !attributes.hasAttribute(QStringLiteral("ext")) ? QStringLiteral(".jpg") : attributes.value(QStringLiteral("ext")).toString();
    source->ext = ext;
    source->bitDensity = parseInt(attributes, QStringLiteral("img_density"), 16);
    source->avgSize = parseInt(attributes, QStringLiteral("avg_img_size"), 18000);
    const QString randoms = attributes.value(QStringLiteral("randoms")).toString();
    source->randoms = randoms;
    source->randomsArray = OnlineTileSources_P::parseRandoms(randoms);
    urlTemplate = QStringLiteral("http://whoots.mapwarper.net/tms/{0}/{1}/{2}/") + layer + QStringLiteral("/") + urlTemplate;
    source->urlToLoad = urlTemplate;
    source->rule = QStringLiteral("wms_tile");
    
    return source;
}

std::shared_ptr<OsmAnd::OnlineTileSources::Source> OsmAnd::OnlineTileSources_P::createSimpleTileSourceTemplate(const QXmlStreamAttributes &attributes, const QString &rule)
{
    QString urlTemplate = attributes.value(QStringLiteral("url_template")).toString();
    QString name = attributes.value(QStringLiteral("name")).toString();
    
    bool ellipticCorrection = attributes.value(QStringLiteral("ellipsoid")).toString() == QStringLiteral("true");
    if (name.isEmpty() || urlTemplate.isEmpty())
        return nullptr;
    
    urlTemplate = OnlineTileSources::normalizeUrl(urlTemplate);
    
    std::shared_ptr<Source> source(new Source(name));
    
    source->urlToLoad = urlTemplate;
    source->maxZoom = ZoomLevel(parseInt(attributes, QStringLiteral("max_zoom"), 18));
    source->minZoom = ZoomLevel(parseInt(attributes, QStringLiteral("min_zoom"), 5));
    source->tileSize = parseInt(attributes, QStringLiteral("tile_size"), 256);
    long expirationTimeMinutes = parseLong(attributes, QStringLiteral("expiration_time_minutes"), -1);
    if (expirationTimeMinutes > 0)
        source->expirationTimeMillis = expirationTimeMinutes * 60 * 1000l;
    const QString ext = !attributes.hasAttribute(QStringLiteral("ext")) ? QStringLiteral(".jpg") : attributes.value(QStringLiteral("ext")).toString();
    source->ext = ext;
    source->bitDensity = parseInt(attributes, QStringLiteral("img_density"), 16);
    source->avgSize = parseInt(attributes, QStringLiteral("avg_img_size"), 18000);
    source->ellipticYTile = ellipticCorrection /*Always false for now*/;
    source->invertedYTile = attributes.value(QStringLiteral("inverted_y")).toString() == QStringLiteral("true");
    
    const QString randoms = attributes.value(QStringLiteral("randoms")).toString();
    source->randoms = randoms;
    source->randomsArray = OnlineTileSources_P::parseRandoms(randoms);
    source->rule = rule;
    
    return source;
}

QList<QString> OsmAnd::OnlineTileSources_P::parseRandoms(const QString &randoms)
{
    QList<QString> result;
    if (!randoms.isEmpty())
    {
        if (randoms == QStringLiteral("wikimapia"))
        {
            result.append(QStringLiteral("wikimapia"));
            return result;
        }
    
        QStringList valuesArray = randoms.split(QStringLiteral(","));
        for (const auto& s : valuesArray)
        {
            QStringList rangeArray = s.split(QStringLiteral("-"));
            if (rangeArray.count() == 2)
            {
                QString s1 = rangeArray[0];
                QString s2 = rangeArray[1];
                bool rangeValid = false;
                bool ok1, ok2;
                
                int a = s1.toInt(&ok1);
                int b = s2.toInt(&ok2);
                if (ok1 && ok2)
                {
                   if (b > a)
                   {
                        for (int i = a; i <= b; i++)
                        {
                            result.append(QString::number(i));
                        }
                        rangeValid = true;
                    }
                }
                else
                {
                    if (s1.length() == 1 && s2.length() == 1)
                    {
                        char a = s1.at(0).toLatin1();
                        char b = s2.at(0).toLatin1();
                        if (b > a)
                        {
                            for (char i = a; i <= b; i++)
                            {
                                result.append(QString(i));
                            }
                            rangeValid = true;
                        }
                    }
                }
                
                if (!rangeValid)
                {
                    result.append(s1);
                    result.append(s2);
                }
            }
            else
            {
                result.append(s);
            }
        }
    }
    return result;
}

int OsmAnd::OnlineTileSources_P::parseInt(const QXmlStreamAttributes &attributes, const QString attributeName, int defaultValue)
{
    QString val = attributes.value(attributeName).toString();
    bool ok;
    int integerValue = val.toInt(&ok);
    if (!ok)
        return defaultValue;
    
    return integerValue;
}

long OsmAnd::OnlineTileSources_P::parseLong(const QXmlStreamAttributes &attributes, const QString attributeName, long defaultValue)
{
    QString val = attributes.value(attributeName).toString();
    bool ok;
    long longValue = val.toLong(&ok);
    if (!ok)
        return defaultValue;
    
    return longValue;
}

bool OsmAnd::OnlineTileSources_P::deserializeFrom(QXmlStreamReader& xmlReader)
{
    int priority = 0;
    QHash< QString, std::shared_ptr<const Source> > collection;

    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        if (!xmlReader.isStartElement())
            continue;
        const auto tagName = xmlReader.name();
        if (tagName == QLatin1String("tile_source"))
        {
            const auto name = xmlReader.attributes().value(QLatin1String("name")).toString();
            if (collection.contains(name))
            {
                LogPrintf(LogSeverityLevel::Warning,
                    "Ignored duplicate online tile source with name '%s'",
                    qPrintable(name));
                continue;
            }

            auto source = createTileSourceTemplate(xmlReader.attributes());
            if (source != nullptr)
            {
                source->priority = priority++;
                collection.insert(name, source);
            }
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

bool OsmAnd::OnlineTileSources_P::addSource(const std::shared_ptr<const Source>& source)
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

bool OsmAnd::OnlineTileSources_P::createTileSourceTemplate(const QString& metaInfoPath, std::shared_ptr<Source>& source)
{
    QHash<QString, QString> metaInfo;
    
    QFile fileIn(metaInfoPath);
    if (fileIn.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&fileIn);
        while (!in.atEnd())
        {
            QString firstLine = in.readLine();
            if (!in.atEnd())
            {
                QString secondLine = in.readLine();
                if (!firstLine.isNull() && !secondLine.isNull())
                {
                    metaInfo.insert(firstLine.replace(QRegExp(QLatin1String("[\\[\\]]+")), QLatin1String("")), secondLine);
                }
            }
        }
        in.flush();
        fileIn.flush();
        fileIn.close();
    }
    if (metaInfo.count() > 0)
    {
        Source *newSource = new Source(QFileInfo(metaInfoPath).dir().dirName());
        newSource->urlToLoad = metaInfo.value(QStringLiteral("url_template"), QStringLiteral(""));
        newSource->minZoom = ZoomLevel(metaInfo.value(QStringLiteral("min_zoom"), QStringLiteral("5")).toLong());
        newSource->maxZoom = ZoomLevel(metaInfo.value(QStringLiteral("max_zoom"), QStringLiteral("18")).toLong());
        newSource->ellipticYTile = metaInfo.value(QStringLiteral("ellipsoid"), QStringLiteral("")) == QStringLiteral("true");
        newSource->invertedYTile = metaInfo.value(QStringLiteral("inverted_y"), QStringLiteral("")) == QStringLiteral("true");
        newSource->tileSize = metaInfo.value(QStringLiteral("tile_size"), QStringLiteral("256")).toUInt();
        newSource->bitDensity = metaInfo.value(QStringLiteral("img_density"), QStringLiteral("16")).toUInt();
        newSource->avgSize = metaInfo.value(QStringLiteral("avg_img_size"), QStringLiteral("18000")).toUInt();
        newSource->ext = metaInfo.value(QStringLiteral("ext"), QStringLiteral(".jpg"));
        newSource->expirationTimeMillis = metaInfo.value(QStringLiteral("expiration_time_minutes"), QStringLiteral("-1")).toLong();
        newSource->randoms = metaInfo.value(QStringLiteral("randoms"), QStringLiteral(""));
        newSource->randomsArray = OnlineTileSources_P::parseRandoms(newSource->randoms);
        source.reset(newSource);
        return true;
    }
    return false;
}

void OsmAnd::OnlineTileSources_P::installTileSource(const std::shared_ptr<const OnlineTileSources::Source> toInstall, const QString& cachePath)
{
    QHash<QString, QString> params;
    params.insert(QStringLiteral("url_template"), toInstall->urlToLoad);
    params.insert(QStringLiteral("min_zoom"), QString::number(toInstall->minZoom));
    params.insert(QStringLiteral("max_zoom"), QString::number(toInstall->maxZoom));
    params.insert(QStringLiteral("ellipsoid"), toInstall->ellipticYTile ? QStringLiteral("true") : QStringLiteral("false"));
    params.insert(QStringLiteral("tile_size"), QString::number(toInstall->tileSize));
    params.insert(QStringLiteral("img_density"), QString::number(toInstall->bitDensity));
    params.insert(QStringLiteral("avg_img_size"), QString::number(toInstall->avgSize));
    params.insert(QStringLiteral("ext"), toInstall->ext);
    params.insert(QStringLiteral("randoms"), toInstall->randoms);
    params.insert(QStringLiteral("inverted_y"), toInstall->invertedYTile ? QStringLiteral("true") : QStringLiteral("false"));
    if (toInstall->expirationTimeMillis != -1)
        params.insert(QStringLiteral("expiration_time_minutes"), QString::number(toInstall->expirationTimeMillis));
    
    QString name = toInstall->name;
    QString path = cachePath + QDir::separator() + name;
    QDir dir;
    if (!dir.exists(path))
        dir.mkpath(path);
    
    QFile fileOut(path + QDir::separator() + QStringLiteral(".metainfo"));
    
    // remove old metainfo file
    if (fileOut.exists())
        fileOut.remove();
    
    if (fileOut.open(QFile::WriteOnly|QFile::Text))
    {
        QTextStream out(&fileOut);
        QHashIterator<QString, QString> i (params);
        while (i.hasNext())
        {
            i.next();
            out << QStringLiteral("[") << i.key() << QStringLiteral("]") << endl << i.value() << endl;
        }
        fileOut.flush();
        fileOut.close();
    }
}

std::shared_ptr<OsmAnd::OnlineTileSources> OsmAnd::OnlineTileSources_P::_builtIn;
std::shared_ptr<const OsmAnd::OnlineTileSources> OsmAnd::OnlineTileSources_P::getBuiltIn()
{
    static QMutex mutex;
    QMutexLocker scopedLocker(&mutex);
    if (!_builtIn)
    {
        bool ok = true;
        _builtIn.reset(new OnlineTileSources());
        _builtIn->loadFrom(getCoreResourcesProvider()->getResource(
            QLatin1String("misc/default.online_tile_sources.xml"),
            &ok));
        assert(ok);
    }
    return _builtIn;
}
