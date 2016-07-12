#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd
{
    class ObfAddressSectionReader_P;

    class OSMAND_CORE_API ObfAddressSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfAddressSectionInfo);
    private:
    protected:
    public:
        ObfAddressSectionInfo(const std::shared_ptr<const ObfInfo>& container);
        virtual ~ObfAddressSectionInfo();

        AreaI area31;
        QHash<QString, QString> localizedNames;

        uint32_t nameIndexInnerOffset;
        uint32_t firstStreetGroupInnerOffset;

    friend class OsmAnd::ObfAddressSectionReader_P;
    };

    /*class OSMAND_CORE_API ObfAddressBlocksSectionInfo : public ObfSectionInfo
    {
    Q_DISABLE_COPY_AND_MOVE(ObfAddressBlocksSectionInfo);
    private:
    protected:
    ObfAddressBlocksSectionInfo(const std::shared_ptr<const ObfAddressSectionInfo>& addressSection, const std::shared_ptr<const ObfInfo>& container);

    ObfAddressBlockType _type;
    public:
    virtual ~ObfAddressBlocksSectionInfo();

    const ObfAddressBlockType& type;

    friend class OsmAnd::ObfAddressSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };*/
}

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_)
