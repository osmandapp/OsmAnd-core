#ifndef _OSMAND_CORE_OBF_INFO_H_
#define _OSMAND_CORE_OBF_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>

class QIODevice;

namespace OsmAnd
{
    class ObfReader;
    class ObfReader_P;

    class ObfMapSectionInfo;
    class ObfAddressSectionInfo;
    class ObfRoutingSectionInfo;
    class ObfPoiSectionInfo;
    class ObfTransportSectionInfo;

    class OSMAND_CORE_API ObfInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfInfo)
    private:
    protected:
        ObfInfo();

        int _version;
        uint64_t _creationTimestamp;
        bool _isBasemap;

        QList< std::shared_ptr<ObfMapSectionInfo> > _mapSections;
        QList< std::shared_ptr<ObfAddressSectionInfo> > _addressSections;
        QList< std::shared_ptr<ObfRoutingSectionInfo> > _routingSections;
        QList< std::shared_ptr<ObfPoiSectionInfo> > _poiSections;
        QList< std::shared_ptr<ObfTransportSectionInfo> > _transportSections;
    public:
        virtual ~ObfInfo();

        const int& version;
        const uint64_t& creationTimestamp;
        const bool& isBasemap;

        const QList< std::shared_ptr<ObfMapSectionInfo> >& mapSections;
        const QList< std::shared_ptr<ObfAddressSectionInfo> >& addressSections;
        const QList< std::shared_ptr<ObfRoutingSectionInfo> >& routingSections;
        const QList< std::shared_ptr<ObfPoiSectionInfo> >& poiSections;
        const QList< std::shared_ptr<ObfTransportSectionInfo> >& transportSections;

    friend class OsmAnd::ObfReader;
    friend class OsmAnd::ObfReader_P;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_INFO_H_)
