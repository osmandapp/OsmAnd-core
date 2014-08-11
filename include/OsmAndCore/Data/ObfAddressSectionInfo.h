#ifndef _OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd
{
    class ObfAddressSectionReader_P;
    class ObfReader_P;
    class ObfAddressBlocksSectionInfo;

    class OSMAND_CORE_API ObfAddressSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfAddressSectionInfo)
    private:
    protected:
        ObfAddressSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        //NOTE:?
        QString _latinName;

        QList< std::shared_ptr<const ObfAddressBlocksSectionInfo> > _addressBlocksSections;
    public:
        virtual ~ObfAddressSectionInfo();

        const QList< std::shared_ptr<const ObfAddressBlocksSectionInfo> >& addressBlocksSections;

        friend class OsmAnd::ObfAddressSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };

    class OSMAND_CORE_API ObfAddressBlocksSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY_AND_MOVE(ObfAddressBlocksSectionInfo)
    private:
    protected:
        ObfAddressBlocksSectionInfo(const std::shared_ptr<const ObfAddressSectionInfo>& addressSection, const std::weak_ptr<ObfInfo>& owner);

        ObfAddressBlockType _type;
    public:
        virtual ~ObfAddressBlocksSectionInfo();

        const ObfAddressBlockType& type;

        friend class OsmAnd::ObfAddressSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_SECTION_INFO_H_)
