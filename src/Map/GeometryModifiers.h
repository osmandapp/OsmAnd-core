#ifndef _OSMAND_CORE_GEOMETRY_MODIFIERS_H_
#define _OSMAND_CORE_GEOMETRY_MODIFIERS_H_

#include <algorithm>
#include <cmath>
#include <deque>
#include <unordered_map>

#include "AtlasMapRenderer.h"
#include "Utilities.h"
#include <OsmAndCore/Map/VectorMapSymbol.h>
#include <OsmAndCore/Color.h>

namespace OsmAnd
{
struct OSMAND_CORE_API GeometryModifiers Q_DECL_FINAL
{
	struct VertexAdv
	{
		// Coordinates
		float x;
		float y;
		float z; // height
		float d; // distance

		// Grid line code
		int32_t g;

		// Color
		FColorARGB color;
	};

	struct VertexForPath
	{
		// Coordinates
		float x;
		float y;
		float z; // height
		float d; // distance

		// Color
		FColorARGB color;

		// Trace color
		FColorARGB traceColor;
	};

	// Triangle
	struct Triangle
	{
		VertexAdv A, B, C;
	};

	// Fragment signature
	struct FragSignature
	{
		uint64_t x, y, g, cx, cy;
	};

	class FragSigHash
	{
	   public:
		size_t operator()(const FragSignature& key) const
		{
			return key.x ^ key.y << 12 ^ key.g << 24 ^ key.cx << 36 ^ key.cx >> 28 ^ key.cy << 48 ^ key.cy >> 16;
		}
	};

	class FragSigEqual
	{
	   public:
		bool operator()(const FragSignature& k1, const FragSignature& k2) const
		{
			return k1.x == k2.x && k1.y == k2.y && k1.g == k2.g && k1.cx == k2.cx && k1.cy == k2.cy;
		}
	};

	// Trim float for hash
	inline static uint32_t trimForHash(float& value)
	{
		auto ui = *reinterpret_cast<uint32_t*>(&value);
		return (ui & 32) > 0 ? ui + 32 & -32 : ui & -32;
	}

	// Create fragment signature
	inline static void makeSignature(FragSignature& signature, int32_t& g, VertexAdv& A, VertexAdv& C)
	{
		signature.x = trimForHash(A.x);
		signature.y = trimForHash(A.y);
		signature.g = g;
		signature.cx = trimForHash(C.x);
		signature.cy = trimForHash(C.y);
	}

	// Simple color interpolation
	inline static FColorARGB middleColor(const FColorARGB& color1, const FColorARGB& color2, const float& Nt)
	{
		return {(color2.a - color1.a) * Nt + color1.a, (color2.r - color1.r) * Nt + color1.r,
				(color2.g - color1.g) * Nt + color1.g, (color2.b - color1.b) * Nt + color1.b};
	}

	// Paving with two triangles
	inline static void twoParts(std::deque<VertexAdv>& inObj, const VertexAdv& A, const VertexAdv& B,
								const VertexAdv& C, const VertexAdv& D, const int32_t& rodCode)
	{
		inObj.push_back(A);
		inObj.push_back({B.x, B.y, B.z, B.d, rodCode, B.color});
		inObj.push_back(D);
		inObj.push_back(C);
		inObj.push_back({D.x, D.y, D.z, D.d, rodCode, D.color});
		inObj.push_back({B.x, B.y, B.z, B.d, A.g, B.color});
	}

	// Paving with three triangles
	inline static void threeParts(std::deque<VertexAdv>& inObj, const VertexAdv& A, const VertexAdv& B,
								  const VertexAdv& C, const VertexAdv& D, const VertexAdv& E, const int32_t& rodCode,
								  const bool simplify)
	{
		bool rewire = false;
		if (simplify && C.g == 0 && A.g == 0 && D.g != 0)
			rewire = true;
		else if (!simplify || C.g != 0 || A.g == 0 || D.g != 0)
		{
			float x = B.y - D.y;
			float y = D.x - B.x;
			float r = sqrt(x * x + y * y);
			x /= r;
			y /= r;
			float m = x * B.x + y * B.y;
			m = fmin(fabs(x * C.x + y * C.y - m), fabs(x * E.x + y * E.y - m));
			x = C.y - E.y;
			y = E.x - C.x;
			r = sqrt(x * x + y * y);
			x /= r;
			y /= r;
			r = x * C.x + y * C.y;
			r = fmin(fabs(x * B.x + y * B.y - r), fabs(x * D.x + y * D.y - r));
			rewire = m < r;
		}
		inObj.push_back(A);
		inObj.push_back({B.x, B.y, B.z, B.d, rodCode, B.color});
		inObj.push_back({E.x, E.y, E.z, E.d, D.g, E.color});
		if (rewire)
		{
			inObj.push_back({B.x, B.y, B.z, B.d, A.g, B.color});
			inObj.push_back({C.x, C.y, C.z, C.d, 0, C.color});
			inObj.push_back({E.x, E.y, E.z, E.d, rodCode, E.color});
			inObj.push_back(C);
			inObj.push_back(D);
			inObj.push_back(E);
		}
		else
		{
			inObj.push_back(B);
			inObj.push_back(D);
			inObj.push_back({E.x, E.y, E.z, E.d, rodCode, E.color});
			inObj.push_back(C);
			inObj.push_back({D.x, D.y, D.z, D.d, 0, D.color});
			inObj.push_back({B.x, B.y, B.z, B.d, A.g, B.color});
		}
	}

