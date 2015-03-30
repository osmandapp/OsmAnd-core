#ifndef _OSMAND_CORE_ADDRESSES_BY_NAME_SEARCH_H_
#define _OSMAND_CORE_ADDRESSES_BY_NAME_SEARCH_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Search/BaseSearch.h>

namespace OsmAnd
{
    class Address;

    class OSMAND_CORE_API AddressesByNameSearch Q_DECL_FINAL : public BaseSearch
    {
        Q_DISABLE_COPY_AND_MOVE(AddressesByNameSearch);
    public:
        struct OSMAND_CORE_API Criteria : public BaseSearch::Criteria
        {
            Criteria();
            virtual ~Criteria();

            QString name;
            ObfAddressStreetGroupTypesMask streetGroupTypesMask;
            bool includeStreets;
        };

        struct OSMAND_CORE_API ResultEntry : public IResultEntry
        {
            ResultEntry();
            virtual ~ResultEntry();

            std::shared_ptr<const Address> address;
        };

    private:
    protected:
    public:
        AddressesByNameSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~AddressesByNameSearch();

        virtual void performSearch(
            const ISearch::Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const IQueryController* const controller = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_ADDRESSES_BY_NAME_SEARCH_H_)
