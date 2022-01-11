#include "CachedOsmandIndexes_P.h"
#include "CachedOsmandIndexes.h"

#include <fcntl.h>

#include "QtExtensions.h"
#include <QFile>
#include <QDateTime>

#include "ignore_warnings_on_external_includes.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "restore_internal_warnings.h"

#include "QIODeviceInputStream.h"
#include "QFileDeviceInputStream.h"
#include "ObfFile.h"
#include "ObfFile_P.h"
#include "ObfInfo.h"
#include "ObfReader.h"
#include "ObfMapSectionInfo.h"
#include "ObfMapSectionReader_P.h"
#include "ObfAddressSectionInfo.h"
#include "ObfAddressSectionReader_P.h"
#include "ObfTransportSectionInfo.h"
#include "ObfTransportSectionReader_P.h"
#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionReader_P.h"
#include "ObfPoiSectionInfo.h"
#include "ObfPoiSectionReader_P.h"
#include "ObfReaderUtilities.h"
#include "Logging.h"
#include "Stopwatch.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"

OsmAnd::CachedOsmandIndexes_P::CachedOsmandIndexes_P(
    CachedOsmandIndexes* const owner_)
    : _storedIndex(nullptr)
    , _hasChanged(true)
    , owner(owner_)
{
}

OsmAnd::CachedOsmandIndexes_P::~CachedOsmandIndexes_P()
{
}

void OsmAnd::CachedOsmandIndexes_P::addToCache(const std::shared_ptr<const ObfFile>& file)
{
    _hasChanged = true;
    if (_storedIndex == nullptr)
    {
        _storedIndex = std::make_shared<OBF::OsmAndStoredIndex>();
        _storedIndex->set_version(CachedOsmandIndexes::VERSION);
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        _storedIndex->set_datecreated(millis);
    }
    
    const auto& fileInfo = QFileInfo(file->filePath);
    auto obfInfo = file->obfInfo;
    
    auto fileIndex = _storedIndex->add_fileindex();
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
        if (index->localizedNames.contains(QLatin1String("en")))
            addr->set_nameen(index->localizedNames[QLatin1String("en")].toStdString());

        addr->set_indexnameoffset(index->nameIndexInnerOffset);
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
        transport->set_incompleterouteslength(index->incompleteRoutesLength);
        transport->set_incompleteroutesoffset(index->incompleteRoutesOffset);
        transport->set_stringtablelength(index->stringTable->length);
        transport->set_stringtableoffset(index->stringTable->fileOffset);
    }
    
    auto obfReader = std::make_shared<const ObfReader>(file);
    for (auto index : obfInfo->routingSections)
    {
        auto routing = fileIndex->add_routingindex();
        routing->set_size(index->length);
        routing->set_offset(index->offset);
        routing->set_name(index->name.toStdString());

        QList<std::shared_ptr<const ObfRoutingSectionLevelTreeNode>> baseNodes;
        OsmAnd::ObfRoutingSectionReader::loadTreeNodes(
            obfReader,
            index,
            RoutingDataLevel::Basemap,
            &baseNodes);

        QList<std::shared_ptr<const ObfRoutingSectionLevelTreeNode>> detailedNodes;
        OsmAnd::ObfRoutingSectionReader::loadTreeNodes(
            obfReader,
            index,
            RoutingDataLevel::Detailed,
            &detailedNodes);

        for (auto node : baseNodes)
            addRouteSubregion(routing, node, true);
        
        for (auto node : detailedNodes)
            addRouteSubregion(routing, node, false);
    }
}

void OsmAnd::CachedOsmandIndexes_P::addRouteSubregion(OBF::RoutingPart* routing, std::shared_ptr<const ObfRoutingSectionLevelTreeNode>& node, bool base)
{
    auto rpart = routing->add_subregions();
    rpart->set_size(node->length);
    rpart->set_offset(node->offset);
    rpart->set_left(node->area31.left());
    rpart->set_right(node->area31.right());
    rpart->set_top(node->area31.top());
    rpart->set_basemap(base);
    rpart->set_bottom(node->area31.bottom());
    if (node->dataOffset > node->offset)
        rpart->set_shiftodata((unsigned int)(node->dataOffset - node->offset));
    else
        rpart->set_shiftodata(0);
}

