#include "Utilities.h"

#include <cassert>
#include <limits>
#include <cmath>

#include "QtExtensions.h"
#include <QtNumeric>
#include <QtCore>

#include "Logging.h"

OsmAnd::Utilities::Utilities()
{
}

OsmAnd::Utilities::~Utilities()
{
}

bool OsmAnd::Utilities::extractFirstNumberPosition(const QString& value, int& first, int& last, bool allowSigned, bool allowDot)
{
    first = -1;
    last = -1;
    int curPos = 0;
    for(auto itChr = value.cbegin(); itChr != value.cend() && (first == -1 || last == -1); ++itChr, curPos++)
    {
        auto chr = *itChr;
        if (first == -1 && chr.isDigit())
            first = curPos;
        if (last == -1 && first != -1 && !chr.isDigit() && ((allowDot && chr != '.') || !allowDot))
            last = curPos - 1;
    }
    if (first >= 1 && allowSigned && value[first - 1] == '-')
        first -= 1;
    if (first != -1 && last == -1)
        last = value.length() - 1;
    return first != -1;
}

double OsmAnd::Utilities::parseSpeed(const QString& value, double defValue, bool* wasParsed/* = nullptr*/)
{
    if (value == QLatin1String("none"))
    {
        if (wasParsed)
            *wasParsed = true;
        return 40;
    }

    int first, last;
    if (!extractFirstNumberPosition(value, first, last, false, true))
    {
        if (wasParsed)
            *wasParsed = false;
        return defValue;
    }
    bool ok;
    auto result = value.midRef(first, last - first + 1).toDouble(&ok);
    if (wasParsed)
        *wasParsed = ok;
    if (!ok)
        return defValue;

    result /= 3.6;
    if (value.contains(QLatin1String("mph")))
        result *= 1.6;
    return result;
}

double OsmAnd::Utilities::parseLength(const QString& value, double defValue, bool* wasParsed/* = nullptr*/)
{
    int first, last;
    if (wasParsed)
        *wasParsed = false;
    if (!extractFirstNumberPosition(value, first, last, false, true))
        return defValue;
    bool ok;
    auto result = value.midRef(first, last - first + 1).toDouble(&ok);
    if (!ok)
        return defValue;

    if (wasParsed)
        *wasParsed = true;
    if (value.contains(QLatin1String("ft")) || value.contains('"'))
        result *= 0.3048;
    if (value.contains('\''))
    {
        auto inchesSubstr = value.mid(value.indexOf('"') + 1);
        if (!extractFirstNumberPosition(inchesSubstr, first, last, false, true))
        {
            if (wasParsed)
                *wasParsed = false;

            return defValue;
        }
        bool ok;
        auto inches = inchesSubstr.midRef(first, last - first + 1).toDouble(&ok);
        if (ok)
            result += inches * 0.0254;
    }
    return result;
}

double OsmAnd::Utilities::parseWeight(const QString& value, double defValue, bool* wasParsed/* = nullptr*/)
{
    int first, last;
    if (wasParsed)
        *wasParsed = false;
    if (!extractFirstNumberPosition(value, first, last, false, true))
        return defValue;
    bool ok;
    auto result = value.midRef(first, last - first + 1).toDouble(&ok);
    if (!ok)
        return defValue;

    if (wasParsed)
        *wasParsed = true;
    if (value.contains(QLatin1String("lbs")))
        result = (result * 0.4535) / 1000.0; // lbs -> kg -> ton
    return result;
}

OsmAnd::ColorARGB OsmAnd::Utilities::parseColor(const QString& value, const ColorARGB defValue, bool* wasParsed /*= nullptr*/)
{
	if (wasParsed)
		*wasParsed = false;

	if (value.size() < 2 || value[0] != '#')
		return defValue;

	ColorARGB result;
    result.argb = value.midRef(1).toUInt(nullptr, 16);
	if (value.size() <= 7)
		result.setAlpha(0xFF);

	if (wasParsed)
		*wasParsed = true;

	return result;
}

