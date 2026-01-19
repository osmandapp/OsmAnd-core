#pragma once

#include "LineSegment.h"
#include <vector>
#include <iterator>
#include <cassert>
#include <OsmAndCore/Map/VectorMapSymbol.h>
#include <OsmAndCore/Color.h>

namespace crushedpixel {

class Polyline2D {
public:
	enum class JointStyle {
		/**
		 * Corners are drawn with sharp joints.
		 * If the joint's outer angle is too large,
		 * the joint is drawn as beveled instead,
		 * to avoid the miter extending too far out.
		 */
		MITER,
		/**
		 * Corners are flattened.
		 */
		BEVEL,
		/**
		 * Corners are rounded off.
		 */
		ROUND
	};

	enum class EndCapStyle {
		/**
		 * Path ends are drawn flat,
		 * and don't exceed the actual end point.
		 */
		BUTT, // lol
		/**
		 * Path ends are drawn flat,
		 * but extended beyond the end point
		 * by half the line thickness.
		 */
		SQUARE,
		/**
		 * Path ends are rounded off.
		 */
		ROUND,
		/**
		 * Path ends are connected according to the JointStyle.
		 * When using this EndCapStyle, don't specify the common start/end point twice,
		 * as Polyline2D connects the first and last input point itself.
		 */
		JOINT,
        /**
         * Path end has an arrow head, path start is rounded off.
         */
        ARROW
	};

	/**
	 * Creates a vector of vertices describing a solid path through the input points.
	 * @param points The points of the path.
	 * @param thickness The path's thickness.
	 * @param outline The outline's thickness.
	 * @param heights The path's distance map (in meters).
	 * @param fillColor The path's color.
	 * @param nearOutlineColor The outline's inner color/filter (see outlineColorizationMapping).
	 * @param farOutlineColor The outline's outer color/filter (see outlineColorizationMapping).
	 * @param colorizationMapping The path's color map.
	 * @param outlineColorizationMapping The outline's color map (inner and outer colors are used as filters).
	 * @param heights The path's height map (in meters).
	 * @param startY31 Starting Y-coordinate of the path if thickness adjustment is needed (NaN elsewhere).
	 * @param flatten Factor, that reduces the impact of thickness adjustment.
	 * @param jointStyle The path's joint style.
	 * @param startCapStyle The path's start cap style.
	 * @param endCapStyle The path's end cap style.
	 * @param solidColors When set to "true" the path and outline color maps are used as solid colors for segments.
	 * @return The vertices describing the path.
	 * @tparam Vec2 The vector type to use for the vertices.
	 *              Must have public non-const float fields "x" and "y".
	 *              Must have a two-args constructor taking x and y values.
	 *              See crushedpixel::Vec2 for a type that satisfies these requirements.
	 * @tparam InputCollection The collection type of the input points.
	 *                         Must contain elements of type Vec2.
	 *                         Must expose size() and operator[] functions.
	 */
	template<typename Vertex, typename InputCollection>
    static std::vector<OsmAnd::VectorMapSymbol::Vertex> create(OsmAnd::VectorMapSymbol::Vertex &vertex,
                                                               const InputCollection &points,
                                                               float thickness,
                                                               float outline,
                                                               QList<float> &distances,
                                                               OsmAnd::FColorARGB &fillColor,
                                                               OsmAnd::FColorARGB &nearOutlineColor,
                                                               OsmAnd::FColorARGB &farOutlineColor,
                                                               QList<OsmAnd::FColorARGB> &colorizationMapping,
                                                               QList<OsmAnd::FColorARGB> &outlineColorizationMapping,
                                                               QList<float> &heights,
                                                               double startY31,
                                                               float flatten,
                                                               JointStyle jointStyle = JointStyle::MITER,
                                                               EndCapStyle startCapStyle = EndCapStyle::BUTT,
                                                               EndCapStyle endCapStyle = EndCapStyle::BUTT,
                                                               bool solidColors = false) {
		std::vector<OsmAnd::VectorMapSymbol::Vertex> vertices;
		create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(vertex, vertices, points, thickness,
            outline, distances, fillColor, nearOutlineColor, farOutlineColor, colorizationMapping,
            outlineColorizationMapping, heights, startY31, flatten, jointStyle, startCapStyle, endCapStyle,
            solidColors);
		return vertices;
	}

	template<typename Vertex>
    static std::vector<OsmAnd::VectorMapSymbol::Vertex> create(OsmAnd::VectorMapSymbol::Vertex &vertex,
                                                               const std::vector<OsmAnd::PointD> &points,
                                                               float thickness,
                                                               float outline,
                                                               QList<float> &distances,
                                                               OsmAnd::FColorARGB &fillColor,
                                                               OsmAnd::FColorARGB &nearOutlineColor,
                                                               OsmAnd::FColorARGB &farOutlineColor,
                                                               QList<OsmAnd::FColorARGB> &colorizationMapping,
                                                               QList<OsmAnd::FColorARGB> &outlineColorizationMapping,
                                                               QList<float> &heights,
                                                               double startY31,
                                                               float flatten,
                                                               JointStyle jointStyle = JointStyle::MITER,
                                                               EndCapStyle startCapStyle = EndCapStyle::BUTT,
                                                               EndCapStyle endCapStyle = EndCapStyle::BUTT,
                                                               bool solidColors = false) {
        std::vector<OsmAnd::VectorMapSymbol::Vertex> vertices;
		create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(vertex, vertices, points, thickness,
            outline, distances, fillColor, nearOutlineColor, farOutlineColor, colorizationMapping,
            outlineColorizationMapping, heights, startY31, flatten, jointStyle, startCapStyle, endCapStyle,
            solidColors);
		return vertices;
	}

	template<typename Vertex, typename InputCollection>
    static size_t create(OsmAnd::VectorMapSymbol::Vertex &vertex,
                         std::vector<OsmAnd::VectorMapSymbol::Vertex> &vertices,
                         const InputCollection &points,
                         float thickness,
                         float outline,
                         QList<float> &distances,
                         OsmAnd::FColorARGB &fillColor,
                         OsmAnd::FColorARGB &nearOutlineColor,
                         OsmAnd::FColorARGB &farOutlineColor,
                         QList<OsmAnd::FColorARGB> &colorizationMapping,
                         QList<OsmAnd::FColorARGB> &outlineColorizationMapping,
                         QList<float> &heights,
                         double startY31,
                         float flatten,
                         JointStyle jointStyle = JointStyle::MITER,
                         EndCapStyle startCapStyle = EndCapStyle::BUTT,
                         EndCapStyle endCapStyle = EndCapStyle::BUTT,
                         bool solidColors = false) {
		auto numVerticesBefore = vertices.size();

        create<Vertex, InputCollection>(vertex, std::back_inserter(vertices), points, thickness, outline, distances,
            fillColor, nearOutlineColor, farOutlineColor, colorizationMapping, outlineColorizationMapping, heights,
            startY31, flatten, jointStyle, startCapStyle, endCapStyle, solidColors);

		return vertices.size() - numVerticesBefore;
	}

