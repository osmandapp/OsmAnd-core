#include "ObfReader.h"
#include "ObfReader_P.h"

#include "ObfFile.h"
#include "ObfFile_P.h"

#include "QIODeviceInputStream.h"
#include "QFileDeviceInputStream.h"

OsmAnd::ObfReader::ObfReader( const std::shared_ptr<const ObfFile>& obfFile_ )
    : _d(new ObfReader_P(this))
    , obfFile(obfFile_)
{
}

OsmAnd::ObfReader::ObfReader( const std::shared_ptr<QIODevice>& input )
    : _d(new ObfReader_P(this))
{
    _d->_input = input;
}

OsmAnd::ObfReader::~ObfReader()
{
}

std::shared_ptr<OsmAnd::ObfInfo> OsmAnd::ObfReader::obtainInfo() const
{
    // Check if information is already available
    if(_d->_obfInfo)
        return _d->_obfInfo;

    // Open file for reading (if needed)
    if(!_d->_codedInputStream)
    {
        if(obfFile)
        {
            auto input = new QFile(obfFile->filePath);
            _d->_input.reset(input);
        }

        // Create zero-copy input stream
        gpb::io::ZeroCopyInputStream* zcis = nullptr;
        if(const auto inputAsFileDevice = std::dynamic_pointer_cast<QFileDevice>(_d->_input))
        {
            zcis = new QFileDeviceInputStream(inputAsFileDevice);
        }
        else
        {
            zcis = new QIODeviceInputStream(_d->_input);
        }
        _d->_zeroCopyInputStream.reset(zcis);

        // Create coded input stream wrapper
        const auto cis = new gpb::io::CodedInputStream(zcis);
        cis->SetTotalBytesLimit(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
        _d->_codedInputStream.reset(cis);
    }

    if(obfFile)
    {
        QMutexLocker scopedLock(&obfFile->_d->_obfInfoMutex);

        if(!obfFile->_d->_obfInfo)
        {
            const std::shared_ptr<ObfInfo> obfInfo(new ObfInfo());
            ObfReader_P::readInfo(_d, obfInfo);
            obfFile->_d->_obfInfo = qMove(obfInfo);
        }
        _d->_obfInfo = obfFile->_d->_obfInfo;

        return _d->_obfInfo;
    }
    else
    {
        const std::shared_ptr<ObfInfo> obfInfo(new ObfInfo());
        ObfReader_P::readInfo(_d, obfInfo);
        _d->_obfInfo = qMove(obfInfo);

        return _d->_obfInfo;
    }
}
