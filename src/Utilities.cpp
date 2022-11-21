#include "Utilities.h"

#include <cassert>
#include <limits>
#include <cmath>
#include <stdexcept>

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

const uint precisionPower = 10;
const uint precisionDiv = 1 << (31 - precisionPower);

double coefficientsY[1 << precisionPower];
bool initializeYArray = false;
double OsmAnd::Utilities::y31ToMeters(int y1, int y2, int x)
{
    if (!initializeYArray) {
        coefficientsY[0] = 0;
        for (uint i = 0; i < (1 << precisionPower) - 1; i++) {
            coefficientsY[i + 1] =
                coefficientsY[i] + measuredDist31(0, i << (31 - precisionPower), 0, ((i + 1) << (31 - precisionPower)));
        }
        initializeYArray = true;
    }
    uint div1 = y1 / precisionDiv;
    uint mod1 = y1 % precisionDiv;
    uint div2 = y2 / precisionDiv;
    uint mod2 = y2 % precisionDiv;
    double h1;
        if(div1 + 1 >= sizeof(coefficientsY)/sizeof(*coefficientsY)) {
            h1 = coefficientsY[div1] + mod1 / ((double)precisionDiv) * (coefficientsY[div1] - coefficientsY[div1 - 1]);
        } else {
            h1 = coefficientsY[div1] + mod1 / ((double)precisionDiv) * (coefficientsY[div1 + 1] - coefficientsY[div1]);
        }
        double h2 ;
        if(div2 + 1 >= sizeof(coefficientsY)/sizeof(*coefficientsY)) {
            h2 = coefficientsY[div2] + mod2 / ((double)precisionDiv) * (coefficientsY[div2] - coefficientsY[div2 - 1]);
        } else {
            h2 = coefficientsY[div2] + mod2 / ((double)precisionDiv) * (coefficientsY[div2 + 1] - coefficientsY[div2]);
        }
    double res = h1 - h2;
    // OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "ind %f != %f", res,  measuredDist31(x, y1, x, y2));
    return res;
}

double coefficientsX[1 << precisionPower];
bool initializeXArray = false;
double OsmAnd::Utilities::x31ToMeters(int x1, int x2, int y)
{
    if (!initializeXArray) {
        for (uint i = 0; i < (1 << precisionPower); i++) {
            coefficientsX[i] = 0;
        }
        initializeXArray = true;
    }
    int ind = y / precisionDiv;
    if (coefficientsX[ind] == 0) {
        double md = measuredDist31(x1, y, x2, y);
        if (md < 10 || x1 == x2) {
            return md;
        }
        coefficientsX[ind] = md / fabs((double)x1 - (double)x2);
    }
    // translate into meters
    return ((double)x1 - x2) * coefficientsX[ind];
}

