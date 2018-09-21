#include "CachedOsmandIndexes.h"

#include <chrono>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

#include <OsmAndCore/Data/ObfMapSectionInfo.h>
#include <OsmAndCore/Data/ObfPoiSectionInfo.h>
#include <OsmAndCore/Data/ObfAddressSectionInfo.h>
#include <OsmAndCore/Data/ObfRoutingSectionInfo.h>
#include <OsmAndCore/Data/ObfTransportSectionInfo.h>

/*
 std::string outputName = "ind.cache";
 #if defined(_WIN32)
 int outputFileDescriptor = open(outputName.c_str(), O_RDWR | O_BINARY);
 #else
 int outputFileDescriptor = open(outputName.c_str(), O_RDWR);
 #endif
 if (outputFileDescriptor < 0) {
 OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Cache file could not be written : %s", outputName.c_str());
 return false;
 }
 FileOutputStream output(outputFileDescriptor);
 CodedOutputStream cos(&output);
 c->SerializeToCodedStream(&cos);
 FileIndex* fileIndex = c->add_fileindex();
 */

OsmAnd::CachedOsmandIndexes::CachedOsmandIndexes()
    : storedIndex(nullptr)
    , hasChanged(true)
{
}

OsmAnd::CachedOsmandIndexes::~CachedOsmandIndexes()
{
}

void OsmAnd::CachedOsmandIndexes::addToCache(const std::shared_ptr<const ObfFile>& file)
{
    hasChanged = true;
    if (storedIndex == nullptr)
    {
        storedIndex = std::make_shared<OBF::OsmAndStoredIndex>();
        storedIndex->set_version(VERSION);
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        storedIndex->set_datecreated(millis);
    }
    
    const auto& fileInfo = QFileInfo(file->filePath);
    auto obfInfo = file->obfInfo;
    
    auto fileIndex = storedIndex->add_fileindex();
    auto d = obfInfo->creationTimestamp;
    if (d == 0)
    {
        auto lastModified = fileInfo.lastModified().toMSecsSinceEpoch();
        fileIndex->set_datemodified(lastModified);
    }
    else
    {
        fileIndex->set_datemodified(d);
    }
    fileIndex->set_size(file->fileSize);
    fileIndex->set_version(obfInfo->version);
    fileIndex->set_filename(fileInfo.fileName().toStdString());
    for(auto& index : obfInfo->mapSections)
    {
        auto map = fileIndex->add_mapindex();
        map->set_size(index->length);
        map->set_offset(index->offset);
        map->set_name(index->name.toStdString());
        
        for (auto& mr : index->levels)
        {
            auto lev = map->add_levels();
            lev->set_size(mr->length);
            lev->set_offset(mr->offset);
            lev->set_left(mr->area31.left());
            lev->set_right(mr->area31.right());
            lev->set_top(mr->area31.top());
            lev->set_bottom(mr->area31.bottom());
            lev->set_minzoom(mr->minZoom);
            lev->set_maxzoom(mr->maxZoom);
        }
    }
    
    for (auto& index : obfInfo->addressSections)
    {
        auto addr = fileIndex->add_addressindex();
        addr->set_size(index->length);
        addr->set_offset(index->offset);
        addr->set_name(index->name.toStdString());
        if (index->localizedNames.contains(QLatin1String("en"))) {
            addr->set_nameen(index->localizedNames[QLatin1String("en")].toStdString());
        }
        addr->set_indexnameoffset(index->nameIndexInnerOffset + index->offset);
        for (auto& mr : index->cities)
        {
            auto cblock = addr->add_cities();
            cblock->set_size(mr->length);
            cblock->set_offset(mr->offset);
            cblock->set_type(mr->type);
        }
        for (const auto& s : index->attributeTagsTable)
            addr->add_additionaltags(s.toStdString());
    }
    
    for (auto index : obfInfo->poiSections)
    {
        auto poi = fileIndex->add_poiindex();
        poi->set_size(index->length);
        poi->set_offset(index->offset);
        poi->set_name(index->name.toStdString());
        poi->set_left(index->area31.left());
        poi->set_right(index->area31.right());
        poi->set_top(index->area31.top());
        poi->set_bottom(index->area31.bottom());
    }
    
    for (auto index : obfInfo->transportSections)
    {
        auto transport = fileIndex->add_transportindex();
        transport->set_size(index->length);
        transport->set_offset(index->offset);
        transport->set_name(index->name.toStdString());
        transport->set_left(index->area31.left());
        transport->set_right(index->area31.right());
        transport->set_top(index->area31.top());
        transport->set_bottom(index->area31.bottom());
        transport->set_stopstablelength(index->stopsLength);
        transport->set_stopstableoffset(index->stopsOffset);
        transport->set_stringtablelength(index->stringTable->length);
        transport->set_stringtableoffset(index->stringTable->fileOffset);
    }
    
    for (auto index : obfInfo->routingSections)
    {
        auto routing = fileIndex->add_routingindex();
        routing->set_size(index->length);
        routing->set_offset(index->offset);
        routing->set_name(index->name.toStdString());

        /*
        for (auto sub : index->getSubregions()) {
            addRouteSubregion(routing, sub, false);
        }
        for(RouteSubregion sub : index.getBaseSubregions()) {
            addRouteSubregion(routing, sub, true);
        }
         */
    }
}

