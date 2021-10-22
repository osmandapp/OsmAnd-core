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
		JOINT
	};

	/**
	 * Creates a vector of vertices describing a solid path through the input points.
	 * @param points The points of the path.
	 * @param thickness The path's thickness.
	 * @param jointStyle The path's joint style.
	 * @param endCapStyle The path's end cap style.
	 * @param allowOverlap Whether to allow overlapping vertices.
	 * 					   This yields better results when dealing with paths
	 * 					   whose points have a distance smaller than the thickness,
	 * 					   but may introduce overlapping vertices,
	 * 					   which is undesirable when rendering transparent paths.
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
    static std::vector<OsmAnd::VectorMapSymbol::Vertex> create(OsmAnd::VectorMapSymbol::Vertex &vertex, const InputCollection &points, float thickness,
                                                               OsmAnd::FColorARGB fillColor,
                                                               QList<OsmAnd::FColorARGB> colorizationMapping,
                                                               JointStyle jointStyle = JointStyle::MITER,
                                                               EndCapStyle endCapStyle = EndCapStyle::BUTT,
	                                bool allowOverlap = false) {
		std::vector<OsmAnd::VectorMapSymbol::Vertex> vertices;
		create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(vertex, vertices, points, thickness, fillColor, colorizationMapping, jointStyle, endCapStyle, allowOverlap);
		return vertices;
	}

	template<typename Vertex>
    static std::vector<OsmAnd::VectorMapSymbol::Vertex> create(OsmAnd::VectorMapSymbol::Vertex &vertex, const std::vector<OsmAnd::PointD> &points, float thickness,
                                                               OsmAnd::FColorARGB fillColor,
                                                               QList<OsmAnd::FColorARGB> colorizationMapping,
                                                               JointStyle jointStyle = JointStyle::MITER,
                                                               EndCapStyle endCapStyle = EndCapStyle::BUTT,
                                                               bool allowOverlap = false) {
        std::vector<OsmAnd::VectorMapSymbol::Vertex> vertices;
		create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(vertex, vertices, points, thickness, fillColor, colorizationMapping, jointStyle, endCapStyle, allowOverlap);
		return vertices;
	}

	template<typename Vertex, typename InputCollection>
    static size_t create(OsmAnd::VectorMapSymbol::Vertex &vertex, std::vector<OsmAnd::VectorMapSymbol::Vertex> &vertices, const InputCollection &points, float thickness,
                         OsmAnd::FColorARGB fillColor,
                         QList<OsmAnd::FColorARGB> colorizationMapping,
                         JointStyle jointStyle = JointStyle::MITER,
                         EndCapStyle endCapStyle = EndCapStyle::BUTT,
                         bool allowOverlap = false) {
		auto numVerticesBefore = vertices.size();

        create<Vertex, InputCollection>(vertex, std::back_inserter(vertices), points, thickness,
                                                                 fillColor, colorizationMapping,
                                                                 jointStyle, endCapStyle, allowOverlap);

		return vertices.size() - numVerticesBefore;
	}

	template<typename Vertex, typename InputCollection, typename OutputIterator>
	static OutputIterator create(OsmAnd::VectorMapSymbol::Vertex &vertex, OutputIterator vertices, const InputCollection &points, float thickness,
                                 OsmAnd::FColorARGB fillColor,
                                 QList<OsmAnd::FColorARGB> colorizationMapping,
	                             JointStyle jointStyle = JointStyle::MITER,
	                             EndCapStyle endCapStyle = EndCapStyle::BUTT,
	                             bool allowOverlap = false) {
        OsmAnd::VectorMapSymbol::Vertex* pVertex = &vertex;
        
		// operate on half the thickness to make our lives easier
		thickness /= 2;

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
				segments.emplace_back(LineSegment<Vec2>(point1, point2), thickness);
			}
		}

		if (endCapStyle == EndCapStyle::JOINT) {
			// create a connecting segment from the last to the first point

			auto &pointD1 = points[points.size() - 1];
			auto &pointD2 = points[0];
            
            Vec2 point1 = Vec2(pointD1.x, pointD1.y);
            Vec2 point2 = Vec2(pointD2.x, pointD2.y);

			// to avoid division-by-zero errors,
			// only create a line segment for non-identical points
			if (!Vec2Maths::equal(point1, point2)) {
				segments.emplace_back(LineSegment<Vec2>(point1, point2), thickness);
			}
		}

		if (segments.empty()) {
			// handle the case of insufficient input points
			return vertices;
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
        
        bool hasColorMapping = !colorizationMapping.isEmpty();

		// handle different end cap styles
		if (endCapStyle == EndCapStyle::SQUARE) {
			// extend the start/end points by half the thickness
			pathStart1 = Vec2Maths::subtract(pathStart1, Vec2Maths::multiply(firstSegment.edge1.direction(), thickness));
			pathStart2 = Vec2Maths::subtract(pathStart2, Vec2Maths::multiply(firstSegment.edge2.direction(), thickness));
			pathEnd1 = Vec2Maths::add(pathEnd1, Vec2Maths::multiply(lastSegment.edge1.direction(), thickness));
			pathEnd2 = Vec2Maths::add(pathEnd2, Vec2Maths::multiply(lastSegment.edge2.direction(), thickness));

		} else if (endCapStyle == EndCapStyle::ROUND) {
			// draw half circle end caps
			createTriangleFan(vertex, vertices, hasColorMapping ? colorizationMapping.first() : fillColor, firstSegment.center.a, firstSegment.center.a,
			                  firstSegment.edge1.a, firstSegment.edge2.a, false);
			createTriangleFan(vertex, vertices, hasColorMapping ? colorizationMapping.last() : fillColor, lastSegment.center.b, lastSegment.center.b,
			                  lastSegment.edge1.b, lastSegment.edge2.b, true);

		} else if (endCapStyle == EndCapStyle::JOINT) {
			// join the last (connecting) segment and the first segment
			createJoint(vertex, vertices, lastSegment, firstSegment, hasColorMapping ? colorizationMapping.last() : fillColor, jointStyle,
			            pathEnd1, pathEnd2, pathStart1, pathStart2, allowOverlap);
		}

		// generate mesh data for path segments
		for (size_t i = 0; i < segments.size(); i++) {
			auto &segment = segments[i];
            
            auto &startColor = !colorizationMapping.isEmpty() && i < colorizationMapping.size() ? colorizationMapping[i] : fillColor;
            auto &endColor = !colorizationMapping.isEmpty() && i < colorizationMapping.size() - 1 ? colorizationMapping[i + 1] : fillColor;

			// calculate start
			if (i == 0) {
				// this is the first segment
				start1 = pathStart1;
				start2 = pathStart2;
			}

			if (i + 1 == segments.size()) {
				// this is the last segment
				end1 = pathEnd1;
				end2 = pathEnd2;

			} else {
                auto& nextSegment = segments[i + 1];
                // calculate the angle between the two line segments
                auto dir1 = segment.center.direction();
                auto dir2 = nextSegment.center.direction();
                auto angle = Vec2Maths::angle(dir1, dir2);
                
				createJoint(vertex, vertices, segment, segments[i + 1], endColor, jointStyle,
				            end1, end2, nextStart1, nextStart2, angle <= 3 * pi / 4);
			}
            
            // emit vertices
            pVertex->color = startColor;
            pVertex->positionXY[0] = start1.x,
            pVertex->positionXY[1] = start1.y;
            *vertices++ = vertex;
            
            pVertex->positionXY[0] = start2.x,
            pVertex->positionXY[1] = start2.y;
            *vertices++ = vertex;
            
            pVertex->color = endColor;
            pVertex->positionXY[0] = end1.x;
            pVertex->positionXY[1] = end1.y;
            *vertices++ = vertex;
            *vertices++ = vertex;
            
            pVertex->color = startColor;
            pVertex->positionXY[0] = start2.x,
            pVertex->positionXY[1] = start2.y;
            *vertices++ = vertex;
            
            pVertex->color = endColor;
            pVertex->positionXY[0] = end2.x;
            pVertex->positionXY[1] = end2.y;
            *vertices++ = vertex;

			start1 = nextStart1;
			start2 = nextStart2;
		}

		return vertices;
	}

private:
	static constexpr float pi = 3.14159265358979323846f;

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

	template<typename Vec2>
	struct PolySegment {
		PolySegment(const LineSegment<Vec2> &center, float thickness) :
				center(center),
				// calculate the segment's outer edges by offsetting
				// the central line by the normal vector
				// multiplied with the thickness

				// center + center.normal() * thickness
				edge1(center + Vec2Maths::multiply(center.normal(), thickness)),
				edge2(center - Vec2Maths::multiply(center.normal(), thickness)) {}

		LineSegment<Vec2> center, edge1, edge2;
	};

	template<typename Vec2, typename OutputIterator>
	static OutputIterator createJoint(OsmAnd::VectorMapSymbol::Vertex &vertex, OutputIterator vertices,
	                                  const PolySegment<Vec2> &segment1, const PolySegment<Vec2> &segment2,
                                      const OsmAnd::FColorARGB fillColor,
	                                  JointStyle jointStyle, Vec2 &end1, Vec2 &end2,
	                                  Vec2 &nextStart1, Vec2 &nextStart2,
	                                  bool allowOverlap) {
        OsmAnd::VectorMapSymbol::Vertex* pVertex = &vertex;
		// calculate the angle between the two line segments
		auto dir1 = segment1.center.direction();
		auto dir2 = segment2.center.direction();

		auto angle = Vec2Maths::angle(dir1, dir2);

		// wrap the angle around the 180° mark if it exceeds 90°
		// for minimum angle detection
		auto wrappedAngle = angle;
		if (wrappedAngle > pi / 2) {
			wrappedAngle = pi - wrappedAngle;
		}

		if (jointStyle == JointStyle::MITER && wrappedAngle < miterMinAngle) {
			// the minimum angle for mitered joints wasn't exceeded.
			// to avoid the intersection point being extremely far out,
			// thus producing an enormous joint like a rasta on 4/20,
			// we render the joint beveled instead.
			jointStyle = JointStyle::BEVEL;
		}

		if (jointStyle == JointStyle::MITER) {
			// calculate each edge's intersection point
			// with the next segment's central line
			auto sec1 = LineSegment<Vec2>::intersection(segment1.edge1, segment2.edge1, true);
			auto sec2 = LineSegment<Vec2>::intersection(segment1.edge2, segment2.edge2, true);

			end1 = sec1.second ? sec1.first : segment1.edge1.b;
			end2 = sec2.second ? sec2.first : segment1.edge2.b;

			nextStart1 = end1;
			nextStart2 = end2;

		} else {
			// joint style is either BEVEL or ROUND

			// find out which are the inner edges for this joint
			auto x1 = dir1.x;
			auto x2 = dir2.x;
			auto y1 = dir1.y;
			auto y2 = dir2.y;

			auto clockwise = x1 * y2 - x2 * y1 < 0;

			const LineSegment<Vec2> *inner1, *inner2, *outer1, *outer2;

			// as the normal vector is rotated counter-clockwise,
			// the first edge lies to the left
			// from the central line's perspective,
			// and the second one to the right.
			if (clockwise) {
				outer1 = &segment1.edge1;
				outer2 = &segment2.edge1;
				inner1 = &segment1.edge2;
				inner2 = &segment2.edge2;
			} else {
				outer1 = &segment1.edge2;
				outer2 = &segment2.edge2;
				inner1 = &segment1.edge1;
				inner2 = &segment2.edge1;
			}

			// calculate the intersection point of the inner edges
			auto innerSecOpt = LineSegment<Vec2>::intersection(*inner1, *inner2, allowOverlap);

			auto innerSec = innerSecOpt.second
			                ? innerSecOpt.first
			                // for parallel lines, simply connect them directly
			                : inner1->b;

			// if there's no inner intersection, flip
			// the next start position for near-180° turns
			Vec2 innerStart;
			if (innerSecOpt.second) {
				innerStart = innerSec;
			} else if (angle > pi / 2) {
				innerStart = outer1->b;
			} else {
				innerStart = inner1->b;
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

			if (jointStyle == JointStyle::BEVEL) {
				// simply connect the intersection points
                
                pVertex->color = fillColor;
                
                pVertex->positionXY[0] = outer1->b.x;
                pVertex->positionXY[1] = outer1->b.y;
                *vertices++ = vertex;
                
                pVertex->positionXY[0] = outer2->a.x;
                pVertex->positionXY[1] = outer2->a.y;
                *vertices++ = vertex;
                
                pVertex->positionXY[0] = innerSec.x;
                pVertex->positionXY[1] = innerSec.y;
                *vertices++ = vertex;

			} else if (jointStyle == JointStyle::ROUND) {
				// draw a circle between the ends of the outer edges,
				// centered at the actual point
				// with half the line thickness as the radius
				createTriangleFan(vertex, vertices, fillColor, innerSec, segment1.center.b, outer1->b, outer2->a, clockwise);
			} else {
				assert(false);
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
	static OutputIterator createTriangleFan(OsmAnd::VectorMapSymbol::Vertex &vertex, OutputIterator vertices, const OsmAnd::FColorARGB fillColor, Vec2 connectTo, Vec2 origin,
	                                        Vec2 start, Vec2 end, bool clockwise) {

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
		auto numTriangles = std::max(1, (int) std::floor(std::abs(jointAngle) / roundMinAngle));

		// calculate the angle of each triangle
		auto triAngle = jointAngle / numTriangles;

		Vec2 startPoint = start;
		Vec2 endPoint;
		for (int t = 0; t < numTriangles; t++) {
			if (t + 1 == numTriangles) {
				// it's the last triangle - ensure it perfectly
				// connects to the next line
				endPoint = end;
			} else {
				auto rot = (t + 1) * triAngle;

				// rotate the original point around the origin
				endPoint.x = std::cos(rot) * point1.x - std::sin(rot) * point1.y;
				endPoint.y = std::sin(rot) * point1.x + std::cos(rot) * point1.y;

				// re-add the rotation origin to the target point
				endPoint = Vec2Maths::add(endPoint, origin);
			}
            
            // emit the triangle
            pVertex->color = fillColor;
            
            pVertex->positionXY[0] = startPoint.x;
            pVertex->positionXY[1] = startPoint.y;
            *vertices++ = vertex;
            
            pVertex->positionXY[0] = endPoint.x;
            pVertex->positionXY[1] = endPoint.y;
            *vertices++ = vertex;
            
            pVertex->positionXY[0] = connectTo.x;
            pVertex->positionXY[1] = connectTo.y;
            *vertices++ = vertex;

			startPoint = endPoint;
		}

		return vertices;
	}
};

} // namespace crushedpixel
