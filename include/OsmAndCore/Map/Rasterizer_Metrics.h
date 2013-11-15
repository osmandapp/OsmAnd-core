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

#ifndef __RASTERIZER_METRICS_H_
#define __RASTERIZER_METRICS_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd {

    namespace Rasterizer_Metrics {

        struct Metric_prepareContext
        {
            inline Metric_prepareContext()
            {
                memset(this, 0, sizeof(Metric_prepareContext));
            }

            // Time spent on objects sorting (seconds)
            float elapsedTimeForSortingObjects;

            // Time spent on polygonizing coastlines (seconds)
            float elapsedTimeForPolygonizingCoastlines;

            // Number of polygonized coastlines
            unsigned int polygonizedCoastlines;

            // Time spent on objects combining
            float elapsedTimeForCombiningObjects;

            // Time spent on obtaining primitives
            float elapsedTimeForObtainingPrimitives;

            // Time spent on Order rules evaluation
            float elapsedTimeForOrderEvaluation;

            // Time spent on Polygon rules evaluation
            float elapsedTimeForPolygonEvaluation;

            // Time spent on Polyline rules evaluation
            float elapsedTimeForPolylineEvaluation;

            // Time spent on Point rules evaluation
            float elapsedTimeForPointEvaluation;

            // Time spent on obtaining primitives symbols
            float elapsedTimeForObtainingPrimitivesSymbols;
        };

    } // namespace Rasterizer_Metrics

} // namespace OsmAnd

#endif // __RASTERIZER_METRICS_H_