	template<typename Vertex, typename InputCollection, typename OutputIterator>
	static OutputIterator create(OsmAnd::VectorMapSymbol::Vertex &vertex,
                                 OutputIterator vertices,
                                 const InputCollection &points,
                                 float thickness,
                                 float outline,
                                 QList<float> &distances,
                                 OsmAnd::FColorARGB &fillColor,
                                 OsmAnd::FColorARGB &nearOutlineColor,
                                 OsmAnd::FColorARGB &farOutlineColor,
                                 QList<OsmAnd::FColorARGB> &colorizationMapping,
                                 QList<OsmAnd::FColorARGB> &outlineColorizationMapping,
                                 QList<float> &heights,
                                 double startY31,
                                 float flatten,
	                             JointStyle jointStyle = JointStyle::MITER,
	                             EndCapStyle startCapStyle = EndCapStyle::BUTT,
	                             EndCapStyle endCapStyle = EndCapStyle::BUTT,
                                 bool solidColors = false) {
        OsmAnd::VectorMapSymbol::Vertex* pVertex = &vertex;
        
		// operate on half the thickness to make our lives easier
		auto edgeOffset = thickness / 2;

		// create poly segments from the points
		std::vector<PolySegment<Vec2>> segments;
		for (size_t i = 0; i + 1 < points.size(); i++) {
			auto &pointD1 = points[i];
			auto &pointD2 = points[i + 1];
            
            Vec2 point1 = Vec2(pointD1.x, pointD1.y);
            Vec2 point2 = Vec2(pointD2.x, pointD2.y);

			// to avoid division-by-zero errors,
			// only create a line segment for non-identical points
			if (!Vec2Maths::equal(point1, point2)) {
                auto startEdgeOffset = correct(point1.y, startY31, flatten, edgeOffset);
                auto endEdgeOffset = correct(point2.y, startY31, flatten, edgeOffset);
				segments.emplace_back(LineSegment<Vec2>(point1, point2), startEdgeOffset, endEdgeOffset);
			}
		}

		if (segments.empty()) {
			// handle the case of insufficient input points
			return vertices;
		}

		if (startCapStyle == EndCapStyle::JOINT && endCapStyle == EndCapStyle::JOINT) {
			// create a connecting segment from the last to the first point

			auto &pointD1 = points[points.size() - 1];
			auto &pointD2 = points[0];
            
            Vec2 point1 = Vec2(pointD1.x, pointD1.y);
            Vec2 point2 = Vec2(pointD2.x, pointD2.y);

			// to avoid division-by-zero errors,
			// only create a line segment for non-identical points
			if (!Vec2Maths::equal(point1, point2)) {
                auto startEdgeOffset = correct(point1.y, startY31, flatten, edgeOffset);
                auto endEdgeOffset = correct(point2.y, startY31, flatten, edgeOffset);
				segments.emplace_back(LineSegment<Vec2>(point1, point2), startEdgeOffset, endEdgeOffset);
			}
		}

		Vec2 nextStart1{0, 0};
		Vec2 nextStart2{0, 0};
		Vec2 start1{0, 0};
		Vec2 start2{0, 0};
		Vec2 end1{0, 0};
		Vec2 end2{0, 0};

		// calculate the path's global start and end points
		auto &firstSegment = segments[0];
		auto &lastSegment = segments[segments.size() - 1];

		auto pathStart1 = firstSegment.edge1.a;
		auto pathStart2 = firstSegment.edge2.a;
		auto pathEnd1 = lastSegment.edge1.b;
		auto pathEnd2 = lastSegment.edge2.b;

        auto pathStartThickness = correct(firstSegment.center.a.y, startY31, flatten, thickness);
        auto pathEndThickness = correct(lastSegment.center.b.y, startY31, flatten, thickness);

        auto pathStartEdgeOffset = pathStartThickness / 2;
        auto pathEndEdgeOffset = pathEndThickness / 2;

        bool hasDistances = distances.size() == points.size();
        auto noDistance = NAN;
        bool hasColorMapping = !colorizationMapping.isEmpty();
        bool hasOutlineColorMapping = !outlineColorizationMapping.isEmpty();
        bool hasHeights = heights.size() == points.size();
        auto noHeight = OsmAnd::VectorMapSymbol::_absentElevation;
        auto outlineWidth = !hasHeights && outline > 0.0f ? outline : -1.0f;
        bool showOutline = outlineWidth >= 0.0f;
		Vec2 startSide1, startSide2, endSide1, endSide2, nextStartSide1, nextStartSide2, lastSide1, lastSide2;

        // Prepare ending arrow parameters
        auto lastMidPoint = Vec2((pathEnd1.x + pathEnd2.x) / 2, (pathEnd1.y + pathEnd2.y) / 2);
        auto endArrowLength = pathEndThickness * 2;
        auto endArrowShift = endArrowLength;
        auto lastLength = Vec2Maths::magnitude(Vec2Maths::subtract(lastSegment.center.b, lastSegment.center.a));
        auto lastSegmentD = lastSegment.center.direction();
        auto lastSegmentN = lastSegment.center.normal();

		// handle different end cap styles
        if (endCapStyle == EndCapStyle::ARROW) {
            // Define location for an arrow cap
            if (!hasHeights && showOutline) {
                auto arrowOutline = correct(lastMidPoint.y, startY31, flatten, outlineWidth);
                endArrowShift += arrowOutline;
            }

            // Reserve minimum length for the last segment to avoid possible artifacts
            auto length = lastLength;
            length -= segmentMinLength;

            if (endArrowShift > length) {
                lastMidPoint = Vec2Maths::add(lastMidPoint, Vec2Maths::multiply(lastSegmentD, endArrowShift - length));
                endArrowShift = length;
            }

            pathEnd1 =
                Vec2Maths::subtract(pathEnd1, Vec2Maths::multiply(lastSegment.edge1.direction(), endArrowShift));
            pathEnd2 =
                Vec2Maths::subtract(pathEnd2, Vec2Maths::multiply(lastSegment.edge2.direction(), endArrowShift));

            lastSegment.edge1.b = pathEnd1;
            lastSegment.edge2.b = pathEnd2;
            lastSegment.center.b =
                Vec2Maths::subtract(lastSegment.center.b, Vec2Maths::multiply(lastSegmentD, endArrowShift));
        }

        if (startCapStyle == EndCapStyle::ARROW) {
            // Prepare starting arrow parameters
            auto firstSegmentD = firstSegment.center.direction();
            auto firstSegmentN = firstSegment.center.normal();
            auto firstLength =
                Vec2Maths::magnitude(Vec2Maths::subtract(firstSegment.center.b, firstSegment.center.a));
            auto firstMidPoint = Vec2((pathStart1.x + pathStart2.x) / 2, (pathStart1.y + pathStart2.y) / 2);
            auto startArrowLength = pathStartThickness * 2;
            auto startArrowShift = startArrowLength;

            // Define location for an arrow cap
            if (!hasHeights && showOutline) {
                auto arrowOutline = correct(firstMidPoint.y, startY31, flatten, outlineWidth);
                startArrowShift += arrowOutline;
            }

            // Reserve minimum length for the first segment to avoid possible artifacts
            auto length = firstLength;
            length -= segmentMinLength;

            if (startArrowShift > length) {
                firstMidPoint =
                    Vec2Maths::subtract(firstMidPoint, Vec2Maths::multiply(firstSegmentD, startArrowShift - length));
                startArrowShift = length;
            }

            pathStart1 =
                Vec2Maths::add(pathStart1, Vec2Maths::multiply(firstSegment.edge1.direction(), startArrowShift));
            pathStart2 =
                Vec2Maths::add(pathStart2, Vec2Maths::multiply(firstSegment.edge2.direction(), startArrowShift));

            firstSegment.edge1.a = pathStart1;
            firstSegment.edge2.a = pathStart2;
            firstSegment.center.a =
                Vec2Maths::add(firstSegment.center.a, Vec2Maths::multiply(firstSegmentD, startArrowShift));

            // Generate an arrow cap
            auto c_pt = Vec2Maths::add(firstMidPoint, Vec2Maths::multiply(firstSegmentD, startArrowLength));
            auto pt1 = Vec2Maths::add(c_pt, Vec2Maths::multiply(firstSegmentN, pathStartThickness));
            auto pt2 = Vec2Maths::subtract(c_pt, Vec2Maths::multiply(firstSegmentN, pathStartThickness));
            auto arrowCenter = Vec2((pt1.x + pt2.x) / 2, (pt1.y + pt2.y) / 2);
            
            auto arrowStart1 = pathStart1;
            auto arrowStart2 = pathStart2;
            if (showOutline) {
                auto arrowOutline1 = correct(arrowStart1.y, startY31, flatten, outlineWidth);
                auto arrowOutline2 = correct(arrowStart2.y, startY31, flatten, outlineWidth);
                arrowStart1 = Vec2Maths::subtract(
                    arrowStart1, Vec2Maths::multiply(firstSegment.edge1.direction(), arrowOutline1));
                arrowStart2 = Vec2Maths::subtract(
                    arrowStart2, Vec2Maths::multiply(firstSegment.edge2.direction(), arrowOutline2));
            }

            auto distance = hasDistances ? distances.first() : noDistance;
            auto tipDistance = distance;
            auto top = hasHeights ? heights.first() : noHeight;
            auto tip = top;
            if (hasDistances) {
                auto difDistance = distance - distances[1];
                distance -= startArrowShift * difDistance / firstLength;
                tipDistance += (startArrowLength - startArrowShift) * difDistance / firstLength;
            }
            if (hasHeights) {
                auto difHeight = top - heights[1];
                top -= startArrowShift * difHeight / firstLength;
                tip += (startArrowLength - startArrowShift) * difHeight / firstLength;
            }

            pVertex->color = hasColorMapping ? colorizationMapping.first() : fillColor;

            pVertex->positionXYZD[0] = firstMidPoint.x;
            pVertex->positionXYZD[1] = tip;
            pVertex->positionXYZD[2] = firstMidPoint.y;
            pVertex->positionXYZD[3] = tipDistance;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = pt2.x;
            pVertex->positionXYZD[1] = top;
            pVertex->positionXYZD[2] = pt2.y;
            pVertex->positionXYZD[3] = distance;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = arrowStart2.x;
            pVertex->positionXYZD[2] = arrowStart2.y;
            *vertices++ = vertex;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = arrowCenter.x;
            pVertex->positionXYZD[2] = arrowCenter.y;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = firstMidPoint.x;
            pVertex->positionXYZD[1] = tip;
            pVertex->positionXYZD[2] = firstMidPoint.y;
            pVertex->positionXYZD[3] = tipDistance;
            *vertices++ = vertex;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = arrowCenter.x;
            pVertex->positionXYZD[1] = top;
            pVertex->positionXYZD[2] = arrowCenter.y;
            pVertex->positionXYZD[3] = distance;
            *vertices++ = vertex;

            pVertex->positionXYZD[0] = arrowStart1.x;
            pVertex->positionXYZD[2] = arrowStart1.y;
            *vertices++ = vertex;
            *vertices++ = vertex;
           
            pVertex->positionXYZD[0] = pt1.x;
            pVertex->positionXYZD[2] = pt1.y;
            *vertices++ = vertex;
           
            pVertex->positionXYZD[0] = firstMidPoint.x;
            pVertex->positionXYZD[1] = tip;
            pVertex->positionXYZD[2] = firstMidPoint.y;
            pVertex->positionXYZD[3] = tipDistance;
            *vertices++ = vertex;

            if (!hasHeights && showOutline) {
                pVertex->positionXYZD[0] = arrowStart2.x;
                pVertex->positionXYZD[2] = arrowStart2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathStart2.x;
                pVertex->positionXYZD[2] = pathStart2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = firstSegment.center.a.x;
                pVertex->positionXYZD[2] = firstSegment.center.a.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathStart1.x;
                pVertex->positionXYZD[2] = pathStart1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart1.x;
                pVertex->positionXYZD[2] = arrowStart1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowCenter.x;
                pVertex->positionXYZD[2] = arrowCenter.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = firstSegment.center.a.x;
                pVertex->positionXYZD[2] = firstSegment.center.a.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowCenter.x;
                pVertex->positionXYZD[2] = arrowCenter.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart2.x;
                pVertex->positionXYZD[2] = arrowStart2.y;
                *vertices++ = vertex;
            }

            if (showOutline) {
                auto outline1 = correct(pathStart1.y, startY31, flatten, outlineWidth);
                auto outline2 = correct(pathStart2.y, startY31, flatten, outlineWidth);
                auto side1 = Vec2Maths::add(pathStart1, Vec2Maths::multiply(firstSegment.edge1.normal(), outline1));
                auto side2 =
                    Vec2Maths::subtract(pathStart2, Vec2Maths::multiply(firstSegment.edge2.normal(), outline2));
                auto edgeSegment = LineSegment<Vec2>(side1, side2);
                auto sideSegment1 = LineSegment<Vec2>(pt1, firstMidPoint);
                auto sideSegmentN = sideSegment1.normal();
                outline1 = correct(sideSegment1.a.y, startY31, flatten, outlineWidth);
                outline2 = correct(sideSegment1.b.y, startY31, flatten, outlineWidth);
                sideSegment1.a = Vec2Maths::subtract(sideSegment1.a, Vec2Maths::multiply(sideSegmentN, outline1));
                sideSegment1.b = Vec2Maths::subtract(sideSegment1.b, Vec2Maths::multiply(sideSegmentN, outline2));
                auto secSide1 = LineSegment<Vec2>::intersection(edgeSegment, sideSegment1, true);
                auto arrowSide1 = secSide1.second ? secSide1.first : pt1;
                auto sideSegment2 = LineSegment<Vec2>(pt2, firstMidPoint);
                sideSegmentN = sideSegment2.normal();
                outline1 = correct(sideSegment2.a.y, startY31, flatten, outlineWidth);
                outline2 = correct(sideSegment2.b.y, startY31, flatten, outlineWidth);
                sideSegment2.a = Vec2Maths::add(sideSegment2.a, Vec2Maths::multiply(sideSegmentN, outline1));
                sideSegment2.b = Vec2Maths::add(sideSegment2.b, Vec2Maths::multiply(sideSegmentN, outline2));
                auto secSide2 = LineSegment<Vec2>::intersection(edgeSegment, sideSegment2, true);
                auto arrowSide2 = secSide2.second ? secSide2.first : pt2;
                auto secSide3 = LineSegment<Vec2>::intersection(sideSegment1, sideSegment2, true);
                auto arrowSide3 = secSide3.second ? secSide3.first : firstMidPoint;

                auto bottom = hasHeights ? top - outline : noHeight;
                auto tipBottom = hasHeights ? tip - outline : noHeight;

                auto innerColor = nearOutlineColor;
                auto outerColor = farOutlineColor;
                if (showOutline && hasOutlineColorMapping) {
                    innerColor *= outlineColorizationMapping.first();
                    outerColor *= outlineColorizationMapping.first();
                }

                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side2.y;
                pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pathStart2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pathStart2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart2.x;
                pVertex->positionXYZD[2] = arrowStart2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[2] = pt2.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt2.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = arrowSide2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt2.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide3.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = arrowSide3.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = firstMidPoint.x;
                pVertex->positionXYZD[1] = tip;
                pVertex->positionXYZD[2] = firstMidPoint.y;
                pVertex->positionXYZD[3] = tipDistance;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt1.y;
                pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide3.x;
                pVertex->positionXYZD[1] = tipBottom;
                pVertex->positionXYZD[2] = arrowSide3.y;
                pVertex->positionXYZD[3] = tipDistance;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt1.y;
                pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide1.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = arrowSide1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt1.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side1.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart1.x;
                pVertex->positionXYZD[2] = arrowStart1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathStart1.x;
                pVertex->positionXYZD[2] = pathStart1.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side1.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side1.y;
                *vertices++ = vertex;
            }
        } else if (startCapStyle == EndCapStyle::ROUND) {
			// draw half circle first end cap
			auto firstColor = hasColorMapping ? colorizationMapping.first() : fillColor;
			auto innerColor = nearOutlineColor;
			auto outerColor = farOutlineColor;
            if (showOutline && hasOutlineColorMapping) {
			    innerColor *= outlineColorizationMapping.first();
			    outerColor *= outlineColorizationMapping.first();
            }
			auto firstDistance = hasDistances ? distances.first() : noDistance;
			auto firstHeight = hasHeights ? heights.first() : noHeight;
            createTriangleFan(vertex, vertices, firstColor, firstColor, innerColor, innerColor, outerColor,
                outerColor, outlineWidth, firstDistance, firstHeight, outline, firstSegment.center.a,
                firstSegment.center.a, firstSegment.edge1.a, firstSegment.edge2.a, endSide1, nextStartSide1, false,
                startY31, flatten);
        } else if (startCapStyle == EndCapStyle::JOINT && endCapStyle == EndCapStyle::JOINT) {
			// join the last (connecting) segment and the first segment
			auto endColor = hasColorMapping ? colorizationMapping.last() : fillColor;
			auto nextColor = hasColorMapping ? colorizationMapping.first() : fillColor;
			auto distance = hasDistances ? distances.first() : noDistance;
			auto height = hasHeights ? heights.first() : noHeight;
            auto side1 = lastSegment.edge1.a;
            auto side2 = lastSegment.edge2.a;
			auto endInnerColor = nearOutlineColor;
			auto nextInnerColor = nearOutlineColor;
			auto endOuterColor = farOutlineColor;
			auto nextOuterColor = farOutlineColor;
            if (showOutline) {
                auto outSide1 = correct(side1.y, startY31, flatten, outlineWidth);
                auto outSide2 = correct(side2.y, startY31, flatten, outlineWidth);
                side1 = Vec2Maths::add(side1, Vec2Maths::multiply(lastSegment.edge1.normal(), outSide1));
                side2 = Vec2Maths::subtract(side2, Vec2Maths::multiply(lastSegment.edge2.normal(), outSide2));
                if (hasOutlineColorMapping) {
                    endInnerColor *= outlineColorizationMapping.last();
                    nextInnerColor *= outlineColorizationMapping.first();
                    endOuterColor *= outlineColorizationMapping.last();
                    nextOuterColor *= outlineColorizationMapping.first();
                }
            }
            createJoint(vertex, vertices, lastSegment, firstSegment, lastSegment.edge1.a, lastSegment.edge2.a, side1,
                side2, endColor, nextColor, endInnerColor, nextInnerColor, endOuterColor, nextOuterColor, outlineWidth,
                distance, height, outline, jointStyle, pathEnd1, pathEnd2, pathStart1, pathStart2,
                lastSide1, lastSide2, startSide1, startSide2, solidColors, startY31, flatten);
            firstSegment.edge1.a = pathStart1;
            firstSegment.edge2.a = pathStart2;
            lastSegment.edge1.b = pathEnd1;
            lastSegment.edge2.b = pathEnd2;
        } else {
            auto pStart1 = pathStart1;
            auto pStart2 = pathStart2;
            auto pCenter = firstSegment.center.a;
            if (startCapStyle == EndCapStyle::SQUARE) {
                // extend the start points by half the thickness
                pStart1 = Vec2Maths::subtract(
                    pathStart1, Vec2Maths::multiply(firstSegment.edge1.direction(), pathStartEdgeOffset));
                pStart2 = Vec2Maths::subtract(
                    pathStart2, Vec2Maths::multiply(firstSegment.edge2.direction(), pathStartEdgeOffset));
                pCenter = Vec2Maths::subtract(
                    pCenter, Vec2Maths::multiply(firstSegment.center.direction(), pathStartEdgeOffset));

                pVertex->positionXYZD[3] = hasDistances ? distances.first() : noDistance;

			    auto top = hasHeights ? heights.first() : noHeight;

                pVertex->color = hasColorMapping ? colorizationMapping.first() : fillColor;

                pVertex->positionXYZD[0] = pathStart1.x;
			    pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pathStart1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pStart1.x;
                pVertex->positionXYZD[2] = pStart1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = firstSegment.center.a.x;
                pVertex->positionXYZD[2] = firstSegment.center.a.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pStart1.x;
                pVertex->positionXYZD[2] = pStart1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pCenter.x;
                pVertex->positionXYZD[2] = pCenter.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pStart2.x;
                pVertex->positionXYZD[2] = pStart2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = firstSegment.center.a.x;
                pVertex->positionXYZD[2] = firstSegment.center.a.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pStart2.x;
                pVertex->positionXYZD[2] = pStart2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathStart2.x;
                pVertex->positionXYZD[2] = pathStart2.y;
                *vertices++ = vertex;
            }            
    		if (showOutline) {
                // pull out start points for outline part
                auto outPathStartSideWidth1 = correct(pStart1.y, startY31, flatten, outlineWidth);
                auto outPathStartSideWidth2 = correct(pStart2.y, startY31, flatten, outlineWidth);
                auto outPathStartSide1 = Vec2Maths::add(
                    pStart1, Vec2Maths::multiply(firstSegment.edge1.normal(), outPathStartSideWidth1));
                auto outPathStartSide2 = Vec2Maths::subtract(
                    pStart2, Vec2Maths::multiply(firstSegment.edge2.normal(), outPathStartSideWidth2));
                auto outPathStartWidth1 = correct(outPathStartSide1.y, startY31, flatten, outlineWidth);
                auto outPathStartWidth2 = correct(outPathStartSide2.y, startY31, flatten, outlineWidth);
                auto outPathStart1 = Vec2Maths::subtract(
                    outPathStartSide1, Vec2Maths::multiply(firstSegment.edge1.direction(), outPathStartWidth1));
                auto outPathStart2 = Vec2Maths::subtract(
                    outPathStartSide2, Vec2Maths::multiply(firstSegment.edge2.direction(), outPathStartWidth2));

                auto innerColor = nearOutlineColor;
                auto outerColor = farOutlineColor;
                if (showOutline && hasOutlineColorMapping) {
                    innerColor *= outlineColorizationMapping.first();
                    outerColor *= outlineColorizationMapping.first();
                }

                pVertex->positionXYZD[3] = hasDistances ? distances.first() : noDistance;

			    auto top = hasHeights ? heights.first() : noHeight;
			    auto bottom = hasHeights ? heights.first() - outline : noHeight;

                // emit triangles for starting outline part
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathStartSide1.x;
			    pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathStartSide1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = outPathStart1.x;
                pVertex->positionXYZD[2] = outPathStart1.y;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pStart1.x;
			    pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pStart1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathStart1.x;
			    pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathStart1.y;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pCenter.x;
			    pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pCenter.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathStart1.x;
			    pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathStart1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = outPathStart2.x;
                pVertex->positionXYZD[2] = outPathStart2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = outPathStartSide2.x;
                pVertex->positionXYZD[2] = outPathStartSide2.y;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pStart2.x;
			    pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pStart2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pCenter.x;
                pVertex->positionXYZD[2] = pCenter.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathStart2.x;
			    pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathStart2.y;
                *vertices++ = vertex;

                if (startCapStyle == EndCapStyle::SQUARE) {
                    auto outPrevStartSideWidth1 = correct(pathStart1.y, startY31, flatten, outlineWidth);
                    auto outPrevStartSideWidth2 = correct(pathStart2.y, startY31, flatten, outlineWidth);
                    auto outPrevStartSide1 = Vec2Maths::add(
                        pathStart1, Vec2Maths::multiply(firstSegment.edge1.normal(), outPrevStartSideWidth1));
                    auto outPrevStartSide2 = Vec2Maths::subtract(
                        pathStart2, Vec2Maths::multiply(firstSegment.edge2.normal(), outPrevStartSideWidth2));

                    pVertex->color = outerColor;
                    pVertex->positionXYZD[0] = outPrevStartSide1.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = outPrevStartSide1.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = outPathStartSide1.x;
                    pVertex->positionXYZD[2] = outPathStartSide1.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = innerColor;
                    pVertex->positionXYZD[0] = pStart1.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = pStart1.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = pathStart1.x;
                    pVertex->positionXYZD[2] = pathStart1.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = outerColor;
                    pVertex->positionXYZD[0] = outPrevStartSide1.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = outPrevStartSide1.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = outPathStartSide2.x;
                    pVertex->positionXYZD[2] = outPathStartSide2.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = outPrevStartSide2.x;
                    pVertex->positionXYZD[2] = outPrevStartSide2.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = innerColor;
                    pVertex->positionXYZD[0] = pathStart2.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = pathStart2.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = pStart2.x;
                    pVertex->positionXYZD[2] = pStart2.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = outerColor;
                    pVertex->positionXYZD[0] = outPathStartSide2.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = outPathStartSide2.y;
                    *vertices++ = vertex;
                }
            }
        }

		// generate mesh data for path segments
		for (int i = 0; i < segments.size(); i++) {
			auto &segment = segments[i];
            int colorIndexShift = solidColors ? 0 : 1;
            auto &startColor = i < colorizationMapping.size() ? colorizationMapping[i] : fillColor;
            auto &endColor = i < colorizationMapping.size() - colorIndexShift
                ? colorizationMapping[i + colorIndexShift] : fillColor;
            auto &nextColor = i < colorizationMapping.size() - colorIndexShift - 1
                ? colorizationMapping[i + colorIndexShift + 1] : fillColor;
            auto startInnerColor = nearOutlineColor;
            auto endInnerColor = nearOutlineColor;
            auto nextInnerColor = nearOutlineColor;
            auto startOuterColor = farOutlineColor;
            auto endOuterColor = farOutlineColor;
            auto nextOuterColor = farOutlineColor;
            if (showOutline) {
                if (i < outlineColorizationMapping.size()) {
                    startInnerColor *= outlineColorizationMapping[i];
                    startOuterColor *= outlineColorizationMapping[i];
                }
                if (i < outlineColorizationMapping.size() - colorIndexShift) {
                    endInnerColor *= outlineColorizationMapping[i + colorIndexShift];
                    endOuterColor *= outlineColorizationMapping[i + colorIndexShift];
                }
                if (i < outlineColorizationMapping.size() - colorIndexShift - 1) {
                    nextInnerColor *= outlineColorizationMapping[i + colorIndexShift + 1];
                    nextOuterColor *= outlineColorizationMapping[i + colorIndexShift + 1];
                }
            }

            auto startDistance = hasDistances && i < distances.size() ? distances[i] : noDistance;
            auto endDistance = hasDistances && i < distances.size() - 1 ? distances[i + 1] : noDistance;

            auto startHeight = hasHeights && i < heights.size() ? heights[i] : noHeight;
            auto endHeight = hasHeights && i < heights.size() - 1 ? heights[i + 1] : noHeight;

			// calculate start
			if (i == 0) {
				// this is the first segment
				start1 = pathStart1;
				start2 = pathStart2;

        		if (showOutline && (startCapStyle != EndCapStyle::JOINT || endCapStyle != EndCapStyle::JOINT)) {
                    auto shiftStart = Vec2Maths::normalized(Vec2Maths::subtract(start2, start1));
                    auto startOutline1 = correct(start1.y, startY31, flatten, outlineWidth);
                    auto startOutline2 = correct(start2.y, startY31, flatten, outlineWidth);
                    startSide1 = Vec2Maths::subtract(start1, Vec2Maths::multiply(shiftStart, startOutline1));
                    startSide2 = Vec2Maths::add(start2, Vec2Maths::multiply(shiftStart, startOutline2));
                }
			}

			if (i + 1 == segments.size()) {
				// this is the last segment
				end1 = pathEnd1;
				end2 = pathEnd2;

                if (endCapStyle == EndCapStyle::ARROW) {
                    if (hasDistances) {
                        endDistance -= endArrowShift * (endDistance - startDistance) / lastLength;
                    }
                    if (hasHeights) {
                        endHeight -= endArrowShift * (endHeight - startHeight) / lastLength;
                    }
                }

        		if (showOutline) {
                    if (startCapStyle == EndCapStyle::JOINT && endCapStyle == EndCapStyle::JOINT) {
                        endSide1 = lastSide1;
                        endSide2 = lastSide2;
                    } else {
                        auto shiftEnd = Vec2Maths::normalized(Vec2Maths::subtract(end2, end1));
                        auto endOutline1 = correct(end1.y, startY31, flatten, outlineWidth);
                        auto endOutline2 = correct(end2.y, startY31, flatten, outlineWidth);
                        endSide1 = Vec2Maths::subtract(end1, Vec2Maths::multiply(shiftEnd, endOutline1));
                        endSide2 = Vec2Maths::add(end2, Vec2Maths::multiply(shiftEnd, endOutline2));
                    }
                }
			} else {
                createJoint(vertex, vertices, segment, segments[i + 1], start1, start2, startSide1, startSide2,
                    endColor, nextColor, endInnerColor, nextInnerColor, endOuterColor, nextOuterColor, outlineWidth,
                    endDistance, endHeight, outline, jointStyle, end1, end2, nextStart1, nextStart2,
                    endSide1, endSide2, nextStartSide1, nextStartSide2, solidColors, startY31, flatten);
			}

            // emit vertices
            auto startTop = startHeight;
            auto endTop = endHeight;
            if (!Vec2Maths::equal(start2, end2)) {
                pVertex->color = startColor;
                pVertex->positionXYZD[1] = startTop;
                pVertex->positionXYZD[0] = segment.center.a.x;
                pVertex->positionXYZD[2] = segment.center.a.y;
                pVertex->positionXYZD[3] = startDistance;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = start2.x;
                pVertex->positionXYZD[2] = start2.y;
                *vertices++ = vertex;
                
                pVertex->color = endColor;
                pVertex->positionXYZD[1] = endTop;
                pVertex->positionXYZD[0] = end2.x;
                pVertex->positionXYZD[2] = end2.y;
                pVertex->positionXYZD[3] = endDistance;
                *vertices++ = vertex;
            }

            if (!Vec2Maths::equal(start1, end1)) {
                pVertex->color = endColor;
                pVertex->positionXYZD[1] = endTop;
                pVertex->positionXYZD[0] = segment.center.b.x;
                pVertex->positionXYZD[2] = segment.center.b.y;
                pVertex->positionXYZD[3] = endDistance;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = end1.x;
                pVertex->positionXYZD[2] = end1.y;
                *vertices++ = vertex;
                
                pVertex->color = startColor;
                pVertex->positionXYZD[1] = startTop;
                pVertex->positionXYZD[0] = start1.x;
                pVertex->positionXYZD[2] = start1.y;
                pVertex->positionXYZD[3] = startDistance;
                *vertices++ = vertex;
            }

            if (!Vec2Maths::equal(start1, end1)) {
                pVertex->color = startColor;
                pVertex->positionXYZD[1] = startTop;
                pVertex->positionXYZD[0] = start1.x;
                pVertex->positionXYZD[2] = start1.y;
                pVertex->positionXYZD[3] = startDistance;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = segment.center.a.x;
                pVertex->positionXYZD[2] = segment.center.a.y;
                *vertices++ = vertex;
                
                pVertex->color = endColor;
                pVertex->positionXYZD[1] = endTop;
                pVertex->positionXYZD[0] = segment.center.b.x;
                pVertex->positionXYZD[2] = segment.center.b.y;
                pVertex->positionXYZD[3] = endDistance;
                *vertices++ = vertex;
            }

            if (!Vec2Maths::equal(start2, end2)) {
                pVertex->color = endColor;
                pVertex->positionXYZD[1] = endTop;
                pVertex->positionXYZD[0] = end2.x;
                pVertex->positionXYZD[2] = end2.y;
                pVertex->positionXYZD[3] = endDistance;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = segment.center.b.x;
                pVertex->positionXYZD[2] = segment.center.b.y;
                *vertices++ = vertex;
                
                pVertex->color = startColor;
                pVertex->positionXYZD[1] = startTop;
                pVertex->positionXYZD[0] = segment.center.a.x;
                pVertex->positionXYZD[2] = segment.center.a.y;
                pVertex->positionXYZD[3] = startDistance;
                *vertices++ = vertex;
            }

            if (showOutline) {
                auto startBottom =
                    startHeight == OsmAnd::VectorMapSymbol::_absentElevation ? startHeight : startHeight - outline;
                auto endBottom =
                    endHeight == OsmAnd::VectorMapSymbol::_absentElevation ? endHeight : endHeight - outline;
                if (!Vec2Maths::equal(start1, end1)) {
                    if (!Vec2Maths::equal(startSide1, endSide1)) {
                        pVertex->color = startOuterColor;
                        pVertex->positionXYZD[0] = startSide1.x;
                        pVertex->positionXYZD[1] = startBottom;
                        pVertex->positionXYZD[2] = startSide1.y;
                        pVertex->positionXYZD[3] = startDistance;
                        *vertices++ = vertex;
                        
                        pVertex->color = startInnerColor;
                        pVertex->positionXYZD[0] = start1.x;
                        pVertex->positionXYZD[1] = startTop;
                        pVertex->positionXYZD[2] = start1.y;
                        *vertices++ = vertex;
                        
                        pVertex->color = endOuterColor;
                        pVertex->positionXYZD[0] = endSide1.x;
                        pVertex->positionXYZD[1] = endBottom;
                        pVertex->positionXYZD[2] = endSide1.y;
                        pVertex->positionXYZD[3] = endDistance;
                        *vertices++ = vertex;
                    }
                    pVertex->color = endInnerColor;
                    pVertex->positionXYZD[0] = end1.x;
                    pVertex->positionXYZD[1] = endTop;
                    pVertex->positionXYZD[2] = end1.y;
                    pVertex->positionXYZD[3] = endDistance;
                    *vertices++ = vertex;
                    
                    pVertex->color = endOuterColor;
                    pVertex->positionXYZD[0] = endSide1.x;
                    pVertex->positionXYZD[1] = endBottom;
                    pVertex->positionXYZD[2] = endSide1.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = startInnerColor;
                    pVertex->positionXYZD[0] = start1.x;
                    pVertex->positionXYZD[1] = startTop;
                    pVertex->positionXYZD[2] = start1.y;
                    pVertex->positionXYZD[3] = startDistance;
                    *vertices++ = vertex;
                }

                if (!Vec2Maths::equal(start2, end2)) {
                    if (!Vec2Maths::equal(startSide2, endSide2)) {
                        pVertex->color = endOuterColor;
                        pVertex->positionXYZD[0] = endSide2.x;
                        pVertex->positionXYZD[1] = endBottom;
                        pVertex->positionXYZD[2] = endSide2.y;
                        pVertex->positionXYZD[3] = endDistance;
                        *vertices++ = vertex;
                        
                        pVertex->color = endInnerColor;
                        pVertex->positionXYZD[0] = end2.x;
                        pVertex->positionXYZD[1] = endTop;
                        pVertex->positionXYZD[2] = end2.y;
                        *vertices++ = vertex;
                        
                        pVertex->color = startOuterColor;
                        pVertex->positionXYZD[0] = startSide2.x;
                        pVertex->positionXYZD[1] = startBottom;
                        pVertex->positionXYZD[2] = startSide2.y;
                        pVertex->positionXYZD[3] = startDistance;
                        *vertices++ = vertex;
                    }
                    pVertex->color = startInnerColor;
                    pVertex->positionXYZD[0] = start2.x;
                    pVertex->positionXYZD[1] = startTop;
                    pVertex->positionXYZD[2] = start2.y;
                    pVertex->positionXYZD[3] = startDistance;
                    *vertices++ = vertex;
                    
                    pVertex->color = startOuterColor;
                    pVertex->positionXYZD[0] = startSide2.x;
                    pVertex->positionXYZD[1] = startBottom;
                    pVertex->positionXYZD[2] = startSide2.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = endInnerColor;
                    pVertex->positionXYZD[0] = end2.x;
                    pVertex->positionXYZD[1] = endTop;
                    pVertex->positionXYZD[2] = end2.y;
                    pVertex->positionXYZD[3] = endDistance;
                    *vertices++ = vertex;
                }
    			startSide1 = nextStartSide1;
	    		startSide2 = nextStartSide2;
            }

			start1 = nextStart1;
			start2 = nextStart2;
		}

        if (endCapStyle == EndCapStyle::ARROW) {
            // Generate an arrow cap
            auto c_pt = Vec2Maths::subtract(lastMidPoint, Vec2Maths::multiply(lastSegmentD, endArrowLength));
            auto pt1 = Vec2Maths::add(c_pt, Vec2Maths::multiply(lastSegmentN, pathEndThickness));
            auto pt2 = Vec2Maths::subtract(c_pt, Vec2Maths::multiply(lastSegmentN, pathEndThickness));
            auto arrowCenter = Vec2((pt1.x + pt2.x) / 2, (pt1.y + pt2.y) / 2);
            
            auto arrowStart1 = pathEnd1;
            auto arrowStart2 = pathEnd2;
            if (showOutline) {
                auto arrowOutline1 = correct(arrowStart1.y, startY31, flatten, outlineWidth);
                auto arrowOutline2 = correct(arrowStart2.y, startY31, flatten, outlineWidth);
                arrowStart1 =
                    Vec2Maths::add(arrowStart1, Vec2Maths::multiply(lastSegment.edge1.direction(), arrowOutline1));
                arrowStart2 =
                    Vec2Maths::add(arrowStart2, Vec2Maths::multiply(lastSegment.edge2.direction(), arrowOutline2));
            }

            auto distance = hasDistances ? distances.last() : noDistance;
            auto tipDistance = distance;
            auto top = hasHeights ? heights.last() : noHeight;
            auto tip = top;
            if (hasDistances) {
                auto difDistance = distance - distances[distances.size() - 2];
                distance -= endArrowShift * difDistance / lastLength;
                tipDistance += (endArrowLength - endArrowShift) * difDistance / lastLength;
            }
            if (hasHeights) {
                auto difHeight = top - heights[heights.size() - 2];
                top -= endArrowShift * difHeight / lastLength;
                tip += (endArrowLength - endArrowShift) * difHeight / lastLength;
            }

            pVertex->color = hasColorMapping ? colorizationMapping.last() : fillColor;

            pVertex->positionXYZD[0] = lastMidPoint.x;
            pVertex->positionXYZD[1] = tip;
            pVertex->positionXYZD[2] = lastMidPoint.y;
            pVertex->positionXYZD[3] = tipDistance;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = pt1.x;
            pVertex->positionXYZD[1] = top;
            pVertex->positionXYZD[2] = pt1.y;
            pVertex->positionXYZD[3] = distance;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = arrowStart1.x;
            pVertex->positionXYZD[2] = arrowStart1.y;
            *vertices++ = vertex;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = arrowCenter.x;
            pVertex->positionXYZD[2] = arrowCenter.y;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = lastMidPoint.x;
            pVertex->positionXYZD[1] = tip;
            pVertex->positionXYZD[2] = lastMidPoint.y;
            pVertex->positionXYZD[3] = tipDistance;
            *vertices++ = vertex;
            *vertices++ = vertex;
            
            pVertex->positionXYZD[0] = arrowCenter.x;
            pVertex->positionXYZD[1] = top;
            pVertex->positionXYZD[2] = arrowCenter.y;
            pVertex->positionXYZD[3] = distance;
            *vertices++ = vertex;

            pVertex->positionXYZD[0] = arrowStart2.x;
            pVertex->positionXYZD[2] = arrowStart2.y;
            *vertices++ = vertex;
            *vertices++ = vertex;
           
            pVertex->positionXYZD[0] = pt2.x;
            pVertex->positionXYZD[2] = pt2.y;
            *vertices++ = vertex;
           
            pVertex->positionXYZD[0] = lastMidPoint.x;
            pVertex->positionXYZD[1] = tip;
            pVertex->positionXYZD[2] = lastMidPoint.y;
            pVertex->positionXYZD[3] = tipDistance;
            *vertices++ = vertex;

            if (!hasHeights && showOutline) {
                pVertex->positionXYZD[0] = arrowStart1.x;
                pVertex->positionXYZD[2] = arrowStart1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathEnd1.x;
                pVertex->positionXYZD[2] = pathEnd1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = lastSegment.center.b.x;
                pVertex->positionXYZD[2] = lastSegment.center.b.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathEnd2.x;
                pVertex->positionXYZD[2] = pathEnd2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart2.x;
                pVertex->positionXYZD[2] = arrowStart2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowCenter.x;
                pVertex->positionXYZD[2] = arrowCenter.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = lastSegment.center.b.x;
                pVertex->positionXYZD[2] = lastSegment.center.b.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowCenter.x;
                pVertex->positionXYZD[2] = arrowCenter.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart1.x;
                pVertex->positionXYZD[2] = arrowStart1.y;
                *vertices++ = vertex;
            }

            if (showOutline) {
                auto outline1 = correct(pathEnd1.y, startY31, flatten, outlineWidth);
                auto outline2 = correct(pathEnd2.y, startY31, flatten, outlineWidth);
                auto side1 = Vec2Maths::add(pathEnd1, Vec2Maths::multiply(lastSegment.edge1.normal(), outline1));
                auto side2 = Vec2Maths::subtract(pathEnd2, Vec2Maths::multiply(lastSegment.edge2.normal(), outline2));
                auto edgeSegment = LineSegment<Vec2>(side1, side2);
                auto sideSegment1 = LineSegment<Vec2>(pt1, lastMidPoint);
                auto sideSegmentN = sideSegment1.normal();
                outline1 = correct(sideSegment1.a.y, startY31, flatten, outlineWidth);
                outline2 = correct(sideSegment1.b.y, startY31, flatten, outlineWidth);
                sideSegment1.a = Vec2Maths::add(sideSegment1.a, Vec2Maths::multiply(sideSegmentN, outline1));
                sideSegment1.b = Vec2Maths::add(sideSegment1.b, Vec2Maths::multiply(sideSegmentN, outline2));
                auto secSide1 = LineSegment<Vec2>::intersection(edgeSegment, sideSegment1, true);
                auto arrowSide1 = secSide1.second ? secSide1.first : pt1;
                auto sideSegment2 = LineSegment<Vec2>(pt2, lastMidPoint);
                sideSegmentN = sideSegment2.normal();
                outline1 = correct(sideSegment2.a.y, startY31, flatten, outlineWidth);
                outline2 = correct(sideSegment2.b.y, startY31, flatten, outlineWidth);
                sideSegment2.a = Vec2Maths::subtract(sideSegment2.a, Vec2Maths::multiply(sideSegmentN, outline1));
                sideSegment2.b = Vec2Maths::subtract(sideSegment2.b, Vec2Maths::multiply(sideSegmentN, outline2));
                auto secSide2 = LineSegment<Vec2>::intersection(edgeSegment, sideSegment2, true);
                auto arrowSide2 = secSide2.second ? secSide2.first : pt2;
                auto secSide3 = LineSegment<Vec2>::intersection(sideSegment1, sideSegment2, true);
                auto arrowSide3 = secSide3.second ? secSide3.first : lastMidPoint;

                auto bottom = hasHeights ? top - outline : noHeight;
                auto tipBottom = hasHeights ? tip - outline : noHeight;

                auto innerColor = nearOutlineColor;
                auto outerColor = farOutlineColor;
                if (showOutline && hasOutlineColorMapping) {
                    innerColor *= outlineColorizationMapping.last();
                    outerColor *= outlineColorizationMapping.last();
                }

                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side1.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side1.y;
                pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pathEnd1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pathEnd1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart1.x;
                pVertex->positionXYZD[2] = arrowStart1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[2] = pt1.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side1.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt1.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide1.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = arrowSide1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt1.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide3.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = arrowSide3.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = lastMidPoint.x;
                pVertex->positionXYZD[1] = tip;
                pVertex->positionXYZD[2] = lastMidPoint.y;
                pVertex->positionXYZD[3] = tipDistance;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt2.y;
                pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide3.x;
                pVertex->positionXYZD[1] = tipBottom;
                pVertex->positionXYZD[2] = arrowSide3.y;
                pVertex->positionXYZD[3] = tipDistance;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt2.y;
                pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = arrowSide2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = arrowSide2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt2.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pt2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pt2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = arrowStart2.x;
                pVertex->positionXYZD[2] = arrowStart2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathEnd2.x;
                pVertex->positionXYZD[2] = pathEnd2.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = side2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = side2.y;
                *vertices++ = vertex;
            }
        } else if (endCapStyle == EndCapStyle::ROUND) {
			// draw half circle last end cap
			auto lastColor = hasColorMapping ? colorizationMapping.last() : fillColor;
			auto innerColor = nearOutlineColor;
			auto outerColor = farOutlineColor;
            if (showOutline && hasOutlineColorMapping) {
			    innerColor *= outlineColorizationMapping.last();
			    outerColor *= outlineColorizationMapping.last();
            }
			auto lastDistance = hasDistances ? distances.last() : noDistance;
			auto lastHeight = hasHeights ? heights.last() : noHeight;
            createTriangleFan(vertex, vertices, lastColor, lastColor, innerColor, innerColor, outerColor, outerColor,
                outlineWidth, lastDistance, lastHeight, outline, lastSegment.center.b, lastSegment.center.b,
                lastSegment.edge2.b, lastSegment.edge1.b, endSide1, nextStartSide1, false, startY31, flatten);
        } else if (startCapStyle != EndCapStyle::JOINT || endCapStyle != EndCapStyle::JOINT) {
            auto pEnd1 = pathEnd1;
            auto pEnd2 = pathEnd2;
            auto pCenter = lastSegment.center.b;
            if (endCapStyle == EndCapStyle::SQUARE) {
                // extend the end points by half the thickness
                pEnd1 = Vec2Maths::add(pEnd1, Vec2Maths::multiply(lastSegment.edge1.direction(), pathEndEdgeOffset));
                pEnd2 = Vec2Maths::add(pEnd2, Vec2Maths::multiply(lastSegment.edge2.direction(), pathEndEdgeOffset));
                pCenter = Vec2Maths::add(
                    pCenter, Vec2Maths::multiply(lastSegment.center.direction(), pathEndEdgeOffset));

                auto distance = hasDistances ? distances.last() : noDistance;
                auto top = hasHeights ? heights.last() : noHeight;

                pVertex->color = hasColorMapping ? colorizationMapping.last() : fillColor;

                pVertex->positionXYZD[0] = lastSegment.center.b.x;
			    pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = lastSegment.center.b.y;
			    pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pCenter.x;
                pVertex->positionXYZD[2] = pCenter.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pEnd1.x;
                pVertex->positionXYZD[2] = pEnd1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathEnd1.x;
                pVertex->positionXYZD[2] = pathEnd1.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = lastSegment.center.b.x;
                pVertex->positionXYZD[2] = lastSegment.center.b.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pathEnd2.x;
                pVertex->positionXYZD[2] = pathEnd2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pEnd2.x;
                pVertex->positionXYZD[2] = pEnd2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pCenter.x;
                pVertex->positionXYZD[2] = pCenter.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = lastSegment.center.b.x;
                pVertex->positionXYZD[2] = lastSegment.center.b.y;
                *vertices++ = vertex;
            }            

            if (showOutline)
            {
                // pull out end points for outline part
                auto outPathEndSideWidth1 = correct(pEnd1.y, startY31, flatten, outlineWidth);
                auto outPathEndSideWidth2 = correct(pEnd2.y, startY31, flatten, outlineWidth);
                auto outPathEndSide1 =
                    Vec2Maths::add(pEnd1, Vec2Maths::multiply(lastSegment.edge1.normal(), outPathEndSideWidth1));
                auto outPathEndSide2 =
                    Vec2Maths::subtract(pEnd2, Vec2Maths::multiply(lastSegment.edge2.normal(), outPathEndSideWidth2));
                auto outPathEndWidth1 = correct(outPathEndSide1.y, startY31, flatten, outlineWidth);
                auto outPathEndWidth2 = correct(outPathEndSide2.y, startY31, flatten, outlineWidth);
                auto outPathEnd1 = Vec2Maths::add(
                    outPathEndSide1, Vec2Maths::multiply(lastSegment.edge1.direction(), outPathEndWidth1));
                auto outPathEnd2 = Vec2Maths::add(
                    outPathEndSide2, Vec2Maths::multiply(lastSegment.edge2.direction(), outPathEndWidth2));

                auto distance = hasDistances ? distances.last() : noDistance;
                auto top = hasHeights ? heights.last() : noHeight;
                auto bottom = hasHeights ? heights.last() - outline : noHeight;

                auto innerColor = nearOutlineColor;
                auto outerColor = farOutlineColor;
                if (showOutline && hasOutlineColorMapping) {
                    innerColor *= outlineColorizationMapping.last();
                    outerColor *= outlineColorizationMapping.last();
                }

                // emit triangles for ending outline part
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathEndSide2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathEndSide2.y;
                pVertex->positionXYZD[3] = distance;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = outPathEnd2.x;
                pVertex->positionXYZD[2] = outPathEnd2.y;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pEnd2.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pEnd2.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathEnd2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathEnd2.y;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pCenter.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pCenter.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathEnd2.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathEnd2.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = outPathEnd1.x;
                pVertex->positionXYZD[2] = outPathEnd1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = outPathEndSide1.x;
                pVertex->positionXYZD[2] = outPathEndSide1.y;
                *vertices++ = vertex;
                
                pVertex->color = innerColor;
                pVertex->positionXYZD[0] = pEnd1.x;
                pVertex->positionXYZD[1] = top;
                pVertex->positionXYZD[2] = pEnd1.y;
                *vertices++ = vertex;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = pCenter.x;
                pVertex->positionXYZD[2] = pCenter.y;
                *vertices++ = vertex;
                
                pVertex->color = outerColor;
                pVertex->positionXYZD[0] = outPathEnd1.x;
                pVertex->positionXYZD[1] = bottom;
                pVertex->positionXYZD[2] = outPathEnd1.y;
                *vertices++ = vertex;

                if (endCapStyle == EndCapStyle::SQUARE) {
                    auto outPrevEndSideWidth1 = correct(pathEnd1.y, startY31, flatten, outlineWidth);
                    auto outPrevEndSideWidth2 = correct(pathEnd2.y, startY31, flatten, outlineWidth);
                    auto outPrevEndSide1 = Vec2Maths::add(
                        pathEnd1, Vec2Maths::multiply(lastSegment.edge1.normal(), outPrevEndSideWidth1));
                    auto outPrevEndSide2 = Vec2Maths::subtract(
                        pathEnd2, Vec2Maths::multiply(lastSegment.edge2.normal(), outPrevEndSideWidth2));

                    pVertex->color = outerColor;
                    pVertex->positionXYZD[0] = outPathEndSide1.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = outPathEndSide1.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = outPrevEndSide1.x;
                    pVertex->positionXYZD[2] = outPrevEndSide1.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = innerColor;
                    pVertex->positionXYZD[0] = pathEnd1.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = pathEnd1.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = pEnd1.x;
                    pVertex->positionXYZD[2] = pEnd1.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = outerColor;
                    pVertex->positionXYZD[0] = outPathEndSide1.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = outPathEndSide1.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = outPrevEndSide2.x;
                    pVertex->positionXYZD[2] = outPrevEndSide2.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = outPathEndSide2.x;
                    pVertex->positionXYZD[2] = outPathEndSide2.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = innerColor;
                    pVertex->positionXYZD[0] = pEnd2.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = pEnd2.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = pathEnd2.x;
                    pVertex->positionXYZD[2] = pathEnd2.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = outerColor;
                    pVertex->positionXYZD[0] = outPrevEndSide2.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = outPrevEndSide2.y;
                    *vertices++ = vertex;
                }
            }
        }

		return vertices;
	}

private:
	static constexpr float pi = 3.14159265358979323846f;

