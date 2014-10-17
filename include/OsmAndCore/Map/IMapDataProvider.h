#ifndef _OSMAND_CORE_I_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IMapDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IMapDataProvider);
    public:
        struct OSMAND_CORE_API RetainableCacheMetadata
        {
            virtual ~RetainableCacheMetadata() = 0;
        };

        class OSMAND_CORE_API Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        
        private:
        protected:
            void release();
        public:
            Data(const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            // If data provider supports caching, it may need to store some metadata to maintain cache
            std::shared_ptr<const RetainableCacheMetadata> retainableCacheMetadata;
        };

    private:
    protected:
        IMapDataProvider();
    public:
        virtual ~IMapDataProvider();
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_DATA_PROVIDER_H_)