int OsmAnd::Utilities::parseArbitraryInt(const QString& value, int defValue, bool* wasParsed/* = nullptr*/)
{
    int first, last;
    if (wasParsed)
        *wasParsed = false;
    if (!extractFirstNumberPosition(value, first, last, true, false))
        return defValue;
    bool ok;
    auto result = value.midRef(first, last - first + 1).toInt(&ok);
    if (!ok)
        return defValue;

    if (wasParsed)
        *wasParsed = true;
    return result;
}

long OsmAnd::Utilities::parseArbitraryLong(const QString& value, long defValue, bool* wasParsed/* = nullptr*/)
{
    int first, last;
    if (wasParsed)
        *wasParsed = false;
    if (!extractFirstNumberPosition(value, first, last, true, false))
        return defValue;
    bool ok;
    auto result = value.midRef(first, last - first + 1).toLong(&ok);
    if (!ok)
        return defValue;

    if (wasParsed)
        *wasParsed = true;
    return result;
}

unsigned int OsmAnd::Utilities::parseArbitraryUInt(const QString& value, unsigned int defValue, bool* wasParsed/* = nullptr*/)
{
    int first, last;
    if (wasParsed)
        *wasParsed = false;
    if (!extractFirstNumberPosition(value, first, last, false, false))
        return defValue;
    bool ok;
    auto result = value.midRef(first, last - first + 1).toUInt(&ok);
    if (!ok)
        return defValue;

    if (wasParsed)
        *wasParsed = true;
    return result;
}

unsigned long OsmAnd::Utilities::parseArbitraryULong(const QString& value, unsigned long defValue, bool* wasParsed/* = nullptr*/)
{
    int first, last;
    if (wasParsed)
        *wasParsed = false;
    if (!extractFirstNumberPosition(value, first, last, false, false))
        return defValue;
    bool ok;
    auto result = value.midRef(first, last - first + 1).toULong(&ok);
    if (!ok)
        return defValue;

    if (wasParsed)
        *wasParsed = true;
    return result;
}

float OsmAnd::Utilities::parseArbitraryFloat(const QString& value, float defValue, bool* wasParsed /*= nullptr*/)
{
    int first, last;
    if (wasParsed)
        *wasParsed = false;
    if (!extractFirstNumberPosition(value, first, last, true, true))
        return defValue;
    bool ok;
    auto result = value.midRef(first, last - first + 1).toFloat(&ok);
    if (!ok)
        return defValue;

    if (wasParsed)
        *wasParsed = true;
    return result;
}

bool OsmAnd::Utilities::parseArbitraryBool(const QString& value, bool defValue, bool* wasParsed /*= nullptr*/)
{
    if (wasParsed)
        *wasParsed = false;

    if (value.isEmpty())
        return defValue;

    auto result = (value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0);

    if (wasParsed)
        *wasParsed = true;
    return result;
}

int OsmAnd::Utilities::javaDoubleCompare(double l, double r)
{
    const auto lNaN = qIsNaN(l);
    const auto rNaN = qIsNaN(r);
    const auto& li64 = *reinterpret_cast<uint64_t*>(&l);
    const auto& ri64 = *reinterpret_cast<uint64_t*>(&r);
    const auto lPos = (li64 >> 63) == 0;
    const auto rPos = (ri64 >> 63) == 0;
    const auto lZero = (li64 << 1) == 0;
    const auto rZero = (ri64 << 1) == 0;

    // NaN is considered by this method to be equal to itself and greater than all other double values (including +inf).
    if (lNaN && rNaN)
        return 0;
    if (lNaN)
        return +1;
    if (rNaN)
        return -1;

    // 0.0 is considered by this method to be greater than -0.0
    if (lZero && rZero)
    {
        if (lPos && !rPos)
            return -1;
        if (!lPos && rPos)
            return +1;
    }

    // All other cases
    return qCeil(l) - qCeil(r);
}

