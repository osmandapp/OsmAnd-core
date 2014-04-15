#ifndef _OSMAND_CORE_EMBEDDED_RESOURCES_PRIVATE_H_
#define _OSMAND_CORE_EMBEDDED_RESOURCES_PRIVATE_H_

namespace OsmAnd
{
    struct EmbeddedResource
    {
        QString name;
        size_t size;
        const uint8_t* data;
    };

    extern const EmbeddedResource __bundled_resources[];
    extern const uint32_t __bundled_resources_count;
}

#endif // !defined(_OSMAND_CORE_EMBEDDED_RESOURCES_PRIVATE_H_)
