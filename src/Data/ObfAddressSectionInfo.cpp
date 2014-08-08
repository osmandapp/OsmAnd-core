#include "ObfAddressSectionInfo.h"

#include "ignore_warnings_on_external_includes.h"
#include "OBF.pb.h"
#include "restore_internal_warnings.h"

typedef OsmAnd::OBF::OsmAndAddressIndex_CitiesIndex_CitiesType _ObfAddressBlockType;
static_assert(
    _ObfAddressBlockType::OsmAndAddressIndex_CitiesIndex_CitiesType_CitiesOrTowns == static_cast<int>(OsmAnd::ObfAddressBlockType::CitiesOrTowns),
    "OsmAnd::ObfAddressBlockType must match OBF::OsmAndAddressIndex_CitiesIndex_CitiesType");
static_assert(
    _ObfAddressBlockType::OsmAndAddressIndex_CitiesIndex_CitiesType_Postcodes == static_cast<int>(OsmAnd::ObfAddressBlockType::Postcodes),
    "OsmAnd::ObfAddressBlockType must match OBF::OsmAndAddressIndex_CitiesIndex_CitiesType");
static_assert(
    _ObfAddressBlockType::OsmAndAddressIndex_CitiesIndex_CitiesType_Villages == static_cast<int>(OsmAnd::ObfAddressBlockType::Villages),
    "OsmAnd::ObfAddressBlockType must match OBF::OsmAndAddressIndex_CitiesIndex_CitiesType");

OsmAnd::ObfAddressSectionInfo::ObfAddressSectionInfo( const std::weak_ptr<ObfInfo>& owner )
    : ObfSectionInfo(owner)
    , addressBlocksSections(_addressBlocksSections)
{
}

OsmAnd::ObfAddressSectionInfo::~ObfAddressSectionInfo()
{
}

OsmAnd::ObfAddressBlocksSectionInfo::ObfAddressBlocksSectionInfo( const std::shared_ptr<const ObfAddressSectionInfo>& addressSection, const std::weak_ptr<ObfInfo>& owner )
    : ObfSectionInfo(owner)
    , type(_type)
{
}

OsmAnd::ObfAddressBlocksSectionInfo::~ObfAddressBlocksSectionInfo()
{
}
