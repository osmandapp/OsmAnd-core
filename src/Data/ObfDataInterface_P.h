#ifndef _OSMAND_CORE_OBF_DATA_INTERFACE_P_H_
#define _OSMAND_CORE_OBF_DATA_INTERFACE_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>

namespace OsmAnd {

    class ObfReader;

    class ObfDataInterface;
    class ObfDataInterface_P
    {
    private:
    protected:
        ObfDataInterface_P(ObfDataInterface* owner, const QList< std::shared_ptr<ObfReader> >& readers);

        ObfDataInterface* const owner;
        const QList< std::shared_ptr<ObfReader> > readers;
    public:
        virtual ~ObfDataInterface_P();

    friend class OsmAnd::ObfDataInterface;
    };

}

#endif // !defined(_OSMAND_CORE_OBF_DATA_INTERFACE_P_H_)
