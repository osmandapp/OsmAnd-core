/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_
#define _OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    struct OSMAND_CORE_API MapRendererSetupOptions
    {
        MapRendererSetupOptions();
        ~MapRendererSetupOptions();

        // Background GPU worker is used for uploading/unloading resources from GPU in background
        typedef std::function<void ()> GpuWorkerThreadPrologue;
        typedef std::function<void ()> GpuWorkerThreadEpilogue;
        struct {
            bool enabled;
            GpuWorkerThreadPrologue prologue;
            GpuWorkerThreadEpilogue epilogue;
        } gpuWorkerThread;

        // This callback is called when frame needs update
        typedef std::function<void()> FrameUpdateRequestCallback;
        FrameUpdateRequestCallback frameUpdateRequestCallback;

        float displayDensityFactor;
    };

}

#endif // _OSMAND_CORE_MAP_RENDERER_SETUP_OPTIONS_H_
