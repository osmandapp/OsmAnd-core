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

    // State chain for renew:
    // ... => Ready => Uploading => Uploaded [=> IsBeingUsed] [=> PreparingRenew] => PreparedRenew => Renewing => Uploaded

    // State chain for update:
    // ... => Ready => Uploading => Uploaded [=> IsBeingUsed] => Outdated => Updating => PreparedRenew => Renewing => Uploaded
    // ... => Updating => Uploaded

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

        // Resource is required to be renewed, and it needs to be prepared (cached) first
        PreparingRenew,

        // Resource is ready to be renewed
        PreparedRenew,

        // Resource data is being renewed (reuploaded to GPU)
        Renewing,

        // Resource data is already in GPU, but not up-to-date
        Outdated,

        // Up-to-date data for resource is being requested
        Updating,

        // Up-to-date data was requested and should arrive soon
        RequestedUpdate,

        // Up-to-date data request is being processed
        ProcessingUpdate,

        // Up-to-date data request was canceled while being processed
        UpdatingCancelledWhileBeingProcessed,

        // JustBeforeDeath state is installed just before resource is deallocated completely
        JustBeforeDeath
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_RESOURCE_STATE_H_)