std::shared_ptr<const OsmAnd::ObfInfo> OsmAnd::CachedOsmandIndexes_P::initFileIndex(const std::shared_ptr<const OBF::FileIndex>& found)
{
    auto obfInfo = std::make_shared<ObfInfo>();
    obfInfo->version = found->version();
    obfInfo->creationTimestamp = found->datemodified();
    
    Nullable<AreaI> globalBBox31;
    
    for (int i = 0; i < found->mapindex_size(); i++)
    {
        auto index = found->mapindex(i);
        Ref<ObfMapSectionInfo> mi(new ObfMapSectionInfo(obfInfo));
        mi->length = (unsigned int) index.size();
        mi->offset = (unsigned int) index.offset();
        mi->name = QString::fromStdString(index.name());
        
        for (int m = 0; m < index.levels_size(); m++) {
            auto mr = index.levels(m);
            Ref<ObfMapSectionLevel> root(new ObfMapSectionLevel());
            root->length = (unsigned int) mr.size();
            root->offset = (unsigned int) mr.offset();
            root->area31 = AreaI(mr.top(), mr.left(), mr.bottom(), mr.right());
            root->minZoom = (ZoomLevel) mr.minzoom();
            root->maxZoom = (ZoomLevel) mr.maxzoom();
            mi->levels.push_back(qMove(root));
            
            if (globalBBox31.isSet())
                globalBBox31->enlargeToInclude(root->area31);
            else
                globalBBox31 = root->area31;
        }
        mi->isBasemap = mi->name.contains(QLatin1String("basemap"), Qt::CaseInsensitive);
        mi->isBasemapWithCoastlines = mi->name == QLatin1String("basemap");
        obfInfo->isBasemap = obfInfo->isBasemap || mi->isBasemap;
        obfInfo->isBasemapWithCoastlines = obfInfo->isBasemapWithCoastlines || mi->isBasemapWithCoastlines;

        obfInfo->mapSections.push_back(qMove(mi));
    }
    
    for (int i = 0; i < found->poiindex_size(); i++)
    {
        auto index = found->poiindex(i);
        Ref<ObfPoiSectionInfo> mi(new ObfPoiSectionInfo(obfInfo));
        mi->length = (unsigned int) index.size();
        mi->offset = (unsigned int) index.offset();
        mi->name = QString::fromStdString(index.name());
        mi->area31 = AreaI(index.top(), index.left(), index.bottom(), index.right());
        obfInfo->poiSections.push_back(qMove(mi));

        if (globalBBox31.isSet())
            globalBBox31->enlargeToInclude(mi->area31);
        else
            globalBBox31 = mi->area31;
    }
    
    for (int i = 0; i < found->transportindex_size(); i++)
    {
        auto index = found->transportindex(i);
        Ref<ObfTransportSectionInfo> mi(new ObfTransportSectionInfo(obfInfo));
        mi->length = (unsigned int) index.size();
        mi->offset = (unsigned int) index.offset();
        mi->name = QString::fromStdString(index.name());
        mi->_area31 = AreaI(index.top(), index.left(), index.bottom(), index.right());
        mi->_stopsLength = index.stopstablelength();
        mi->_stopsOffset = index.stopstableoffset();
        mi->_incompleteRoutesLength = index.incompleterouteslength();
        mi->_incompleteRoutesOffset = index.incompleteroutesoffset();
        ObfTransportSectionInfo::IndexStringTable stringTable;
        stringTable.fileOffset = index.stringtableoffset();
        stringTable.length = index.stringtablelength();
        mi->_stringTable = stringTable;
        obfInfo->transportSections.push_back(qMove(mi));
        
        if (globalBBox31.isSet())
            globalBBox31->enlargeToInclude(mi->_area31);
        else
            globalBBox31 = mi->_area31;
    }
    
    for (int i = 0; i < found->routingindex_size(); i++)
    {
        auto index = found->routingindex(i);
        Ref<ObfRoutingSectionInfo> mi(new ObfRoutingSectionInfo(obfInfo));
        mi->length = (unsigned int) index.size();
        mi->offset = (unsigned int) index.offset();
        mi->name = QString::fromStdString(index.name());
        
        Nullable<AreaI> bbox31;
        for (int k = 0; k < index.subregions_size(); k++)
        {
            auto subregion = index.subregions(k);
            auto nodeBbox31 = AreaI(subregion.top(), subregion.left(), subregion.bottom(), subregion.right());
            if (bbox31.isSet())
                bbox31->enlargeToInclude(nodeBbox31);
            else
                bbox31 = nodeBbox31;
        }
        if (bbox31.isSet())
        {
            mi->area31 = *bbox31;

            if (globalBBox31.isSet())
                globalBBox31->enlargeToInclude(mi->area31);
            else
                globalBBox31 = mi->area31;
        }
        else
        {
            AreaI empty;
            mi->area31 = empty;
        }
        
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
        
        obfInfo->routingSections.push_back(qMove(mi));
    }
    
    for (int i = 0; i < found->addressindex_size(); i++)
    {
        auto index = found->addressindex(i);
        Ref<ObfAddressSectionInfo> mi(new ObfAddressSectionInfo(obfInfo));
        mi->length = (unsigned int) index.size();
        mi->offset = (unsigned int) index.offset();
        if (globalBBox31.isSet())
            mi->area31 = *globalBBox31;

        mi->name = QString::fromStdString(index.name());
        mi->localizedNames.insert(QLatin1String("en"), QString::fromStdString(index.nameen()));
        mi->nameIndexInnerOffset = index.indexnameoffset();
        for (int m = 0; m < index.cities_size(); m++) {
            auto mr = index.cities(m);
            auto cblock = std::make_shared<ObfAddressSectionInfo::CitiesBlock>(QString(), mr.offset(), mr.size(), mr.type());
            mi->cities.push_back(cblock);
        }
        for (int m = 0; m < index.additionaltags_size(); m++) {
            const auto& s = index.additionaltags(m);
            mi->attributeTagsTable.push_back(QString::fromStdString(s));
        }
        obfInfo->addressSections.push_back(qMove(mi));
    }
    
    return obfInfo;
}