void OsmAnd::Utilities::findFiles(const QDir& origin, const QStringList& masks, QFileInfoList& files, bool recursively /*= true */)
{
    const auto& fileInfoList = origin.entryInfoList(masks, QDir::Files);
    files.append(fileInfoList);

    if (recursively)
    {
        const auto& subdirs = origin.entryInfoList(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot);
        for(const auto& subdir : constOf(subdirs))
            findFiles(QDir(subdir.absoluteFilePath()), masks, files, recursively);
    }
}

void OsmAnd::Utilities::findDirectories(const QDir& origin, const QStringList& masks, QFileInfoList& directories, bool recursively /*= true */)
{
    const auto& directoriesList = origin.entryInfoList(masks, QDir::AllDirs | QDir::NoDotAndDotDot);
    directories.append(directoriesList);

    if (recursively)
    {
        const auto& subdirs = origin.entryInfoList(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot);
        for(const auto& subdir : constOf(subdirs))
            findDirectories(QDir(subdir.absoluteFilePath()), masks, directories, recursively);
    }
}

QString OsmAnd::Utilities::stringifyZoomLevels(const QSet<ZoomLevel>& zoomLevels)
{
    QString result;

    auto sortedZoomLevels = zoomLevels.values();
    std::sort(sortedZoomLevels);
    bool previousCaptured = false;
    ZoomLevel previousZoomLevel = sortedZoomLevels.first();
    bool range = false;
    ZoomLevel rangeStart;
    for (const auto zoomLevel : sortedZoomLevels)
    {
        if (previousCaptured && static_cast<int>(zoomLevel) == static_cast<int>(previousZoomLevel)+1)
        {
            if (!range)
                rangeStart = previousZoomLevel;
            range = true;
            previousZoomLevel = zoomLevel;
            previousCaptured = true;
        }
        else if (range)
        {
            range = false;
            previousZoomLevel = zoomLevel;
            previousCaptured = true;

            result += QString::fromLatin1("%1-%2, %3, ").arg(rangeStart).arg(previousZoomLevel).arg(zoomLevel);
        }
        else
        {
            previousZoomLevel = zoomLevel;
            previousCaptured = true;

            result += QString::fromLatin1("%1, ").arg(zoomLevel);
        }
    }

    // Process last range
    if (range)
        result += QString::fromLatin1("%1-%2, ").arg(rangeStart).arg(sortedZoomLevels.last());

    if (result.length() > 2)
        result.truncate(result.length() - 2);

    return result;
}

QString OsmAnd::Utilities::getQuadKey(const uint32_t x, const uint32_t y, const uint32_t z)
{
    QString quadkey;
    for (auto level = z; level > 0; level--)
    {
        const auto mask = 1u << (level - 1);
        auto value = '0';
        if ((x & mask) != 0)
            value += 1;
        if ((y & mask) != 0)
            value += 2;
        quadkey += QLatin1Char(value);
    }
    return quadkey;
}

QList<OsmAnd::Utilities::ItemPointOnPath> OsmAnd::Utilities::calculateItemPointsOnPath(
    const float pathLength,
    const float itemLength,
    const float padding /*= 0.0f*/,
    const float spacing /*= 0.0f*/)
{
    QList<OsmAnd::Utilities::ItemPointOnPath> itemPoints;

    int depth = 0;
    unsigned int vCountNext = 2;
    while(
        (pathLength / vCountNext >= (spacing + itemLength) || depth == 0) &&
        pathLength / vCountNext >= (padding + itemLength / 2.0f))
    {
        depth++;
        vCountNext <<= 1;
    }
    for (int priority = 0; priority < depth; priority++)
    {
        int ln = (1u << (priority + 1));
        for (int i = 1; i < ln; i += 2)
        {
            const auto offset = (pathLength * static_cast<float>(i)) / static_cast<float>(ln);
            itemPoints.push_back({ priority, offset, offset / pathLength });
        }
    }

    return itemPoints;
}