	/**
	 * The minimum length of a segment.
	 */
	static constexpr float segmentMinLength = 0.1;

	/**
	 * The threshold for mitered joints.
	 * If the joint's angle is smaller than this angle,
	 * the joint will be drawn beveled instead.
	 */
	static constexpr float miterMinAngle = 0.349066; // ~20 degrees

	/**
	 * The minimum angle of a round joint's triangles.
	 */
	static constexpr float roundMinAngle = 0.174533; // ~10 degrees

	/**
	 * The full range of map in 31-coordinates.
	 */
	static constexpr double intFull = static_cast<double>(INT32_MAX) + 1.0;

    template<typename Vec2>
	struct PolySegment {
		PolySegment(const LineSegment<Vec2> &center, float startThickness, float endThickness) :
				center(center),
                edge1(center),
                edge2(center) {
				// calculate the segment's outer edges by offsetting
				// the central line by the normal vector
				// multiplied with the thickness

                auto norm = center.normal();
                auto startShift = Vec2Maths::multiply(norm, startThickness);
                auto endShift = Vec2Maths::multiply(norm, endThickness);
                edge1.a = (center + startShift).a;
                edge1.b = (center + endShift).b;
                edge2.a = (center - startShift).a;
                edge2.b = (center - endShift).b;
            }