const std::shared_ptr<OsmAnd::ObfFile> OsmAnd::CachedOsmandIndexes_P::getObfFile(const QString& filePath)
{
    //RandomAccessFile mf = new RandomAccessFile(f.getPath(), "r");
    QFileInfo f(filePath);
    std::shared_ptr<OBF::FileIndex> found = nullptr;
    if (_storedIndex)
    {
        for (int i = 0; i < _storedIndex->fileindex_size(); i++)
        {
            auto fi = _storedIndex->fileindex(i);
            if (f.size() == fi.size() && f.fileName() == QString(fi.filename().c_str()))
            {
                // f.lastModified() == fi.getDateModified()
                found = std::make_shared<OBF::FileIndex>(qMove(fi));
                break;
            }
        }
    }
    std::shared_ptr<ObfFile> obfFile = nullptr;
    if (!found)
    {
        Stopwatch totalStopwatch(true);
        obfFile = std::make_shared<ObfFile>(filePath);
        if (!ObfReader(obfFile).obtainInfo())
        {
            LogPrintf(LogSeverityLevel::Warning, "Failed to open OBF '%s'", qPrintable(filePath));
        }
        else
        {
            addToCache(obfFile);
            LogPrintf(LogSeverityLevel::Debug, "Initializing OBF '%s' %fs", qPrintable(filePath), totalStopwatch.elapsed());
        }
    }
    else
    {
        auto obfInfo = initFileIndex(found);
        obfFile = std::make_shared<ObfFile>(filePath, obfInfo);
    }
    return obfFile;
}

void OsmAnd::CachedOsmandIndexes_P::readFromFile(const QString& filePath, int version)
{
    int fileDescriptor = open(filePath.toStdString().c_str(), O_RDONLY);
    if (fileDescriptor < 0)
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Cache file could not be open to read: %s", qPrintable(filePath));
        return;
    }
    gpb::io::FileInputStream input(fileDescriptor);
    gpb::io::CodedInputStream cis(&input);
    cis.SetTotalBytesLimit(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    
    Stopwatch totalStopwatch(true);
    
    _storedIndex.reset(new OsmAnd::OBF::OsmAndStoredIndex());
    if (_storedIndex->MergeFromCodedStream(&cis))
    {
        if (_storedIndex->version() != version)
        {
            _storedIndex.reset();
        }
        else
        {
            _hasChanged = false;
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Osmand Cache file initialized %s in %fs", qPrintable(filePath), totalStopwatch.elapsed());
        }
    }
    else
    {
        _storedIndex.reset();
    }
}

void OsmAnd::CachedOsmandIndexes_P::writeToFile(const QString& filePath)
{
    if (_storedIndex && _hasChanged)
    {
        int fileDescriptor = open(filePath.toStdString().c_str(), O_RDWR);
        if (fileDescriptor < 0) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Cache file could not be written: %s", qPrintable(filePath));
            return;
        }
        
        gpb::io::FileOutputStream output(fileDescriptor);
        if (!_storedIndex->SerializeToZeroCopyStream(&output))
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Cache file could not be serialized: %s", qPrintable(filePath));
    }
}

