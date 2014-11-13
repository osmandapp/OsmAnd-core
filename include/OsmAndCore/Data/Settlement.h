#ifndef _OSMAND_CORE_MODEL_SETTLEMENT_H_
#define _OSMAND_CORE_MODEL_SETTLEMENT_H_

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/StreetGroup.h>

namespace OsmAnd {

    class ObfAddressSectionReader_P;

    class OSMAND_CORE_API Settlement : public StreetGroup
    {
    private:
    protected:
        Settlement();
    public:
        virtual ~Settlement();

        friend class OsmAnd::ObfAddressSectionReader_P;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MODEL_SETTLEMENT_H_)
