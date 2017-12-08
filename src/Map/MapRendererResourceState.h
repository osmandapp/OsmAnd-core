#ifndef _OSMAND_CORE_MAP_RENDERER_RESOURCE_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_RESOURCE_STATE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"

namespace OsmAnd
{
    // Possible state chains:
    // Unknown => Requesting => Requested => ProcessingRequest => ...
    // ... => Unavailable => JustBeforeDeath
    // ... => Ready => Uploading => Uploaded [=> IsBeingUsed] => UnloadPending => Unloading => Unloaded => JustBeforeDeath
    // ... => RequestCanceledWhileBeingProcessed => JustBeforeDeath

    // State chains for renew:
    // ... => Ready => Uploading => Uploaded [=> IsBeingUsed] => UnloadPendingForRenew => UnloadingForRenew => Ready

    enum class MapRendererResourceState
    {
        // Resource is not in any determined state (resource entry did not exist)
        Unknown = 0,

        // Resource is being requested
        Requesting,

        // Resource was requested and should arrive soon
        Requested,

        // Resource request is being processed
        ProcessingRequest,

        // Request was canceled while being processed
        RequestCanceledWhileBeingProcessed,

        // Resource is not available at all
        Unavailable,

        // Resource data is in main memory, but not yet uploaded to GPU
        Ready,

        // Resource data is being uploaded to GPU
        Uploading,

        // Resource data is already in GPU
        Uploaded,

        // Resource data in GPU is being used
        IsBeingUsed,

        // Resource is no longer needed, and it needs to be unloaded from GPU
        UnloadPending,

        // Resource data is being removed from GPU
        Unloading,

        // Resource is unloaded, next state is "Dead"
        Unloaded,

        // Resource is required to be renewed, and it needs to be unloaded from GPU first
        UnloadPendingForRenew,

        // Resource data is being removed from GPU before renewing
        UnloadingForRenew,

        // JustBeforeDeath state is installed just before resource is deallocated completely
        JustBeforeDeath
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RESOURCE_STATE_H_)
