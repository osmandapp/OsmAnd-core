#include "ObfFile.h"
#include "ObfFile_P.h"

#include <QFile>
#include <QStringList>
#include <OsmAndCore/Data/ObfInfo.h>
#include <OsmAndCore/Utilities.h>

OsmAnd::ObfFile::ObfFile(const QString& filePath_, const std::shared_ptr<const ObfInfo>& obfInfo_)
    : _p(new ObfFile_P(this, obfInfo_))
    , filePath(filePath_)
    , fileSize(QFile(filePath).size())
    , obfInfo(_p->_obfInfo)
{
}

OsmAnd::ObfFile::ObfFile(const QString& filePath_)
    : _p(new ObfFile_P(this))
    , filePath(filePath_)
    , fileSize(QFile(filePath).size())
    , obfInfo(_p->_obfInfo)
{
}

OsmAnd::ObfFile::ObfFile(const QString& filePath_, const uint64_t fileSize_)
    : _p(new ObfFile_P(this))
    , filePath(filePath_)
    , fileSize(fileSize_)
    , obfInfo(_p->_obfInfo)
{
}

OsmAnd::ObfFile::~ObfFile()
{
}

const QString OsmAnd::ObfFile::getRegionName() const
{
    QStringList rg = obfInfo->getRegionNames();
    if (rg.isEmpty())
    {
        QFileInfo fileInfo(filePath);
        rg << fileInfo.fileName();
    }

    QString ls = rg.first();
    if (ls.lastIndexOf('_') != -1)
        return ls.mid(0, ls.lastIndexOf('_')).replace('_', ' ');

    return ls;
}
