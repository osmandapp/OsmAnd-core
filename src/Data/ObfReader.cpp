#include "ObfReader.h"
#include "ObfReader_P.h"

#include <QFile>

#include "ObfFile.h"

OsmAnd::ObfReader::ObfReader(const std::shared_ptr<const ObfFile>& obfFile_)
    : _p(new ObfReader_P(this, std::shared_ptr<QIODevice>(new QFile(obfFile_->filePath))))
    , obfFile(obfFile_)
{
    open();
}

OsmAnd::ObfReader::ObfReader(const std::shared_ptr<QIODevice>& input)
    : _p(new ObfReader_P(this, input))
{
    open();
}

OsmAnd::ObfReader::~ObfReader()
{
    close();
}

bool OsmAnd::ObfReader::isOpened() const
{
    return _p->isOpened();
}

bool OsmAnd::ObfReader::open()
{
    return _p->open();
}

bool OsmAnd::ObfReader::close()
{
    return _p->close();
}

std::shared_ptr<const OsmAnd::ObfInfo> OsmAnd::ObfReader::obtainInfo() const
{
    return _p->obtainInfo();
}

std::shared_ptr<OsmAnd::gpb::io::CodedInputStream> OsmAnd::ObfReader::getCodedInputStream() const
{
    return _p->getCodedInputStream();
}
