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
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
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
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
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
        auto inches = inchesSubstr.mid(first, last - first + 1).toDouble(&ok);
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
    auto result = value.mid(first, last - first + 1).toDouble(&ok);
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
	result.argb = value.mid(1).toUInt(nullptr, 16);
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
    auto result = value.mid(first, last - first + 1).toInt(&ok);
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
    auto result = value.mid(first, last - first + 1).toLong(&ok);
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
    auto result = value.mid(first, last - first + 1).toUInt(&ok);
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
    auto result = value.mid(first, last - first + 1).toULong(&ok);
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
    auto result = value.mid(first, last - first + 1).toFloat(&ok);
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

void OsmAnd::Utilities::scanlineFillPolygon(const unsigned int verticesCount, const PointF* vertices, std::function<void(const PointI&)> fillPoint)
{
    // Find min-max of Y
    float yMinF, yMaxF;
    yMinF = yMaxF = vertices[0].y;
    for(auto idx = 1u; idx < verticesCount; idx++)
    {
        const auto& y = vertices[idx].y;
        if (y > yMaxF)
            yMaxF = y;
        if (y < yMinF)
            yMinF = y;
    }
    const auto rowMin = qFloor(yMinF);
    const auto rowMax = qFloor(yMaxF);

    // Build set of edges
    struct Edge
    {
        const PointF* v0;
        int startRow;
        const PointF* v1;
        int endRow;
        float xOrigin;
        float slope;
        int nextRow;
    };
    QVector<Edge*> edges;
    edges.reserve(verticesCount);
    auto edgeIdx = 0u;
    for(auto idx = 0u, prevIdx = verticesCount - 1; idx < verticesCount; prevIdx = idx++)
    {
        auto v0 = &vertices[prevIdx];
        auto v1 = &vertices[idx];

        if (v0->y == v1->y)
        {
            // Horizontal edge
            auto edge = new Edge();
            edge->v0 = v0;
            edge->v1 = v1;
            edge->startRow = qFloor(edge->v0->y);
            edge->endRow = qFloor(edge->v1->y);
            edge->xOrigin = edge->v0->x;
            edge->slope = 0;
            edge->nextRow = qFloor(edge->v0->y) + 1;
            edges.push_back(edge);
            //LogPrintf(LogSeverityLevel::Debug, "Edge %p y(%d %d)(%f %f), next row = %d", edge, edge->startRow, edge->endRow, edge->v0->y, edge->v1->y, edge->nextRow);

            continue;
        }

        const PointF* pLower = nullptr;
        const PointF* pUpper = nullptr;
        if (v0->y < v1->y)
        {
            // Up-going edge
            pLower = v0;
            pUpper = v1;
        }
        else if (v0->y > v1->y)
        {
            // Down-going edge
            pLower = v1;
            pUpper = v0;
        }

        // Fill edge 
        auto edge = new Edge();
        edge->v0 = pLower;
        edge->v1 = pUpper;
        edge->startRow = qFloor(edge->v0->y);
        edge->endRow = qFloor(edge->v1->y);
        edge->slope = (edge->v1->x - edge->v0->x) / (edge->v1->y - edge->v0->y);
        edge->xOrigin = edge->v0->x - edge->slope * (edge->v0->y - qFloor(edge->v0->y));
        edge->nextRow = qFloor(edge->v1->y) + 1;
        for(auto vertexIdx = 0u; vertexIdx < verticesCount; vertexIdx++)
        {
            const auto& v = vertices[vertexIdx];

            if (v.y > edge->v0->y && qFloor(v.y) < edge->nextRow)
                edge->nextRow = qFloor(v.y);
        }
        //LogPrintf(LogSeverityLevel::Debug, "Edge %p y(%d %d)(%f %f), next row = %d", edge, edge->startRow, edge->endRow, edge->v0->y, edge->v1->y, edge->nextRow);
        edges.push_back(edge);
    }

    // Sort edges by ascending Y
    qSort(edges.begin(), edges.end(), [](Edge* l, Edge* r) -> bool
    {
        return l->v0->y > r->v0->y;
    });

    // Loop in [yMin .. yMax]
    QVector<Edge*> aet;
    aet.reserve(edges.size());
    for(auto rowIdx = rowMin; rowIdx <= rowMax;)
    {
        //LogPrintf(LogSeverityLevel::Debug, "------------------ %d -----------------", rowIdx);

        // Find active edges
        int nextRow = rowMax;
        for(const auto& edge : constOf(edges))
        {
            const auto isHorizontal = (edge->startRow == edge->endRow);
            if (nextRow > edge->nextRow && edge->nextRow > rowIdx && !isHorizontal)
                nextRow = edge->nextRow;

            if (edge->startRow != rowIdx)
                continue;

            if (isHorizontal)
            {
                // Fill horizontal edge
                const auto xMin = qFloor(qMin(edge->v0->x, edge->v1->x));
                const auto xMax = qFloor(qMax(edge->v0->x, edge->v1->x));
                /*for(auto x = xMin; x <= xMax; x++)
                    fillPoint(PointI(x, rowIdx));*/
                continue;
            }

            //LogPrintf(LogSeverityLevel::Debug, "line %d. Adding edge %p y(%f %f)", rowIdx, edge, edge->v0->y, edge->v1->y);
            aet.push_back(edge);
            continue;
        }

        // If there are no active edges, we've finished filling
        if (aet.isEmpty())
            break;
        assert(aet.size() % 2 == 0);

        // Sort aet by X
        qSort(aet.begin(), aet.end(), [](Edge* l, Edge* r) -> bool
        {
            return l->v0->x > r->v0->x;
        });

        // Find next row
        for(; rowIdx < nextRow; rowIdx++)
        {
            const unsigned int pairsCount = aet.size() / 2;

            auto itEdgeL = aet.cbegin();
            auto itEdgeR = itEdgeL + 1;

            for(auto pairIdx = 0u; pairIdx < pairsCount; pairIdx++, itEdgeL = ++itEdgeR, ++itEdgeR)
            {
                auto lEdge = *itEdgeL;
                auto rEdge = *itEdgeR;

                // Fill from l to r
                auto lXf = lEdge->xOrigin + (rowIdx - lEdge->startRow + 0.5f) * lEdge->slope;
                auto rXf = rEdge->xOrigin + (rowIdx - rEdge->startRow + 0.5f) * rEdge->slope;
                auto xMinF = qMin(lXf, rXf);
                auto xMaxF = qMax(lXf, rXf);
                auto xMin = qFloor(xMinF);
                auto xMax = qFloor(xMaxF);

                LogPrintf(LogSeverityLevel::Debug, "line %d from %d(%f) to %d(%f)", rowIdx, xMin, xMinF, xMax, xMaxF);
                /*for(auto x = xMin; x <= xMax; x++)
                    fillPoint(PointI(x, rowIdx));*/
            }
        }

        // Deactivate those edges that have end at yNext
        for(auto itEdge = aet.begin(); itEdge != aet.end();)
        {
            auto edge = *itEdge;

            if (edge->endRow <= nextRow)
            {
                // When we're done processing the edge, fill it Y-by-X
                auto startCol = qFloor(edge->v0->x);
                auto endCol = qFloor(edge->v1->x);
                auto revSlope = 1.0f / edge->slope;
                auto yOrigin = edge->v0->y - revSlope * (edge->v0->x - qFloor(edge->v0->x));
                auto xMax = qMax(startCol, endCol);
                auto xMin = qMin(startCol, endCol);
                for(auto colIdx = xMin; colIdx <= xMax; colIdx++)
                {
                    auto yf = yOrigin + (colIdx - startCol + 0.5f) * revSlope;
                    auto y = qFloor(yf);

                    LogPrintf(LogSeverityLevel::Debug, "col %d(s%d) added Y = %d (%f)", colIdx, colIdx - startCol, y, yf);
                    fillPoint(PointI(colIdx, y));
                }

                //////////////////////////////////////////////////////////////////////////
                auto yMax = qMax(edge->startRow, edge->endRow);
                auto yMin = qMin(edge->startRow, edge->endRow);
                for(auto rowIdx_ = yMin; rowIdx_ <= yMax; rowIdx_++)
                {
                    auto xf = edge->xOrigin + (rowIdx_ - edge->startRow + 0.5f) * edge->slope;
                    auto x = qFloor(xf);

                    LogPrintf(LogSeverityLevel::Debug, "row %d(s%d) added Y = %d (%f)", rowIdx_, rowIdx_ - edge->startRow, x, xf);
                    fillPoint(PointI(x, rowIdx_));
                }
                //////////////////////////////////////////////////////////////////////////

                //LogPrintf(LogSeverityLevel::Debug, "line %d. Removing edge %p y(%f %f)", rowIdx, edge, edge->v0->y, edge->v1->y);
                itEdge = aet.erase(itEdge);
            }
            else
                ++itEdge;
        }
    }

    // Cleanup
    for(const auto& edge : constOf(edges))
        delete edge;
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
