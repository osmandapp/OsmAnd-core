#include "ObfReader.h"
#include "ObfReader_P.h"

#include "ObfFile.h"
#include "ObfFile_P.h"

#include "QIODeviceInputStream.h"
#include "QFileDeviceInputStream.h"

OsmAnd::ObfReader::ObfReader(const std::shared_ptr<const ObfFile>& obfFile_, const bool lockForRead /*= true*/)
    : _p(new ObfReader_P(this))
    , obfFile(obfFile_)
{
    if(lockForRead)
        obfFile->lockForReading();
}

OsmAnd::ObfReader::ObfReader(const std::shared_ptr<QIODevice>& input)
    : _p(new ObfReader_P(this))
{
    _p->_input = input;
}

OsmAnd::ObfReader::~ObfReader()
{
    if(obfFile)
        obfFile->unlockFromReading();
}

std::shared_ptr<const OsmAnd::ObfInfo> OsmAnd::ObfReader::obtainInfo() const
{
    // Check if information is already available
    if(_p->_obfInfo)
        return _p->_obfInfo;

    // Open file for reading (if needed)
    if(!_p->_codedInputStream)
    {
        if(obfFile)
        {
            auto input = new QFile(obfFile->filePath);
            _p->_input.reset(input);
        }

        // Create zero-copy input stream
        gpb::io::ZeroCopyInputStream* zcis = nullptr;
        if(const auto inputAsFileDevice = std::dynamic_pointer_cast<QFileDevice>(_p->_input))
        {
            zcis = new QFileDeviceInputStream(inputAsFileDevice);
        }
        else
        {
            zcis = new QIODeviceInputStream(_p->_input);
        }
        _p->_zeroCopyInputStream.reset(zcis);

        // Create coded input stream wrapper
        const auto cis = new gpb::io::CodedInputStream(zcis);
        cis->SetTotalBytesLimit(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
        _p->_codedInputStream.reset(cis);
    }

    if(obfFile)
    {
        QMutexLocker scopedLock(&obfFile->_p->_obfInfoMutex);

        if(!obfFile->_p->_obfInfo)
        {
            if(!ObfReader_P::readInfo(*_p, obfFile->_p->_obfInfo))
                return std::shared_ptr<const OsmAnd::ObfInfo>();
        }
        _p->_obfInfo = obfFile->_p->_obfInfo;

        return _p->_obfInfo;
    }
    else
    {
        if(!ObfReader_P::readInfo(*_p, _p->_obfInfo))
            return std::shared_ptr<const OsmAnd::ObfInfo>();

        return _p->_obfInfo;
    }
}
