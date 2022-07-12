#ifndef _OSMAND_CORE_GEOMETRY_MODIFIERS_H_
#define _OSMAND_CORE_GEOMETRY_MODIFIERS_H_

#include "VectorMapSymbol.h"

#include <cmath>
#include <algorithm>
#include <deque>
#include <unordered_map>

namespace OsmAnd
{

    struct OSMAND_CORE_API GeometryModifiers Q_DECL_FINAL
    {

        struct VertexAdv {

            // XY coordinates: Y up, X right
            float x;
            float y;

            // Grid line code
            int32_t g;

            // Color
            FColorARGB color;
        };

        // Triangle
        struct Triangle {
            VertexAdv A, B, C;
        };

        // Fragment signature
        struct FragSignature {
            uint64_t x, y, g, cx, cy;
        };

        class FragSigHash {
            public:
            size_t operator() (const FragSignature& key) const {
                return key.x ^ key.y << 12 ^ key.g << 24 ^ key.cx << 36 ^ key.cx >> 28 ^ key.cy << 48 ^ key.cy >> 16;
            }
        };

        class FragSigEqual {
            public:
            bool operator() (const FragSignature& k1, const FragSignature& k2) const {
                return k1.x == k2.x && k1.y == k2.y && k1.g == k2.g && k1.cx == k2.cx && k1.cy == k2.cy;
            }
        };

        // Trim float for hash
        inline static uint32_t trimForHash(float& value) {
            auto ui = *reinterpret_cast<uint32_t*>(&value);
            return (ui & 32) > 0 ? ui + 32 & -32 : ui & -32;
        }

        // Create fragment signature
        inline static void makeSignature(FragSignature& signature,
                                int32_t& g,
                                VertexAdv& A,
                                VertexAdv& C) {
            signature.x = trimForHash(A.x);
            signature.y = trimForHash(A.y);
            signature.g = g;
            signature.cx = trimForHash(C.x);
            signature.cy = trimForHash(C.y);
        }

        // Paving with two triangles
        inline static void twoParts(std::deque<VertexAdv>& inObj,
                                const VertexAdv& A,
                                const VertexAdv& B,
                                const VertexAdv& C,
                                const VertexAdv& D,
                                const int32_t& rodCode) {
            inObj.push_back(A);
            inObj.push_back({ B.x, B.y, rodCode, A.color }); // TODO: Set calculated color
            inObj.push_back(D);
            inObj.push_back(C);
            inObj.push_back({ D.x, D.y, rodCode, D.color });
            inObj.push_back({ B.x, B.y, A.g, A.color }); // TODO: Set calculated color
        }

        // Paving with three triangles
        inline static void threeParts(std::deque<VertexAdv>& inObj,
                                const VertexAdv& A,
                                const VertexAdv& B,
                                const VertexAdv& C,
                                const VertexAdv& D,
                                const VertexAdv& E,
                                const int32_t& rodCode,
                                const bool simplify) {
            bool rewire = false;
            if (simplify && C.g == 0 && A.g == 0 && D.g != 0)
                rewire = true;
            else if (!simplify || C.g != 0 || A.g == 0 || D.g != 0) {
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
            inObj.push_back({ B.x, B.y, rodCode, A.color }); // TODO: Set calculated color
            inObj.push_back({ E.x, E.y, D.g, D.color }); // TODO: Set calculated color
            if (rewire) {
                inObj.push_back({ B.x, B.y, A.g, A.color }); // TODO: Set calculated color
                inObj.push_back({ C.x, C.y, 0, C.color });
                inObj.push_back({ E.x, E.y, rodCode, D.color }); // TODO: Set calculated color
                inObj.push_back(C);
                inObj.push_back(D);
                inObj.push_back(E);
            }
            else {
                inObj.push_back(B);
                inObj.push_back(D);
                inObj.push_back({ E.x, E.y, rodCode, D.color }); // TODO: Set calculated color
                inObj.push_back(C);
                inObj.push_back({ D.x, D.y, 0, D.color });
                inObj.push_back({ B.x, B.y, A.g, A.color }); // TODO: Set calculated color
            }
        }

        // Check points in line
        inline static bool straightRod(const VertexAdv& A, const VertexAdv& B, const VertexAdv& C) {
            float vx = B.x - A.x;
            float vy = B.y - A.y;
            if (fabs(vx) < fabs(vy)) {
                float dy = C.y - B.y;
                return fabs(dy) > 0.0f ? fabs((C.x - B.x) / dy - vx / vy) < 0.01f : false;
            }
            else {
                float dx = C.x - B.x;
                return fabs(dx) > 0.0f ? fabs((C.y - B.y) / dx - vy / vx) < 0.01f : false;
            }
        }

        // Put triangle to the tile list
        inline static void putTriangle(std::shared_ptr<std::map<TileId, std::vector<VectorMapSymbol::Vertex>>>& meshes,
                        const VertexAdv& A,
                        const VertexAdv& B,
                        const VertexAdv& C,
                        const int32_t& gridDimXY,
                        const float& gridStepXY,
                        const float& gridPosX,
                        const float& gridPosY) {
            auto x = static_cast<int32_t>(floor(((A.x + B.x + C.x) / 3.0f - gridPosX) / gridStepXY)) / gridDimXY;
            auto y = static_cast<int32_t>(floor(((A.y + B.y + C.y) / 3.0f - gridPosY) / gridStepXY)) / gridDimXY;
            auto tileId = TileId::fromXY(x, y);
            auto ftile = meshes->find(tileId);
            if (ftile != meshes->end()) {
                ftile->second.push_back({ { A.x, A.y }, A.color });
                ftile->second.push_back({ { B.x, B.y }, B.color });
                ftile->second.push_back({ { C.x, C.y }, C.color });
            }
            else {
                std::vector<VectorMapSymbol::Vertex> tvec;
                tvec.push_back({ { A.x, A.y }, A.color });
                tvec.push_back({ { B.x, B.y }, B.color });
                tvec.push_back({ { C.x, C.y }, C.color });
                meshes->insert({ tileId, tvec });
            }
        }

        // Cut the mesh by tiles and four grid lines (+ optional mesh optimization)
        static bool overGrid(const std::shared_ptr<VectorMapSymbol>& symbol,
                std::shared_ptr<std::map<TileId, std::vector<VectorMapSymbol::Vertex>>>& meshes,
                const int32_t& gridDimXY,
                const float& gridStepXY,
                const float& gridPosX,
                const float& gridPosY,
                const float& minDistance,
                const bool simplify);
        
        private:
        GeometryModifiers();
        ~GeometryModifiers();
    };
}
#endif // !defined(_OSMAND_CORE_GEOMETRY_MODIFIERS_H_)
