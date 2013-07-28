/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
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
#ifndef __MAP_RENDERER_CONFIGURATION_H_
#define __MAP_RENDERER_CONFIGURATION_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class IMapRenderer;
    class MapRenderer;
    struct OSMAND_CORE_API MapRendererConfiguration
    {
        MapRendererConfiguration();
        ~MapRendererConfiguration();

        typedef std::function<void ()> FrameRequestCallback;

        typedef std::function<void ()> BackgroundWorkerPrologue;
        typedef std::function<void ()> BackgroundWorkerEpilogue;

        float displayDensityFactor;

        bool textureAtlasesAllowed;
        uint32_t heightmapPatchesPerSide;

        struct {
            bool enabled;
            BackgroundWorkerPrologue prologue;
            BackgroundWorkerEpilogue epilogue;
        } backgroundWorker;

        FrameRequestCallback frameRequestCallback;

    friend class OsmAnd::IMapRenderer;
    friend class OsmAnd::MapRenderer;
    };

}

#endif // __MAP_RENDERER_CONFIGURATION_H_