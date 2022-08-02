#include "GeometryModifiers.h"

OsmAnd::GeometryModifiers::GeometryModifiers()
{
}

OsmAnd::GeometryModifiers::~GeometryModifiers()
{
}

// Cut the mesh by tiles and grid lines (+ optional mesh optimization)
bool OsmAnd::GeometryModifiers::overGrid(std::vector<VectorMapSymbol::Vertex>& vertices,
                        const std::shared_ptr<std::vector<VectorMapSymbol::Index>>& indices,
                        const VectorMapSymbol::PrimitiveType& primitiveType,
                        std::shared_ptr<std::vector<std::pair<TileId, int32_t>>>& partSizes,
                        const double& tileSize,
                        const PointD& tilePosN,
                        const float& minDistance,
                        const float& maxBreakTangent,
                        const bool diagonals,
                        const bool simplify) {
    if (partSizes == nullptr) return false;    
    if (tileSize <= 0.0 || minDistance <= 0.0f) return false;
    auto verticesCount = vertices.size();
    if (verticesCount < 3) return false;
    auto indicesCount = indices != nullptr ? indices->size() : 0;
    float minDist = minDistance;
    float gridStepXY = tileSize / static_cast<double>(AtlasMapRenderer::HeixelsPerTileSide - 1);
    float gridPosX = -(tilePosN.x - std::floor(tilePosN.x)) * tileSize;
    float gridPosY = -(tilePosN.y - std::floor(tilePosN.y)) * tileSize;    
    int32_t i, j, v;
    std::deque<VertexAdv> inObj;
    auto pv = vertices.data();
    auto pvf = pv;
    auto pvs = pv;
    auto pvt = pv;
    auto pvd = pv;
    switch (primitiveType) {
        // Unwind triangle fan
        case VectorMapSymbol::PrimitiveType::TriangleFan:
            if (indicesCount > 2) {
                v = (*indices)[0];
                if (v < verticesCount) {
                    pvf = &vertices[v];
                    v = (*indices)[1];
                    if (v < verticesCount) {
                        pvs = &vertices[v];
                        for (i = 2; i < indicesCount; i++) {
                            v = (*indices)[i];
                            if (v < verticesCount) {
                                pv = &vertices[v];
                                pvt = pv;
                                if (simplify) ccwTriangle(pvf, pvs, pvt);
                                inObj.push_back({ pvf->positionXY[0], pvf->positionXY[1], 0, pvf->color });
                                inObj.push_back({ pvs->positionXY[0], pvs->positionXY[1], 0, pvs->color });
                                inObj.push_back({ pvt->positionXY[0], pvt->positionXY[1], 0, pvt->color });
                                pvs = pv;
                            }
                            else return false;
                        }
                    }
                    else return false;
                }
                else return false;
            }
            else {
                pvf = pv;
                pv += 2;                
                for (v = 2; v < verticesCount; v++) {
                    pvs = pv - 1;
                    pvt = pv;
                    if (simplify) ccwTriangle(pvf, pvs, pvt);
                    inObj.push_back({ pvf->positionXY[0], pvf->positionXY[1], 0, pvf->color });
                    inObj.push_back({ pvs->positionXY[0], pvs->positionXY[1], 0, pvs->color });
                    inObj.push_back({ pvt->positionXY[0], pvt->positionXY[1], 0, pvt->color });
                    pv++;
                }
            }
            break;
        // Unwind triangle strip
        case VectorMapSymbol::PrimitiveType::TriangleStrip:
            if (indicesCount > 2) {
                v = (*indices)[0];
                if (v < verticesCount) {
                    pvf = &vertices[v];
                    v = (*indices)[1];
                    if (v < verticesCount) {
                        pvs = &vertices[v];
                        for (i = 2; i < indicesCount; i++) {
                            v = (*indices)[i];
                            if (v < verticesCount) {
                                pv = &vertices[v];
                                pvd = pvs;
                                pvt = pv;
                                if (simplify) ccwTriangle(pvf, pvd, pvt);
                                inObj.push_back({ pvf->positionXY[0], pvf->positionXY[1], 0, pvf->color });
                                inObj.push_back({ pvd->positionXY[0], pvd->positionXY[1], 0, pvd->color });
                                inObj.push_back({ pvt->positionXY[0], pvt->positionXY[1], 0, pvt->color });
                                if (v % 2)
                                    pvs = pv;
                                else
                                    pvf = pv;
                            }
                            else return false;
                        }
                    }
                    else return false;
                }
                else return false;
            }
            else {
                pv += 2;
                for (v = 2; v < verticesCount; v++) {
                    if (v % 2) {
                        pvf = pv - 1;
                        pvs = pv - 2;
                    }
                    else {
                        pvf = pv - 2;
                        pvs = pv - 1;
                    }
                    pvt = pv;
                    if (simplify) ccwTriangle(pvf, pvs, pvt);
                    inObj.push_back({ pvf->positionXY[0], pvf->positionXY[1], 0, pvf->color });
                    inObj.push_back({ pvs->positionXY[0], pvs->positionXY[1], 0, pvs->color });
                    inObj.push_back({ pvt->positionXY[0], pvt->positionXY[1], 0, pvt->color });
                    pv++;
                }
            }
            break;
        // Unwind triangles
        case VectorMapSymbol::PrimitiveType::Triangles:
            if (indicesCount > 2)
                for (i = 0; i < indicesCount; i += 3) {
                    v = (*indices)[i];
                    if (v < verticesCount) {
                        pvf = &vertices[v];
                        v = (*indices)[i + 1];
                        if (v < verticesCount) {
                            pvs = &vertices[v];
                            v = (*indices)[i + 2];
                            if (v < verticesCount) {
                                pvt = &vertices[v];
                                if (simplify) ccwTriangle(pvf, pvs, pvt);
                                inObj.push_back({ pvf->positionXY[0], pvf->positionXY[1], 0, pvf->color });
                                inObj.push_back({ pvs->positionXY[0], pvs->positionXY[1], 0, pvs->color });
                                inObj.push_back({ pvt->positionXY[0], pvt->positionXY[1], 0, pvt->color });
                            }
                            else return false;
                        }
                        else return false;
                    }
                    else return false;
                }
            else
                for (v = 0; v < verticesCount; v += 3) {
                    pvf = pv++;
                    pvs = pv++;
                    pvt = pv++;
                    if (simplify) ccwTriangle(pvf, pvs, pvt);
                    inObj.push_back({ pvf->positionXY[0], pvf->positionXY[1], 0, pvf->color });
                    inObj.push_back({ pvs->positionXY[0], pvs->positionXY[1], 0, pvs->color });
                    inObj.push_back({ pvt->positionXY[0], pvt->positionXY[1], 0, pvt->color });
                }
            break;
        default:
            return false;
    }
    std::map<TileId, std::vector<VectorMapSymbol::Vertex>> meshes;
    std::unordered_map<FragSignature, Triangle, FragSigHash, FragSigEqual> fragments;
    Triangle triangle;
    FragSignature signature;
    VertexAdv tgl[3], point1, point2;
    float RSQRT2 = 0.70710678118654752440084436210485L;
    float gridPosW = (gridPosX - gridPosY) * RSQRT2;
    float gridPosU = (gridPosX + gridPosY) * RSQRT2;
    float gridStepWU = gridStepXY * RSQRT2;
    float w, u, rod, rate, xMin, xMax, yMin, yMax, wMin, wMax, uMin, uMax;
    int32_t prev, next, c, cell, iMinX, iMaxX, iMinY, iMaxY, iMinW, iMaxW, iMinU, iMaxU;
    bool intoTwo;
    while (inObj.size() > 0) {
        xMin = std::numeric_limits<float>::max();
        xMax = -std::numeric_limits<float>::max();
        yMin = xMin;
        yMax = xMax;
        wMin = xMin;
        wMax = xMax;
        uMin = xMin;
        uMax = xMax;
        // get a triangle and find its boundaries:
        for (i = 0; i < 3; i++) {
            if (inObj.empty()) return false;
            tgl[i] = inObj.front();
            xMin = fmin(tgl[i].x, xMin);
            if (xMin == tgl[i].x)
                iMinX = i;
            xMax = fmax(tgl[i].x, xMax);
            if (xMax == tgl[i].x)
                iMaxX = i;
            yMin = fmin(tgl[i].y, yMin);
            if (yMin == tgl[i].y)
                iMinY = i;
            yMax = fmax(tgl[i].y, yMax);
            if (yMax == tgl[i].y)
                iMaxY = i;
            if (diagonals) {
                w = (tgl[i].x - tgl[i].y) * RSQRT2;
                wMin = fmin(w, wMin);
                if (wMin == w)
                    iMinW = i;
                wMax = fmax(w, wMax);
                if (wMax == w)
                    iMaxW = i;
                u = (tgl[i].x + tgl[i].y) * RSQRT2;
                uMin = fmin(u, uMin);
                if (uMin == u)
                    iMinU = i;
                uMax = fmax(u, uMax);
                if (uMax == u)
                    iMaxU = i;
            }
            inObj.pop_front();
        }
        // increase minimal distance in accordance to precission of float:
        minDist = fmax(fmax(fmax(fabs(xMin), fabs(xMax)), fmax(fabs(yMin), fabs(yMax))) / 1000000.0f, minDist);
        next = 0;
        prev = 3;
        rate = xMax - xMin;
        // median vertical line to cut:
        c = static_cast<int32_t>(round(((xMin + xMax) * 0.5f - gridPosX) / gridStepXY));
        rod = c * gridStepXY + gridPosX;
        i = 3 - iMinX - iMaxX;
        if (rod >= xMin + minDist && rod <= xMax - minDist) {
            if (rod >= tgl[i].x - minDist && rod <= tgl[i].x + minDist)
                prev = 2;
            cell = c * 10;
            cell += cell < 0 ? -1 : 1;
            next = 1;
        }
        else if (simplify) {
            if (xMin >= rod - minDist && xMin <= rod + minDist && xMax > rod + minDist) {
                if (rod >= tgl[i].x - minDist && rod <= tgl[i].x + minDist) {
                    c *= 10;
                    c += c < 0 ? -1 : 1;
                    tgl[iMinX + i != 2 ? std::min(iMinX, i) : 2].g = c;
                }
            }
            else if (xMax >= rod - minDist && xMax <= rod + minDist && xMin < rod - minDist) {
                if (rod >= tgl[i].x - minDist && rod <= tgl[i].x + minDist) {
                    c *= 10;
                    c += c < 0 ? -1 : 1;
                    tgl[i + iMaxX != 2 ? std::min(i, iMaxX) : 2].g = c;
                }
            }
        }
        // median horizontal line to cut:
        c = static_cast<int32_t>(round(((yMin + yMax) * 0.5f - gridPosY) / gridStepXY));
        w = c * gridStepXY + gridPosY;
        j = 3 - iMinY - iMaxY;
        if (w >= yMin + minDist && w <= yMax - minDist) {
            if (prev > 2 && w >= tgl[j].y - minDist && w <= tgl[j].y + minDist) {
                next = 0;
                prev = 2;
            }
            else if (prev == 2)
                std::swap(yMax, yMin);
            if (next == 0 || rate < yMax - yMin) {
                rate = yMax - yMin;
                cell = c * 10;
                cell += cell < 0 ? -2 : 2;
                rod = w;
                i = j;
                next = 2;
            }
        }
        else if (simplify) {
            if (yMin >= w - minDist && yMin <= w + minDist && yMax > w + minDist) {
                if (w >= tgl[j].y - minDist && w <= tgl[j].y + minDist) {
                    c *= 10;
                    c += c < 0 ? -2 : 2;
                    tgl[iMinY + j != 2 ? std::min(iMinY, j) : 2].g = c;
                }
            }
            else if (yMax >= w - minDist && yMax <= w + minDist && yMin < w - minDist) {
                if (w >= tgl[j].y - minDist && w <= tgl[j].y + minDist) {
                    c *= 10;
                    c += c < 0 ? -2 : 2;
                    tgl[j + iMaxY != 2 ? std::min(j, iMaxY) : 2].g = c;
                }
            }
        }
        if (diagonals) {
            // median diagonal line to cut (y=x):
            c = static_cast<int32_t>(round(((wMin + wMax) * 0.5f - gridPosW) / gridStepWU));
            w = c * gridStepWU + gridPosW;
            j = 3 - iMinW - iMaxW;
            if (w >= wMin + minDist && w <= wMax - minDist) {
                u = (tgl[j].x - tgl[j].y) * RSQRT2;
                if (prev > 2 && w >= u - minDist && w <= u + minDist) {
                    next = 0;
                    prev = 2;
                }
                else if (prev == 2)
                    std::swap(wMax, wMin);
                if (next == 0 || rate < wMax - wMin) {
                    rate = wMax - wMin;
                    cell = c * 10;
                    cell += cell < 0 ? -3 : 3;
                    rod = w;
                    i = j;
                    next = 3;
                }
            }
            else if (simplify) {
                if (wMin >= w - minDist && wMin <= w + minDist && wMax > w + minDist) {
                    u = (tgl[j].x - tgl[j].y) * RSQRT2;
                    if (w >= u - minDist && w <= u + minDist) {
                        c *= 10;
                        c += c < 0 ? -3 : 3;
                        tgl[iMinW + j != 2 ? std::min(iMinW, j) : 2].g = c;
                    }
                }
                else if (wMax >= w - minDist && wMax <= w + minDist && wMin < w - minDist) {
                    u = (tgl[j].x - tgl[j].y) * RSQRT2;
                    if (w >= u - minDist && w <= u + minDist) {
                        c *= 10;
                        c += c < 0 ? -3 : 3;
                        tgl[j + iMaxW != 2 ? std::min(j, iMaxW) : 2].g = c;
                    }
                }
            }
            // median diagonal line to cut (y=-x):
            c = static_cast<int32_t>(round(((uMin + uMax) * 0.5f - gridPosU) / gridStepWU));
            w = c * gridStepWU + gridPosU;
            j = 3 - iMinU - iMaxU;
            if (w >= uMin + minDist && w <= uMax - minDist) {
                u = (tgl[j].x + tgl[j].y) * RSQRT2;
                if (prev > 2 && w >= u - minDist && w <= u + minDist) {
                    next = 0;
                    prev = 2;
                }
                else if (prev == 2)
                    std::swap(uMax, uMin);
                if (next == 0 || rate < uMax - uMin) {
                    cell = c * 10;
                    cell += cell < 0 ? -4 : 4;
                    rod = w;
                    i = j;
                    next = 4;
                }
            }
            else if (simplify) {
                if (uMin >= w - minDist && uMin <= w + minDist && uMax > w + minDist) {
                    u = (tgl[j].x + tgl[j].y) * RSQRT2;
                    if (w >= u - minDist && w <= u + minDist) {
                        c *= 10;
                        c += c < 0 ? -4 : 4;
                        tgl[iMinU + j != 2 ? std::min(iMinU, j) : 2].g = c;
                    }
                }
                else if (uMax >= w - minDist && uMax <= w + minDist && uMin < w - minDist) {
                    u = (tgl[j].x + tgl[j].y) * RSQRT2;
                    if (w >= u - minDist && w <= u + minDist) {
                        c *= 10;
                        c += c < 0 ? -4 : 4;
                        tgl[j + iMaxU != 2 ? std::min(j, iMaxU) : 2].g = c;
                    }
                }
            }
        }
        switch (next) {
        case 1:
            prev = iMinX + iMaxX != 2 ? std::min(iMinX, iMaxX) : 2;
            next = iMinX + iMaxX != 2 ? std::max(iMinX, iMaxX) : 0;
            rate = (rod - xMin) / (xMax - xMin);
            point1 = { rod, tgl[iMinX].y + (tgl[iMaxX].y - tgl[iMinX].y) * rate, 0, middleColor(tgl[iMinX], tgl[iMaxX], rate) };
            intoTwo = false;
            if (rod < tgl[i].x - minDist) {
                rate = (rod - xMin) / (tgl[i].x - xMin);
                point2 = { rod, tgl[iMinX].y + (tgl[i].y - tgl[iMinX].y) * rate, 0, middleColor(tgl[iMinX], tgl[i], rate) };
            }
            else if (rod > tgl[i].x + minDist) {
                rate = (rod - tgl[i].x) / (xMax - tgl[i].x);
                point2 = { rod, tgl[i].y + (tgl[iMaxX].y - tgl[i].y) * rate, 0, middleColor(tgl[i], tgl[iMaxX], rate) };
            }
            else
                intoTwo = true;
            if (intoTwo)
                twoParts(inObj, tgl[prev], point1, tgl[next], tgl[i], cell);
            else if ((rod < tgl[i].x) == (iMaxX - iMinX == 1 || iMaxX - iMinX == -2))
                threeParts(inObj, tgl[prev], point1, tgl[next], tgl[i], point2, cell, simplify);
            else
                threeParts(inObj, tgl[next], point2, tgl[i], tgl[prev], point1, cell, simplify);
            continue;
        case 2:
            prev = iMinY + iMaxY != 2 ? std::min(iMinY, iMaxY) : 2;
            next = iMinY + iMaxY != 2 ? std::max(iMinY, iMaxY) : 0;
            rate = (rod - yMin) / (yMax - yMin);
            point1 = { tgl[iMinY].x + (tgl[iMaxY].x - tgl[iMinY].x) * rate, rod, 0, middleColor(tgl[iMinY], tgl[iMaxY], rate) };
            intoTwo = false;
            if (rod < tgl[i].y - minDist) {
                rate = (rod - yMin) / (tgl[i].y - yMin);
                point2 = { tgl[iMinY].x + (tgl[i].x - tgl[iMinY].x) * rate, rod, 0, middleColor(tgl[iMinY], tgl[i], rate) };
            }
            else if (rod > tgl[i].y + minDist) {
                rate = (rod - tgl[i].y) / (yMax - tgl[i].y);
                point2 = { tgl[i].x + (tgl[iMaxY].x - tgl[i].x) * rate, rod, 0, middleColor(tgl[i], tgl[iMaxY], rate) };
            }
            else
                intoTwo = true;
            if (intoTwo)
                twoParts(inObj, tgl[prev], point1, tgl[next], tgl[i], cell);
            else if ((rod < tgl[i].y) == (iMaxY - iMinY == 1 || iMaxY - iMinY == -2))
                threeParts(inObj, tgl[prev], point1, tgl[next], tgl[i], point2, cell, simplify);
            else
                threeParts(inObj, tgl[next], point2, tgl[i], tgl[prev], point1, cell, simplify);
            continue;
        case 3:
            prev = iMinW + iMaxW != 2 ? std::min(iMinW, iMaxW) : 2;
            next = iMinW + iMaxW != 2 ? std::max(iMinW, iMaxW) : 0;
            rate = (rod - wMin) / (wMax - wMin);
            point1 = { tgl[iMinW].x + (tgl[iMaxW].x - tgl[iMinW].x) * rate, tgl[iMinW].y + (tgl[iMaxW].y - tgl[iMinW].y) * rate, 0, middleColor(tgl[iMinW], tgl[iMaxW], rate) };
            w = (tgl[i].x - tgl[i].y) * RSQRT2;
            intoTwo = false;
            if (rod < w - minDist) {
                rate = (rod - wMin) / (w - wMin);
                point2 = { tgl[iMinW].x + (tgl[i].x - tgl[iMinW].x) * rate, tgl[iMinW].y + (tgl[i].y - tgl[iMinW].y) * rate, 0, middleColor(tgl[iMinW], tgl[i], rate) };
            }
            else if (rod > w + minDist) {
                rate = (rod - w) / (wMax - w);
                point2 = { tgl[i].x + (tgl[iMaxW].x - tgl[i].x) * rate, tgl[i].y + (tgl[iMaxW].y - tgl[i].y) * rate, 0, middleColor(tgl[i], tgl[iMaxW], rate) };
            }
            else
                intoTwo = true;
            if (intoTwo)
                twoParts(inObj, tgl[prev], point1, tgl[next], tgl[i], cell);
            else if ((rod < w) == (iMaxW - iMinW == 1 || iMaxW - iMinW == -2))
                threeParts(inObj, tgl[prev], point1, tgl[next], tgl[i], point2, cell, simplify);
            else
                threeParts(inObj, tgl[next], point2, tgl[i], tgl[prev], point1, cell, simplify);
            continue;
        case 4:
            prev = iMinU + iMaxU != 2 ? std::min(iMinU, iMaxU) : 2;
            next = iMinU + iMaxU != 2 ? std::max(iMinU, iMaxU) : 0;
            rate = (rod - uMin) / (uMax - uMin);
            point1 = { tgl[iMinU].x + (tgl[iMaxU].x - tgl[iMinU].x) * rate, tgl[iMinU].y + (tgl[iMaxU].y - tgl[iMinU].y) * rate, 0, middleColor(tgl[iMinU], tgl[iMaxU], rate) };
            u = (tgl[i].x + tgl[i].y) * RSQRT2;
            intoTwo = false;
            if (rod < u - minDist) {
                rate = (rod - uMin) / (u - uMin);
                point2 = { tgl[iMinU].x + (tgl[i].x - tgl[iMinU].x) * rate, tgl[iMinU].y + (tgl[i].y - tgl[iMinU].y) * rate, 0, middleColor(tgl[iMinU], tgl[i], rate) };
            }
            else if (rod > u + minDist) {
                rate = (rod - u) / (uMax - u);
                point2 = { tgl[i].x + (tgl[iMaxU].x - tgl[i].x) * rate, tgl[i].y + (tgl[iMaxU].y - tgl[i].y) * rate, 0, middleColor(tgl[i], tgl[iMaxU], rate) };
            }
            else
                intoTwo = true;
            if (intoTwo)
                twoParts(inObj, tgl[prev], point1, tgl[next], tgl[i], cell);
            else if ((rod < u) == (iMaxU - iMinU == 1 || iMaxU - iMinU == -2))
                threeParts(inObj, tgl[prev], point1, tgl[next], tgl[i], point2, cell, simplify);
            else
                threeParts(inObj, tgl[next], point2, tgl[i], tgl[prev], point1, cell, simplify);
            continue;
        }
        // output the produced triangle (to the map of fragments or to results)
        intoTwo = !simplify || (tgl[0].g == 0 && tgl[1].g == 0 && tgl[2].g == 0) || (tgl[0].g != 0 && tgl[1].g != 0 && tgl[2].g != 0);
        if (!intoTwo) {
            for (i = 0; i < 3; i++)
                if (tgl[i].g != 0 && tgl[(i + 2) % 3].g == 0)
                    break;
            triangle.A = tgl[i];
            i = (i + 1) % 3;
            triangle.B = tgl[i];
            i = (i + 1) % 3;
            triangle.C = tgl[i];
            makeSignature(signature, triangle.A.g, triangle.A, triangle.C);
            intoTwo = !fragments.insert({ signature, triangle }).second;
        }
        if (intoTwo)
            putTriangle(meshes, tgl[0], tgl[1], tgl[2], tileSize, tilePosN);
    }
    // Merge fragments and store triangles
    if (simplify) {
        auto found = fragments.begin();
        FragSignature oldFrag;
        do {
            i = 0;
            j = 0;
            auto frag = fragments.begin();
            if (frag != fragments.end()) {
                do {
                    do {
                        intoTwo = true;
                        if (frag->second.B.g == 0) {
                            makeSignature(signature, frag->second.A.g, frag->second.B, frag->second.C);
                            if ((found = fragments.find(signature)) != frag && found != fragments.end() && straightRod(frag->second.A, frag->second.B, found->second.B, maxBreakTangent)) {
                                frag->second.B = found->second.B;
                                fragments.erase(found);
                                intoTwo = false;
                            }
                        }
                        else if (frag->second.C.g == 0) {
                            makeSignature(signature, frag->second.B.g, frag->second.C, frag->second.A);
                            if ((found = fragments.find(signature)) != frag && found != fragments.end() && straightRod(frag->second.B, frag->second.C, found->second.B, maxBreakTangent)) {
                                frag->second.C = found->second.B;
                                triangle = frag->second;
                                fragments.erase(found);
                                intoTwo = false;
                                j = 1;
                                if (triangle.C.g == 0)
                                    j++;
                                oldFrag = frag->first;
                            }
                        }
                    } while (!intoTwo);
                    frag++;
                    if (j > 0) {
                        fragments.erase(oldFrag);
                        if (j > 1) {
                            makeSignature(signature, triangle.A.g, triangle.A, triangle.C);
                            if (fragments.insert({ signature, triangle }).second)
                                i = 1;
                            else
                                j = 1;
                        }
                        if (j == 1)
                            putTriangle(meshes, triangle.A, triangle.B, triangle.C, tileSize, tilePosN);
                        j = 0;
                    }
                } while (frag != fragments.end());
            }
        } while (i > 0);
        for (const auto frag : fragments)
            putTriangle(meshes, frag.second.A, frag.second.B, frag.second.C, tileSize, tilePosN);
    }
    vertices.clear();
    partSizes->clear();
    for (const auto mesh : meshes) {
        vertices.insert(vertices.end(), mesh.second.begin(), mesh.second.end());
        partSizes->push_back({mesh.first, mesh.second.size()});
    }
    return true;
}