	// Check points in line
	inline static bool straightRod(const VertexAdv& A, const VertexAdv& B, const VertexAdv& C,
								   const float& maxBreakTangent)
	{
		float vx = B.x - A.x;
		float vy = B.y - A.y;
		if (fabs(vx) < fabs(vy))
		{
			float dy = C.y - B.y;
			return fabs(dy) > 0.0f ? fabs((C.x - B.x) / dy - vx / vy) < maxBreakTangent : false;
		}
		else
		{
			float dx = C.x - B.x;
			return fabs(dx) > 0.0f ? fabs((C.y - B.y) / dx - vy / vx) < maxBreakTangent : false;
		}
	}

	// Test triangles for CCW and rewire if not
	inline static void ccwTriangle(VectorMapSymbol::Vertex* A, VectorMapSymbol::Vertex* B, VectorMapSymbol::Vertex* C)
	{
		float s = A->positionXYZD[0] * B->positionXYZD[2] - A->positionXYZD[2] * B->positionXYZD[0];
		s += B->positionXYZD[0] * C->positionXYZD[2] - B->positionXYZD[2] * C->positionXYZD[0];
		s += C->positionXYZD[0] * A->positionXYZD[2] - C->positionXYZD[2] * A->positionXYZD[0];
		if (s < 0.0f)
		{
			VectorMapSymbol::Vertex* D = B;
			B = C;
			C = D;
		}
	}

	// Get triange from vertex
	inline static void getVertex(VectorMapSymbol::Vertex* vertex, std::deque<VertexAdv>& outQueue)
	{
        outQueue.push_back(
            {vertex->positionXYZD[0],
            vertex->positionXYZD[2],
            vertex->positionXYZD[1],
            vertex->positionXYZD[3],
            0,
            vertex->color});
	}

	// Put triangle to the tile list
	inline static void putTriangle(std::map<TileId, std::vector<VectorMapSymbol::Vertex>>& meshes, const VertexAdv& A,
								   const VertexAdv& B, const VertexAdv& C, const double tileSize,
								   const PointD& tilePosN)
	{
		auto x = static_cast<int32_t>(
			floor((static_cast<double>(A.x) + static_cast<double>(B.x) + static_cast<double>(C.x)) / 3.0 / tileSize +
				  tilePosN.x));
		auto y = static_cast<int32_t>(
			floor((static_cast<double>(A.y) + static_cast<double>(B.y) + static_cast<double>(C.y)) / 3.0 / tileSize +
				  tilePosN.y));
		auto tileId = TileId::fromXY(x, y);
		auto ftile = meshes.find(tileId);
		if (ftile != meshes.end())
		{
			ftile->second.push_back({{A.x, A.z, A.y, A.d}, A.color});
			ftile->second.push_back({{B.x, B.z, B.y, B.d}, B.color});
			ftile->second.push_back({{C.x, C.z, C.y, C.d}, C.color});
		}
		else
		{
			std::vector<VectorMapSymbol::Vertex> tvec;
			tvec.push_back({{A.x, A.z, A.y, A.d}, A.color});
			tvec.push_back({{B.x, B.z, B.y, B.d}, B.color});
			tvec.push_back({{C.x, C.z, C.y, C.d}, C.color});
			meshes.insert({tileId, tvec});
		}
	}

	// Cut the mesh by tiles and grid cells (+ optional mesh optimization)
	static bool cutMeshWithGrid(std::vector<VectorMapSymbol::Vertex>& vertices,
						const std::shared_ptr<std::vector<VectorMapSymbol::Index>>& indices,
						const VectorMapSymbol::PrimitiveType& primitiveType,
						std::shared_ptr<std::vector<std::pair<TileId, int32_t>>>& partSizes,
						const ZoomLevel zoomLevel,
						const PointD& tilePosN, const int32_t cellsPerTileSize, const float minDistance,
						const float maxBreakTangent, const bool diagonals, const bool simplify,
                        std::vector<VectorMapSymbol::Vertex>& outVertices);

	// Create vertical faces for path, cutting it by tiles and grid cells
	static bool getTesselatedPlane(std::vector<VectorMapSymbol::Vertex>& vertices,
						const std::vector<OsmAnd::PointD>& points,
						QList<float>& distances,
						QList<float>& heights,
						FColorARGB& fillColor,
						FColorARGB& topColor,
						FColorARGB& bottomColor,
						QList<FColorARGB>& colorizationMapping,
						QList<FColorARGB>& traceColorizationMapping,
						const float roofHeight,
						const ZoomLevel zoomLevel,
						const PointD& tilePosN,
						const int32_t cellsPerTileSize,
						const float minDistance,
						const bool diagonals,
						const bool solidColors);

   private:
	GeometryModifiers();
	~GeometryModifiers();
};
}  // namespace OsmAnd
#endif	// !defined(_OSMAND_CORE_GEOMETRY_MODIFIERS_H_)