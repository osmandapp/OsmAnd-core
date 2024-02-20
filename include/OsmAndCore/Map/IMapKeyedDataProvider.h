#ifndef _OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/IMapDataProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapKeyedDataProvider : public IMapDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapKeyedDataProvider);

    public:
        typedef const void* Key;

        class OSMAND_CORE_API Data : public IMapDataProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(const Key key, const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            Key key;
        };

        struct OSMAND_CORE_API Request : public IMapDataProvider::Request
        {
            Key key;
            
            MapState mapState;

            Request();
            Request(const IMapDataProvider::Request& that);
            virtual ~Request();

            static void copy(Request& dst, const IMapDataProvider::Request& src);
            virtual std::shared_ptr<IMapDataProvider::Request> clone() const Q_DECL_OVERRIDE;

        private:
            typedef IMapDataProvider::Request super;
        protected:
            Request(const Request& that);
        };

    private:
    protected:
        IMapKeyedDataProvider();
    public:
        virtual ~IMapKeyedDataProvider();

        virtual ZoomLevel getMinZoom() const = 0;
        virtual ZoomLevel getMaxZoom() const = 0;
        
        virtual QList<Key> getProvidedDataKeys() const = 0;
        virtual bool obtainKeyedData(
            const Request& request,
            std::shared_ptr<Data>& outKeyedData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_KEYED_DATA_PROVIDER_H_)