		LineSegment<Vec2> center, edge1, edge2;
	};

	inline static float correct(double y, double startY31, float flatten, float thickness) {
        if (isnan(startY31))
            return thickness;
        auto f = cosh((2.0 * M_PI * (y + startY31)) / intFull - M_PI);
        return thickness * (f + flatten - f * flatten);
    }

	template<typename Vec2, typename OutputIterator>
	inline static OutputIterator createJoint(OsmAnd::VectorMapSymbol::Vertex &vertex,
	                                  OutputIterator vertices,
	                                  const PolySegment<Vec2> &segment1,
                                      const PolySegment<Vec2> &segment2,
                                      const Vec2 &start1,
                                      const Vec2 &start2,
                                      const Vec2 &startSide1,
                                      const Vec2 &startSide2,
                                      const OsmAnd::FColorARGB &endColor,
                                      const OsmAnd::FColorARGB &nextColor,
                                      const OsmAnd::FColorARGB &endNearOutlineColor,
                                      const OsmAnd::FColorARGB &nextNearOutlineColor,
                                      const OsmAnd::FColorARGB &endFarOutlineColor,
                                      const OsmAnd::FColorARGB &nextFarOutlineColor,
                                      float outlineWidth,
                                      float distance,
                                      float height,
                                      float outlineHeight,
	                                  JointStyle jointStyle,
                                      Vec2 &end1, Vec2 &end2,
	                                  Vec2 &nextStart1, Vec2 &nextStart2,
                                      Vec2 &endSide1, Vec2 &endSide2,
	                                  Vec2 &nextStartSide1, Vec2 &nextStartSide2,
                                      bool solidColors,
                                      double startY31,
                                      float flatten) {
        OsmAnd::VectorMapSymbol::Vertex* pVertex = &vertex;
        bool showOutline = outlineWidth >= 0.0f;

		// calculate the angle between the two line segments
		auto dir1 = segment1.center.direction();
		auto dir2 = segment2.center.direction();

		auto angle = Vec2Maths::angle(dir1, dir2);

        if (jointStyle == JointStyle::MITER && angle > pi / 2 && pi - angle < miterMinAngle) {
            // to avoid the intersection point being extremely far out,
            // thus producing an enormous joint like a rasta on 4/20,
            // we render the joint beveled instead.
            jointStyle = JointStyle::BEVEL;
        }

        auto thisSegment1 = segment1.edge1;
        auto thisSegment2 = segment1.edge2;
        thisSegment1.a = start1;
        thisSegment2.a = start2;

        auto thisSegmentSide1 = segment1.edge1;
        auto thisSegmentSide2 = segment1.edge2;
        auto nextSegmentSide1 = segment2.edge1;
        auto nextSegmentSide2 = segment2.edge2;

        if (showOutline) {
            auto thisSegmentSideN = thisSegmentSide1.normal();
            auto thisSegmentOutline = correct(thisSegmentSide1.a.y, startY31, flatten, outlineWidth);
            thisSegmentSide1.a =
                Vec2Maths::add(thisSegmentSide1.a, Vec2Maths::multiply(thisSegmentSideN, thisSegmentOutline));
            thisSegmentOutline = correct(thisSegmentSide1.b.y, startY31, flatten, outlineWidth);
            thisSegmentSide1.b =
                Vec2Maths::add(thisSegmentSide1.b, Vec2Maths::multiply(thisSegmentSideN, thisSegmentOutline));
            thisSegmentSideN = thisSegmentSide2.normal();
            thisSegmentOutline = correct(thisSegmentSide2.a.y, startY31, flatten, outlineWidth);
            thisSegmentSide2.a =
                Vec2Maths::subtract(thisSegmentSide2.a, Vec2Maths::multiply(thisSegmentSideN, thisSegmentOutline));
            thisSegmentOutline = correct(thisSegmentSide2.b.y, startY31, flatten, outlineWidth);
            thisSegmentSide2.b =
                Vec2Maths::subtract(thisSegmentSide2.b, Vec2Maths::multiply(thisSegmentSideN, thisSegmentOutline));
            auto nextSegmentSideN = nextSegmentSide1.normal();
            auto nextSegmentOutline = correct(nextSegmentSide1.a.y, startY31, flatten, outlineWidth);
            nextSegmentSide1.a =
                Vec2Maths::add(nextSegmentSide1.a, Vec2Maths::multiply(nextSegmentSideN, nextSegmentOutline));
            nextSegmentOutline = correct(nextSegmentSide1.b.y, startY31, flatten, outlineWidth);
            nextSegmentSide1.b =
                Vec2Maths::add(nextSegmentSide1.b, Vec2Maths::multiply(nextSegmentSideN, nextSegmentOutline));
            nextSegmentSideN = nextSegmentSide2.normal();
            nextSegmentOutline = correct(nextSegmentSide2.a.y, startY31, flatten, outlineWidth);
            nextSegmentSide2.a =
                Vec2Maths::subtract(nextSegmentSide2.a, Vec2Maths::multiply(nextSegmentSideN, nextSegmentOutline));
            nextSegmentOutline = correct(nextSegmentSide2.b.y, startY31, flatten, outlineWidth);
            nextSegmentSide2.b =
                Vec2Maths::subtract(nextSegmentSide2.b, Vec2Maths::multiply(nextSegmentSideN, nextSegmentOutline));
            thisSegmentSide1.a = startSide1;
            thisSegmentSide2.a = startSide2;
        }

		bool almostParallel = angle < roundMinAngle;
        if (jointStyle == JointStyle::MITER || almostParallel) {
			auto sec1 = LineSegment<Vec2>::intersection(thisSegment1, segment2.edge1, false);
			auto sec2 = LineSegment<Vec2>::intersection(thisSegment2, segment2.edge2, false);
            if (!sec1.second && !sec2.second && !almostParallel) {
                jointStyle = JointStyle::BEVEL;
            } else {
                sec1 = LineSegment<Vec2>::intersection(segment1.edge1, segment2.edge1, true);
                sec2 = LineSegment<Vec2>::intersection(segment1.edge2, segment2.edge2, true);

                end1 = sec1.second ? sec1.first : segment1.edge1.b;
                end2 = sec2.second ? sec2.first : segment1.edge2.b;

                nextStart1 = end1;
                nextStart2 = end2;

                if (showOutline) {
                    auto secSide1 = LineSegment<Vec2>::intersection(thisSegmentSide1, nextSegmentSide1, false);
                    auto secSide2 = LineSegment<Vec2>::intersection(thisSegmentSide2, nextSegmentSide2, false);
                    if (!sec1.second && !sec2.second && !almostParallel) {
                        jointStyle = JointStyle::BEVEL;
                    } else {
                        secSide1 = LineSegment<Vec2>::intersection(thisSegmentSide1, nextSegmentSide1, true);
                        secSide2 = LineSegment<Vec2>::intersection(thisSegmentSide2, nextSegmentSide2, true);

                        endSide1 = secSide1.second ? secSide1.first : thisSegmentSide1.b;
                        endSide2 = secSide2.second ? secSide2.first : thisSegmentSide2.b;

                        nextStartSide1 = endSide1;
                        nextStartSide2 = endSide2;
                    }
                }
            }
        }

        if (jointStyle != JointStyle::MITER) {
			// joint style is either BEVEL or ROUND

			// find out which are the inner edges for this joint
			auto x1 = dir1.x;
			auto x2 = dir2.x;
			auto y1 = dir1.y;
			auto y2 = dir2.y;

			auto clockwise = x1 * y2 - x2 * y1 < 0;

            bool withIntersection = true;
            Vec2 innerSideSec, innerStartSide, endSide, nextStartSide;
            if (showOutline) {
                const LineSegment<Vec2> *innerSide1, *innerSide2, *outerSide1, *outerSide2;

                // as the normal vector is rotated counter-clockwise,
                // the first edge lies to the left
                // from the central line's perspective,
                // and the second one to the right.
                if (clockwise) {
                    outerSide1 = &thisSegmentSide1;
                    outerSide2 = &nextSegmentSide1;
                    innerSide1 = &thisSegmentSide2;
                    innerSide2 = &nextSegmentSide2;
                } else {
                    outerSide1 = &thisSegmentSide2;
                    outerSide2 = &nextSegmentSide2;
                    innerSide1 = &thisSegmentSide1;
                    innerSide2 = &nextSegmentSide1;
                }

    			// calculate the intersection point of the inner edges of outline
                auto innerSideSecOpt = LineSegment<Vec2>::intersection(*innerSide1, *innerSide2, false);
                withIntersection = innerSideSecOpt.second;
                innerSideSec = withIntersection
                                ? innerSideSecOpt.first
                                : innerSide1->b;

                if (withIntersection) {
                    innerStartSide = innerSideSec;
                } else {
                    innerStartSide = innerSide2->a;
                }
            }

			const LineSegment<Vec2> *inner1, *inner2, *outer1, *outer2;

			// as the normal vector is rotated counter-clockwise,
			// the first edge lies to the left
			// from the central line's perspective,
			// and the second one to the right.
			if (clockwise) {
				outer1 = &thisSegment1;
				outer2 = &segment2.edge1;
				inner1 = &thisSegment2;
				inner2 = &segment2.edge2;
			} else {
				outer1 = &thisSegment2;
				outer2 = &segment2.edge2;
				inner1 = &thisSegment1;
				inner2 = &segment2.edge1;
			}

			// calculate the intersection point of the inner edges
			auto innerSecOpt = LineSegment<Vec2>::intersection(*inner1, *inner2, false);
            withIntersection = withIntersection && innerSecOpt.second;
			Vec2 innerSec, innerStart;
			if (withIntersection) {
				innerStart = innerSec = innerSecOpt.first;
			} else {
				innerStart = inner2->a;
                auto secondNorm = inner2->normal();
                auto nextDist = Vec2Maths::dot(secondNorm, inner2->a);
                auto startDist = Vec2Maths::dot(secondNorm, inner1->a) - nextDist;
                auto endDist = Vec2Maths::dot(secondNorm, inner1->b) - nextDist;
                innerSec = /* Disabled due to artifacts on the spherical map:
                    !showOutline && ((startDist > 0 && endDist > 0) || (startDist < 0 && endDist < 0))
                    ? segment1.center.b : */
                    inner1->b;
			}

			if (clockwise) {
				end1 = outer1->b;
				end2 = innerSec;

				nextStart1 = outer2->a;
				nextStart2 = innerStart;

			} else {
				end1 = innerSec;
				end2 = outer1->b;

				nextStart1 = innerStart;
				nextStart2 = outer2->a;
			}

			// connect the intersection points according to the joint style
            pVertex->positionXYZD[3] = distance;
            float top = height;
            float bottom = height == OsmAnd::VectorMapSymbol::_absentElevation ? height : height - outlineHeight;
			if (jointStyle == JointStyle::BEVEL) {
                // simply connect the intersection points
				const Vec2& firstPoint = clockwise ? outer1->b : outer2->a;
				const Vec2& secondPoint = clockwise ? outer2->a : outer1->b;
                auto middlePoint = Vec2((firstPoint.x + secondPoint.x) / 2, (firstPoint.y + secondPoint.y) / 2);
                
                bool takeEndColor = clockwise || !(withIntersection && solidColors);
                bool takeNextColor = clockwise && withIntersection && solidColors;

                pVertex->positionXYZD[1] = top;
                pVertex->color = takeEndColor ? endColor : nextColor;
                pVertex->positionXYZD[0] = middlePoint.x;
                pVertex->positionXYZD[2] = middlePoint.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = firstPoint.x;
                pVertex->positionXYZD[2] = firstPoint.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = segment1.center.b.x;
                pVertex->positionXYZD[2] = segment1.center.b.y;
                *vertices++ = vertex;
                
                pVertex->color = takeNextColor ? nextColor : endColor;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = secondPoint.x;
                pVertex->positionXYZD[2] = secondPoint.y;
                *vertices++ = vertex;
                
                pVertex->positionXYZD[0] = middlePoint.x;
                pVertex->positionXYZD[2] = middlePoint.y;
                *vertices++ = vertex;
                if (showOutline) {
                    auto sideSegment = LineSegment<Vec2>(outer1->b, outer2->a);
                    if (clockwise) {
                        auto segmentSideN = sideSegment.normal();
                        auto sideSegmentOutline = correct(sideSegment.a.y, startY31, flatten, outlineWidth);
                        sideSegment.a =
                            Vec2Maths::add(sideSegment.a, Vec2Maths::multiply(segmentSideN, sideSegmentOutline));
                        sideSegmentOutline = correct(sideSegment.b.y, startY31, flatten, outlineWidth);
                        sideSegment.b =
                            Vec2Maths::add(sideSegment.b, Vec2Maths::multiply(segmentSideN, sideSegmentOutline));
                        auto secEndSide = LineSegment<Vec2>::intersection(thisSegmentSide1, sideSegment, true);
                        endSide = secEndSide.second ? secEndSide.first : thisSegmentSide1.b;
                        auto secNextStartSide = LineSegment<Vec2>::intersection(nextSegmentSide1, sideSegment, true);
    	    			nextStartSide = secNextStartSide.second ? secNextStartSide.first : nextSegmentSide1.a;
                    } else {
                        auto segmentSideN = sideSegment.normal();
                        auto sideSegmentOutline = correct(sideSegment.a.y, startY31, flatten, outlineWidth);
                        sideSegment.a =
                            Vec2Maths::subtract(sideSegment.a, Vec2Maths::multiply(segmentSideN, sideSegmentOutline));
                        sideSegmentOutline = correct(sideSegment.b.y, startY31, flatten, outlineWidth);
                        sideSegment.b =
                            Vec2Maths::subtract(sideSegment.b, Vec2Maths::multiply(segmentSideN, sideSegmentOutline));
                        auto secEndSide = LineSegment<Vec2>::intersection(thisSegmentSide2, sideSegment, true);
                        endSide = secEndSide.second ? secEndSide.first : thisSegmentSide2.b;
                        auto secNextStartSide = LineSegment<Vec2>::intersection(nextSegmentSide2, sideSegment, true);
	    	    		nextStartSide = secNextStartSide.second ? secNextStartSide.first : nextSegmentSide2.a;
                    }
                    Vec2& firstSidePoint = clockwise ? endSide : nextStartSide;
                    Vec2& secondSidePoint = clockwise ? nextStartSide : endSide;
                    auto middleSidePoint = Vec2((firstSidePoint.x + secondSidePoint.x) / 2,
                        (firstSidePoint.y + secondSidePoint.y) / 2);

                    auto& firstNearOutlineColor = takeEndColor ? endNearOutlineColor : nextNearOutlineColor;
                    auto& secondNearOutlineColor = takeNextColor ? nextNearOutlineColor : endNearOutlineColor;
                    auto& firstFarOutlineColor = takeEndColor ? endFarOutlineColor : nextFarOutlineColor;
                    auto& secondFarOutlineColor = takeNextColor ? nextFarOutlineColor : endFarOutlineColor;

                    pVertex->color = firstFarOutlineColor;
                    pVertex->positionXYZD[0] = middleSidePoint.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = middleSidePoint.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = firstSidePoint.x;
                    pVertex->positionXYZD[2] = firstSidePoint.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = firstNearOutlineColor;
                    pVertex->positionXYZD[0] = firstPoint.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = firstPoint.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = middlePoint.x;
                    pVertex->positionXYZD[2] = middlePoint.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = firstFarOutlineColor;
                    pVertex->positionXYZD[0] = middleSidePoint.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = middleSidePoint.y;
                    *vertices++ = vertex;

                    pVertex->color = secondFarOutlineColor;
                    *vertices++ = vertex;

                    pVertex->color = secondNearOutlineColor;
                    pVertex->positionXYZD[0] = middlePoint.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = middlePoint.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = secondFarOutlineColor;
                    pVertex->positionXYZD[0] = secondSidePoint.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = secondSidePoint.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->color = secondNearOutlineColor;
                    pVertex->positionXYZD[0] = middlePoint.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = middlePoint.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = secondPoint.x;
                    pVertex->positionXYZD[2] = secondPoint.y;
                    *vertices++ = vertex;
                }
			} else {
				// draw a circle between the ends of the outer edges,
				// centered at the actual point
				// with half the line thickness as the radius
                auto secondColor = withIntersection && solidColors ? nextColor : endColor;
                auto secondNearColor = withIntersection && solidColors ? nextNearOutlineColor : endNearOutlineColor;
                auto secondFarColor = withIntersection && solidColors ? nextFarOutlineColor : endFarOutlineColor;
                createTriangleFan(vertex, vertices, endColor, secondColor, endNearOutlineColor, secondNearColor,
                    endFarOutlineColor, secondFarColor, outlineWidth, distance, height, outlineHeight,
                    segment1.center.b, segment1.center.b, outer1->b, outer2->a, endSide, nextStartSide, clockwise,
                    startY31, flatten);
			}
            if (showOutline) {
                if (clockwise) {
                    endSide1 = endSide;
                    endSide2 = innerSideSec;

                    nextStartSide1 = nextStartSide;
                    nextStartSide2 = innerStartSide;
                } else {
                    endSide1 = innerSideSec;
                    endSide2 = endSide;

                    nextStartSide1 = innerStartSide;
                    nextStartSide2 = nextStartSide;
                }
            }
		}

		return vertices;
	}

