#ifndef _OSMAND_CORE_OBF_READER_H_
#define _OSMAND_CORE_OBF_READER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/ObfInfo.h>
#include <OsmAndCore/PrivateImplementation.h>

class QIODevice;

namespace OsmAnd
{
    class ObfFile;

    class ObfMapSectionReader;
    class ObfAddressSectionReader;
    class ObfRoutingSectionReader;
    class ObfPoiSectionReader;
    class ObfTransportSectionReader;

    class ObfReader_P;
    class OSMAND_CORE_API ObfReader
    {
        Q_DISABLE_COPY(ObfReader)
    private:
        PrivateImplementation<ObfReader_P> _p;
    protected:
    public:
        ObfReader(const std::shared_ptr<const ObfFile>& obfFile, const bool lockForRead = true);
        ObfReader(const std::shared_ptr<QIODevice>& input);
        virtual ~ObfReader();

        const std::shared_ptr<const ObfFile> obfFile;

        std::shared_ptr<const ObfInfo> obtainInfo() const;

    friend class OsmAnd::ObfMapSectionReader;
    friend class OsmAnd::ObfAddressSectionReader;
    friend class OsmAnd::ObfRoutingSectionReader;
    friend class OsmAnd::ObfPoiSectionReader;
    friend class OsmAnd::ObfTransportSectionReader;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_READER_H_)