void OsmAnd::CachedOsmandIndexes::addRouteSubregion(const std::shared_ptr<const OBF::RoutingPart>& routing, const std::shared_ptr<const OBF::RoutingSubregion>& sub, bool base)
{
    
}

std::shared_ptr<const OsmAnd::ObfInfo> OsmAnd::CachedOsmandIndexes::initFileIndex(const std::shared_ptr<const OBF::FileIndex>& found)
{
    auto obfInfo = std::make_shared<ObfInfo>();
    obfInfo->version = found->version();
    obfInfo->creationTimestamp = found->datemodified();
    
    for (int i = 0; i < found->mapindex_size(); i++)
    {
        auto index = found->mapindex(i);
        auto mi = std::make_shared<ObfMapSectionInfo>(obfInfo);
        mi->length = index.size();
        mi->offset = index.offset();
        mi->name = QString::fromStdString(index.name());
        
        for (int m = 0; m < index.levels_size(); m++) {
            auto mr = index.levels(m);
            auto root = std::make_shared<ObfMapSectionLevel>();
            root->length = mr.size();
            root->offset = mr.offset();
            root->area31 = AreaI(mr.top(), mr.left(), mr.bottom(), mr.right());
            root->minZoom = (ZoomLevel) mr.minzoom();
            root->maxZoom = (ZoomLevel) mr.maxzoom();
            mi->levels.push_back(qMove(root));
        }
        obfInfo->isBasemap = obfInfo->isBasemap || mi->isBasemap;
        obfInfo->mapSections.push_back(qMove(mi));
    }
    
    for (int i = 0; i < found->addressindex_size(); i++)
    {
        auto index = found->addressindex(i);
        auto mi = std::make_shared<ObfAddressSectionInfo>(obfInfo);
        mi->length = index.size();
        mi->offset = index.offset();
        mi->name = QString::fromStdString(index.name());
        mi->localizedNames.insert(QLatin1String("en"), QString::fromStdString(index.nameen()));
        mi->nameIndexInnerOffset = index.indexnameoffset() - mi->offset;
        for (int m = 0; m < index.cities_size(); m++) {
            auto mr = index.cities(m);
            auto cblock = std::make_shared<ObfAddressSectionInfo::CitiesBlock>(QString::null, mr.offset(), mr.size(), mr.type());
            mi->cities.push_back(cblock);
        }
        for (int m = 0; m < index.additionaltags_size(); m++) {
            const auto& s = index.additionaltags(m);
            mi->attributeTagsTable.push_back(QString::fromStdString(s));
        }
        obfInfo->addressSections.push_back(mi);
    }
    
    for (int i = 0; i < found->poiindex_size(); i++)
    {
        auto index = found->poiindex(i);
        auto mi = std::make_shared<ObfPoiSectionInfo>(obfInfo);
        mi->length = index.size();
        mi->offset = index.offset();
        mi->name = QString::fromStdString(index.name());
        mi->area31 = AreaI(index.top(), index.left(), index.bottom(), index.right());
        obfInfo->poiSections.push_back(mi);
    }
    
    for (int i = 0; i < found->transportindex_size(); i++)
    {
        auto index = found->transportindex(i);
        const std::shared_ptr<ObfTransportSectionInfo> mi(new ObfTransportSectionInfo(obfInfo));
        mi->length = index.size();
        mi->offset = index.offset();
        mi->name = QString::fromStdString(index.name());
        mi->_area31 = AreaI(index.top(), index.left(), index.bottom(), index.right());
        mi->_stopsLength = index.stopstablelength();
        mi->_stopsOffset = index.stopstableoffset();
        ObfTransportSectionInfo::IndexStringTable stringTable;
        stringTable.fileOffset = index.stringtableoffset();
        stringTable.length = index.stringtablelength();
        mi->_stringTable = stringTable;
        obfInfo->transportSections.push_back(mi);
    }
    
    for (int i = 0; i < found->routingindex_size(); i++)
    {
        auto index = found->routingindex(i);
        auto mi = std::make_shared<ObfRoutingSectionInfo>(obfInfo);
        mi->length = index.size();
        mi->offset = index.offset();
        mi->name = QString::fromStdString(index.name());
        /*
        for(RoutingSubregion mr : index.getSubregionsList()) {
            RouteSubregion sub = new RouteSubregion(mi);
            sub.length = (int) mr.getSize();
            sub.filePointer = (int) mr.getOffset();
            sub.left = mr.getLeft();
            sub.right = mr.getRight();
            sub.top = mr.getTop();
            sub.bottom = mr.getBottom();
            sub.shiftToData = mr.getShifToData();
            if(mr.getBasemap()) {
                mi.basesubregions.add(sub);
            } else {
                mi.subregions.add(sub);
            }
        }
         */
        obfInfo->routingSections.push_back(mi);
    }
    
    return obfInfo;
}

const std::shared_ptr<const OsmAnd::ObfFile> OsmAnd::CachedOsmandIndexes::getObfFile(const QString& filePath)
{
    return nullptr;
}

void OsmAnd::CachedOsmandIndexes::readFromFile(const QString& filePath, int version)
{
    
}

void OsmAnd::CachedOsmandIndexes::writeToFile(const QString& filePath)
{
    
}