	/**
	 * Creates a partial circle between two points.
	 * The points must be equally far away from the origin.
	 * @param vertices The vector to add vertices to.
	 * @param connectTo The position to connect the triangles to.
	 * @param origin The circle's origin.
	 * @param start The circle's starting point.
	 * @param end The circle's ending point.
	 * @param clockwise Whether the circle's rotation is clockwise.
	 */
	template<typename Vec2, typename OutputIterator>
	inline static OutputIterator createTriangleFan(OsmAnd::VectorMapSymbol::Vertex &vertex,
	                                        OutputIterator vertices,
                                            const OsmAnd::FColorARGB &endColor,
                                            const OsmAnd::FColorARGB &nextColor,
                                            const OsmAnd::FColorARGB &endNearOutlineColor,
                                            const OsmAnd::FColorARGB &nextNearOutlineColor,
                                            const OsmAnd::FColorARGB &endFarOutlineColor,
                                            const OsmAnd::FColorARGB &nextFarOutlineColor,
                                            float outlineWidth,
                                            float distance,
                                            float height,
                                            float outlineHeight,
                                            const Vec2 &connectTo,
                                            const Vec2 &origin,
	                                        const Vec2 &start,
                                            const Vec2 &end,
                                            Vec2 &endSide,
                                            Vec2 &nextStartSide,
                                            bool clockwise,
                                            double startY31,
                                            float flatten) {

        OsmAnd::VectorMapSymbol::Vertex* pVertex = &vertex;
		auto point1 = Vec2Maths::subtract(start, origin);
		auto point2 = Vec2Maths::subtract(end, origin);

		// calculate the angle between the two points
		auto angle1 = atan2(point1.y, point1.x);
		auto angle2 = atan2(point2.y, point2.x);

		// ensure the outer angle is calculated
		if (clockwise) {
			if (angle2 > angle1) {
				angle2 = angle2 - 2 * pi;
			}
		} else {
			if (angle1 > angle2) {
				angle1 = angle1 - 2 * pi;
			}
		}

		auto jointAngle = angle2 - angle1;

		// calculate the amount of triangles to use for the joint
		auto halfOfTriangles = std::max(1, (int) std::floor(std::abs(jointAngle) / roundMinAngle) / 2);
        auto numTriangles = halfOfTriangles * 2;

		// calculate the angle of each triangle
		auto triAngle = jointAngle / numTriangles;

		Vec2 startPoint = start;
		Vec2 endPoint, startPointSide, endPointSide;
        pVertex->positionXYZD[3] = distance;
        float top = height;
        float bottom = height == OsmAnd::VectorMapSymbol::_absentElevation ? height : height - outlineHeight;
        bool showOutline = outlineWidth >= 0.0f;
        if (showOutline) {
            auto startOutline = correct(start.y, startY31, flatten, outlineWidth);
            startPointSide = Vec2Maths::add(start, Vec2Maths::multiply(Vec2Maths::normalized(point1), startOutline));
            endSide = startPointSide;
        }
		for (int t = 0; t < numTriangles; t++) {
			if (t + 1 == numTriangles) {
				// it's the last triangle - ensure it perfectly
				// connects to the next line
				endPoint = end;

				// pull out end point for outline
                if (showOutline) {
                    auto endOutline = correct(end.y, startY31, flatten, outlineWidth);
                    endPointSide = Vec2Maths::add(end, Vec2Maths::multiply(Vec2Maths::normalized(point2), endOutline));
                }
			} else {
				auto rot = (t + 1) * triAngle;

				// rotate the original point around the origin
				endPoint.x = std::cos(rot) * point1.x - std::sin(rot) * point1.y;
				endPoint.y = std::sin(rot) * point1.x + std::cos(rot) * point1.y;

				// pull out end point for outline
                if (showOutline) {
                    auto pointOutline = correct(endPoint.y + origin.y, startY31, flatten, outlineWidth);
                    endPointSide =
                        Vec2Maths::add(endPoint, Vec2Maths::multiply(Vec2Maths::normalized(endPoint), pointOutline));
                    endPointSide = Vec2Maths::add(endPointSide, origin);
                }

				// re-add the rotation origin to the target point
				endPoint = Vec2Maths::add(endPoint, origin);
			}
            
            // emit the triangle
            pVertex->color = t < halfOfTriangles ? endColor : nextColor;
            pVertex->positionXYZD[1] = top;
            
            if (!clockwise) {
                pVertex->positionXYZD[0] = startPoint.x;
                pVertex->positionXYZD[2] = startPoint.y;
                *vertices++ = vertex;
            }
            
            pVertex->positionXYZD[0] = endPoint.x;
            pVertex->positionXYZD[2] = endPoint.y;
            *vertices++ = vertex;
            
            if (clockwise) {
                pVertex->positionXYZD[0] = startPoint.x;
                pVertex->positionXYZD[2] = startPoint.y;
                *vertices++ = vertex;
            }

            pVertex->positionXYZD[0] = connectTo.x;
            pVertex->positionXYZD[2] = connectTo.y;
            *vertices++ = vertex;

            if (showOutline) {
                // emit two trianges for outline
                pVertex->color = t < halfOfTriangles ? endFarOutlineColor : nextFarOutlineColor;
                if (clockwise) {
                    pVertex->positionXYZD[0] = endPointSide.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = endPointSide.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = startPointSide.x;
                    pVertex->positionXYZD[2] = startPointSide.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = t < halfOfTriangles ? endNearOutlineColor : nextNearOutlineColor;
                    pVertex->positionXYZD[0] = startPoint.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = startPoint.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = endPoint.x;
                    pVertex->positionXYZD[2] = endPoint.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = t < halfOfTriangles ? endFarOutlineColor : nextFarOutlineColor;
                    pVertex->positionXYZD[0] = endPointSide.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = endPointSide.y;
                    *vertices++ = vertex;
                } else {
                    pVertex->positionXYZD[0] = startPointSide.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = startPointSide.y;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = endPointSide.x;
                    pVertex->positionXYZD[2] = endPointSide.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = t < halfOfTriangles ? endNearOutlineColor : nextNearOutlineColor;
                    pVertex->positionXYZD[0] = endPoint.x;
                    pVertex->positionXYZD[1] = top;
                    pVertex->positionXYZD[2] = endPoint.y;
                    *vertices++ = vertex;
                    *vertices++ = vertex;
                    
                    pVertex->positionXYZD[0] = startPoint.x;
                    pVertex->positionXYZD[2] = startPoint.y;
                    *vertices++ = vertex;
                    
                    pVertex->color = t < halfOfTriangles ? endFarOutlineColor : nextFarOutlineColor;
                    pVertex->positionXYZD[0] = startPointSide.x;
                    pVertex->positionXYZD[1] = bottom;
                    pVertex->positionXYZD[2] = startPointSide.y;
                    *vertices++ = vertex;
                }
    			startPointSide = endPointSide;
            }

			startPoint = endPoint;
		}

        if (showOutline) {
            nextStartSide = endPointSide;
        }

		return vertices;
	}
};

} // namespace crushedpixel