int OsmAnd::Utilities::extractFirstInteger(const QString& s)
{
    int i = 0;
    for (QChar k : s)
        if (k.isDigit())
            i = i * 10 + k.digitValue();
        else
            break;
    return i;
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

bool OsmAnd::Utilities::isColorBright(const ColorARGB color)
{
    return (color.r / 255.0) * 0.299 + (color.g / 255.0) * 0.587 + (color.b / 255.0) * 0.114 >= 0.5;
}

bool OsmAnd::Utilities::isColorBright(const ColorRGB color)
{
    return (color.r / 255.0) * 0.299 + (color.g / 255.0) * 0.587 + (color.b / 255.0) * 0.114 >= 0.5;
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

QString OsmAnd::Utilities::resolveColorFromPalette(const QString& input, const bool usePalette6)
{
    auto value = input.toLower();

    bool colorWasParsed = false;
    const auto parsedColor = parseColor(input, ColorARGB(), &colorWasParsed);
    ColorHSV hsv;
    if (colorWasParsed)
        hsv = ColorRGB(parsedColor).toHSV();
    const auto h = hsv.h;
    const auto s = hsv.s * 100.0f;
    const auto v = hsv.v * 100.0f;

    if ((h < 14 && s > 25 && v > 30) ||
        (h > 326 && s > 25 && v > 30) ||
        (h < 16 && s > 10 && s < 25 && v > 90) ||
        (h > 326 && s > 10 && s < 25 && v > 90) ||
        value == QLatin1String("pink") ||
        value.contains("red") ||
        value == QLatin1String("pink/white") ||
        value == QLatin1String("white-red") ||
        value == QLatin1String("ff0000") ||
        value == QLatin1String("800000") ||
        value == QLatin1String("red/tan") ||
        value == QLatin1String("tan/red") ||
        value == QLatin1String("rose"))
    {
        value = QLatin1String("red");
    }
    else if (
        (h >= 16 && h < 50 && s > 25 && v > 20 && v < 60) ||
        value == QLatin1String("brown") ||
        value == QLatin1String("darkbrown") ||
        value == QLatin1String("tan/brown") ||
        value == QLatin1String("tan_brown") ||
        value == QLatin1String("brown/tan") ||
        value == QLatin1String("light_brown") ||
        value == QLatin1String("brown/white") ||
        value == QLatin1String("tan"))
    {
        value = QLatin1String(usePalette6 ? "red" : "brown");
    }
    else if (
        (h >= 14 && h < 42 && v > 60) ||
        value == QLatin1String("orange") ||
        value == QLatin1String("cream") ||
        value == QLatin1String("gold") ||
        value == QLatin1String("yellow-red") ||
        value == QLatin1String("ff8c00") ||
        value == QLatin1String("peach"))
    {
        value = QLatin1String(usePalette6 ? "red" : "orange");
    }
    else if (
        (h >= 42 && h < 73 && s > 30 && v > 80) ||
        value == QLatin1String("yellow") ||
        value == QLatin1String("gelb") ||
        value == QLatin1String("ffff00") ||
        value == QLatin1String("beige") ||
        value == QLatin1String("lightyellow") ||
        value == QLatin1String("jaune"))
    {
        value = QLatin1String("yellow");
    }
    else if ((h >= 42 && h < 73 && s > 30 && v > 60 && v < 80))
    {
        value = QLatin1String(usePalette6 ? "yellow" : "darkyellow");
    }
    else if ((h >= 74 && h < 150 && s > 30 && v > 77) ||
        value == QLatin1String("lightgreen") ||
        value == QLatin1String("lime") ||
        value == QLatin1String("seagreen") ||
        value == QLatin1String("00ff00") ||
        value == QLatin1String("yellow/green"))
    {
        value = QLatin1String(usePalette6 ? "green" : "lightgreen");
    }
    else if (
        (h >= 74 && h < 174 && s > 30 && v > 30 && v < 77) ||
        value.contains("green") ||
        value == QLatin1String("darkgreen") ||
        value == QLatin1String("natural") ||
        value == QLatin1String("natur") ||
        value == QLatin1String("mediumseagreen") ||
        value == QLatin1String("green/white") ||
        value == QLatin1String("white/green") ||
        value == QLatin1String("blue/yellow") ||
        value == QLatin1String("vert") ||
        value == QLatin1String("green/blue") ||
        value == QLatin1String("olive"))
    {
        value = QLatin1String("green");
    }
    else if (
        (h >= 174 && h < 215 && s > 32 && v > 50) ||
        value == QLatin1String("lightblue") ||
        value == QLatin1String("aqua") ||
        value == QLatin1String("cyan") ||
        value == QLatin1String("87ceeb") ||
        value == QLatin1String("turquoise"))
    {
        value = QLatin1String(usePalette6 ? "blue" : "lightblue");
    }
    else if (
        (h >= 215 && h < 265 && s > 40 && v > 30) ||
        value.contains("blue") ||
        value == QLatin1String("0000ff") ||
        value == QLatin1String("teal") ||
        value == QLatin1String("darkblue") ||
        value == QLatin1String("blu") ||
        value == QLatin1String("navy"))
    {
        value = QLatin1String("blue");
    }
    else if (
        (h >= 265 && h < 325 && s > 15 && v >= 27) ||
        (h > 250 && h < 325 && s > 10 && s < 25 && v > 90) ||
        value == QLatin1String("purple") ||
        value == QLatin1String("violet") ||
        value == QLatin1String("magenta") ||
        value == QLatin1String("maroon") ||
        value == QLatin1String("fuchsia") ||
        value == QLatin1String("800080"))
    {
        value = QLatin1String(usePalette6 ? "blue" : "purple");
    }
    else if (
        (colorWasParsed && v < 27) ||
        value.contains("black") ||
        value == QLatin1String("darkgrey"))
    {
        value = QLatin1String("black");
    }
    else if (
        (s < 32 && v > 30 && v < 90) ||
        value == QLatin1String("gray") ||
        value == QLatin1String("grey") ||
        value == QLatin1String("grey/tan") ||
        value == QLatin1String("silver") ||
        value == QLatin1String("srebrny") ||
        value == QLatin1String("lightgrey") ||
        value == QLatin1String("lightgray") ||
        value == QLatin1String("metal"))
    {
        value = QLatin1String(usePalette6 ? "white" : "gray");
    }
    else if (
        (s < 5 && v > 95) ||
        value.contains("white") /*|| value == QLatin1String("white/tan")*/)
    {
        value = QLatin1String("white");
    }
    else if (colorWasParsed)
    {
        value = QLatin1String("gray");
    }

    return value;
}

OsmAnd::TileId OsmAnd::Utilities::getTileId(const PointI& point31, ZoomLevel zoom, PointF* pOutOffsetN /*= nullptr*/, PointI* pOutOffset /*= nullptr*/)
{
    const auto zoomLevelDelta = MaxZoomLevel - zoom;
    const auto tileId = TileId::fromXY(point31.x >> zoomLevelDelta, point31.y >> zoomLevelDelta);

    if (pOutOffsetN || pOutOffset)
    {
        PointI tile31;
        tile31.x = tileId.x << zoomLevelDelta;
        tile31.y = tileId.y << zoomLevelDelta;

        const auto offsetInTile = point31 - tile31;
        if (pOutOffset)
        {
            *pOutOffset = offsetInTile;
        }

        if (pOutOffsetN)
        {
            const auto tileSize31 = 1u << zoomLevelDelta;
            pOutOffsetN->x = static_cast<float>(static_cast<double>(offsetInTile.x) / tileSize31);
            pOutOffsetN->y = static_cast<float>(static_cast<double>(offsetInTile.y) / tileSize31);
        }
    }

    return tileId;
}

OsmAnd::TileId OsmAnd::Utilities::normalizeTileId(const TileId input, const ZoomLevel zoom)
{
    TileId output = input;

    const auto tilesCount = static_cast<int32_t>(1u << zoom);

    while (output.x < 0)
    {
        output.x += tilesCount;
    }
    while (output.y < 0)
    {
        output.y += tilesCount;
    }

    // Max zoom level (31) is skipped, since value stored in int31 can not be more than tilesCount(31)
    if (zoom < ZoomLevel31)
    {
        while (output.x >= tilesCount)
        {
            output.x -= tilesCount;
        }
        while (output.y >= tilesCount)
        {
            output.y -= tilesCount;
        }
    }

    assert(output.x >= 0 && ((zoom < ZoomLevel31 && output.x < tilesCount) || (zoom == ZoomLevel31)));
    assert(output.x >= 0 && ((zoom < ZoomLevel31 && output.y < tilesCount) || (zoom == ZoomLevel31)));

    return output;
}

OsmAnd::PointI OsmAnd::Utilities::normalizeCoordinates(const PointI& input, const ZoomLevel zoom)
{
    PointI output = input;

    const auto tilesCount = static_cast<int32_t>(1u << zoom);

    while (output.x < 0)
    {
        output.x += tilesCount;
    }
    while (output.y < 0)
    {
        output.y += tilesCount;
    }

    // Max zoom level (31) is skipped, since value stored in int31 can not be more than tilesCount(31)
    if (zoom < ZoomLevel31)
    {
        while (output.x >= tilesCount)
        {
            output.x -= tilesCount;
        }
        while (output.y >= tilesCount)
        {
            output.y -= tilesCount;
        }
    }

    assert(output.x >= 0 && ((zoom < ZoomLevel31 && output.x < tilesCount) || (zoom == ZoomLevel31)));
    assert(output.x >= 0 && ((zoom < ZoomLevel31 && output.y < tilesCount) || (zoom == ZoomLevel31)));

    return output;
}

#if !defined(SWIG)
OsmAnd::PointI OsmAnd::Utilities::normalizeCoordinates(const PointI64& input, const ZoomLevel zoom)
{
    PointI64 output = input;

    const auto tilesCount = static_cast<int64_t>(1ull << zoom);

    while (output.x < 0)
        output.x += tilesCount;
    while (output.y < 0)
        output.y += tilesCount;

    while (output.x >= tilesCount)
        output.x -= tilesCount;
    while (output.y >= tilesCount)
        output.y -= tilesCount;

    assert(output.x >= 0 && output.x < tilesCount);
    assert(output.y >= 0 && output.y < tilesCount);

    return PointI(static_cast<int32_t>(output.x), static_cast<int32_t>(output.y));
}
#endif // !defined(SWIG)

/**
 * Returns the destination point having travelled along a rhumb line from ‘this’ point the given
 * distance on the  given bearing.
 *
 * @param   {number} distance - Distance travelled, in same units as earth radius (default: metres).
 * @param   {number} bearing - Bearing in degrees from north.
 * @param   {number} [radius=6371e3] - (Mean) radius of earth (defaults to radius in metres).
 * @returns {LatLon} Destination point.
 *
 * @example
 *     rhumbDestinationPoint(LatLon(51.127, 1.338), 40300, 116.7); // 50.9642°N, 001.8530°E
 */
OsmAnd::LatLon OsmAnd::Utilities::rhumbDestinationPoint(LatLon latLon, double distance, double bearing)
{
    double radius = 6371e3;

    double d = distance / radius; // angular distance in radians
    double phi1 = qDegreesToRadians(latLon.latitude);
    double lambda1 = qDegreesToRadians(latLon.longitude);
    double theta = qDegreesToRadians(bearing);

    double deltaPhi = d * cos(theta);
    double phi2 = phi1 + deltaPhi;

    // check for some daft bugger going past the pole, normalise latitude if so
    //if (ABS(phi2) > M_PI_2)
    //    phi2 = phi2>0 ? M_PI-phi2 : -M_PI-phi2;

    double deltaPsi = log(tan(phi2 / 2 + M_PI_4) / tan(phi1 / 2 + M_PI_4));
    double q = abs(deltaPsi) > 10e-12 ? deltaPhi / deltaPsi : cos(phi1); // E-W course becomes incorrect with 0/0

    double deltalambda = d * sin(theta) / q;
    double lambda2 = lambda1 + deltalambda;

    return LatLon(qRadiansToDegrees(phi2), qRadiansToDegrees(lambda2));
}

OsmAnd::PointD OsmAnd::Utilities::getTileEllipsoidNumberAndOffsetY(int zoom, double latitude, int tileSize)
{
    double E2 = (double) latitude * M_PI / 180;
    long sradiusa = 6378137;
    long sradiusb = 6356752;
    double J2 = (double) sqrt(sradiusa * sradiusa - sradiusb * sradiusb) / sradiusa;
    double M2 = (double) log((1 + sin(E2))
            / (1 - sin(E2))) / 2 - J2 * log((1 + J2 * sin(E2)) / (1 - J2 * sin(E2))) / 2;
    double B2 = getPowZoom(zoom);
    PointD res;
    res.x = B2 / 2 - M2 * B2 / 2 / M_PI;

    auto xTilesCountForThisZoom = (double)(1 << zoom);
    auto yTileNumber = floor(xTilesCountForThisZoom / 2 - M2 * xTilesCountForThisZoom / 2 / M_PI);
    res.y = floor(((xTilesCountForThisZoom / 2 - M2 * xTilesCountForThisZoom / 2 / M_PI) - yTileNumber) * tileSize);
    return res;
}

std::pair<int, int> OsmAnd::Utilities::calculateFinalXYFromBaseAndPrecisionXY(int bazeZoom, int finalZoom, int precisionXY,
                                                                              int xBase, int yBase, bool ignoreNotEnoughPrecision)
{
    int finalX = xBase;
    int finalY = yBase;
    int precisionCalc = precisionXY;
    for (int zoom = bazeZoom; zoom < finalZoom; zoom++)
    {
        if (precisionXY <= 1 && precisionCalc > 0 && !ignoreNotEnoughPrecision)
        {
            throw std::invalid_argument( "Not enough bits to retrieve zoom approximation" );
        }
        finalY = finalY * 2 + (precisionXY & 1);
        finalX = finalX * 2 + ((precisionXY & 2) >> 1);
        precisionXY = precisionXY >> 2;
    }
    return std::make_pair(finalX, finalY);
}

bool OsmAnd::Utilities::calculateIntersection(const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX)
{
    // Calculates intersection between line and bbox in clockwise manner.
    const auto& px = p0.x;
    const auto& py = p0.y;
    const auto& x = p1.x;
    const auto& y = p1.y;
    const auto& leftX = bbox.left();
    const auto& rightX = bbox.right();
    const auto& topY = bbox.top();
    const auto& bottomY = bbox.bottom();

    // firstly try to search if the line goes in
    if (py < topY && y >= topY) {
        int tx = (int)(px + ((double)(x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = topY;
            return true;
        }
    }
    if (py > bottomY && y <= bottomY) {
        int tx = (int)(px + ((double)(x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = bottomY;
            return true;
        }
    }
    if (px < leftX && x >= leftX) {
        int ty = (int)(py + ((double)(y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;
            pX.y = ty;
            return true;
        }

    }
    if (px > rightX && x <= rightX) {
        int ty = (int)(py + ((double)(y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;
            pX.y = ty;
            return true;
        }

    }

    // try to search if point goes out
    if (py > topY && y <= topY) {
        int tx = (int)(px + ((double)(x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = topY;
            return true;
        }
    }
    if (py < bottomY && y >= bottomY) {
        int tx = (int)(px + ((double)(x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = bottomY;
            return true;
        }
    }
    if (px > leftX && x <= leftX) {
        int ty = (int)(py + ((double)(y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;
            pX.y = ty;
            return true;
        }

    }
    if (px < rightX && x >= rightX) {
        int ty = (int)(py + ((double)(y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;
            pX.y = ty;
            return true;
        }

    }

    if (px == rightX || px == leftX) {
        if (py >= topY && py <= bottomY) {
            pX.x = px;
            pX.y = py;
            return true;
        }
    }

    if (py == topY || py == bottomY) {
        if (leftX <= px && px <= rightX) {
            pX.x = px;
            pX.y = py;
            return true;
        }
    }
    
    /*
    if (px == rightX || px == leftX || py == topY || py == bottomY) {
        pX = p0;//b.first = px; b.second = py;
        //        return true;
        // Is it right? to not return anything?
    }
     */
    return false;
}
