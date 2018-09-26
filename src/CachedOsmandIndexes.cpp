#include "CachedOsmandIndexes.h"
#include "CachedOsmandIndexes_P.h"

#include <chrono>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

#include <OsmAndCore/Data/ObfMapSectionInfo.h>
#include <OsmAndCore/Data/ObfPoiSectionInfo.h>
#include <OsmAndCore/Data/ObfAddressSectionInfo.h>
#include <OsmAndCore/Data/ObfRoutingSectionInfo.h>
#include <OsmAndCore/Data/ObfTransportSectionInfo.h>

OsmAnd::CachedOsmandIndexes::CachedOsmandIndexes(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : _p(new CachedOsmandIndexes_P(this))
    , obfsCollection(obfsCollection_)
{
}

OsmAnd::CachedOsmandIndexes::~CachedOsmandIndexes()
{
}

const std::shared_ptr<const OsmAnd::ObfFile> OsmAnd::CachedOsmandIndexes::getObfFile(const QString& filePath)
{
    return _p->getObfFile(filePath);
}

void OsmAnd::CachedOsmandIndexes::readFromFile(const QString& filePath, int version)
{
    _p->readFromFile(filePath, version);
}

void OsmAnd::CachedOsmandIndexes::writeToFile(const QString& filePath)
{
    _p->writeToFile(filePath);
}
