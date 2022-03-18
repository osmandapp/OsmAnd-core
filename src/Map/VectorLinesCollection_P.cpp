#include "VectorLinesCollection_P.h"
#include "VectorLinesCollection.h"

#include "MapDataProviderHelpers.h"
#include "VectorLine.h"

OsmAnd::VectorLinesCollection_P::VectorLinesCollection_P(VectorLinesCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::VectorLinesCollection_P::~VectorLinesCollection_P()
{
}

QListVectorLine OsmAnd::VectorLinesCollection_P::getLines() const
{
    QReadLocker scopedLocker(&_linesLock);

    return _lines.values();
}

bool OsmAnd::VectorLinesCollection_P::addLine(const std::shared_ptr<VectorLine>& line)
{
    QWriteLocker scopedLocker(&_linesLock);

    const auto key = reinterpret_cast<IMapKeyedSymbolsProvider::Key>(line.get());
    if (_lines.contains(key))
        return false;

    _lines.insert(key, line);

    return true;
}

bool OsmAnd::VectorLinesCollection_P::removeLine(const std::shared_ptr<VectorLine>& line)
{
    QWriteLocker scopedLocker(&_linesLock);

    const bool removed = (_lines.remove(reinterpret_cast<IMapKeyedSymbolsProvider::Key>(line.get())) > 0);
    return removed;
}

void OsmAnd::VectorLinesCollection_P::removeAllLines()
{
    QWriteLocker scopedLocker(&_linesLock);

    _lines.clear();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::VectorLinesCollection_P::getProvidedDataKeys() const
{
    QReadLocker scopedLocker(&_linesLock);

    return _lines.keys();
}

bool OsmAnd::VectorLinesCollection_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData)
{
    const auto& request = MapDataProviderHelpers::castRequest<VectorLinesCollection::Request>(request_);

    QReadLocker scopedLocker(&_linesLock);

    const auto citLine = _lines.constFind(request.key);
    if (citLine == _lines.cend())
        return false;
    auto& line = *citLine;

    outData.reset(new IMapKeyedSymbolsProvider::Data(request.key, line->createSymbolsGroup(request.mapState)));

    return true;
}
