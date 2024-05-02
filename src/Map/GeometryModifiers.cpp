#include "GeometryModifiers.h"

OsmAnd::GeometryModifiers::GeometryModifiers() {}

OsmAnd::GeometryModifiers::~GeometryModifiers() {}

// Cut the mesh by tiles and grid cells (+ optional mesh optimization)
bool OsmAnd::GeometryModifiers::cutMeshWithGrid(std::vector<VectorMapSymbol::Vertex>& vertices,
		const std::shared_ptr<std::vector<VectorMapSymbol::Index>>& indices,
		const VectorMapSymbol::PrimitiveType& primitiveType,
		std::shared_ptr<std::vector<std::pair<TileId, int32_t>>>& partSizes,
		const ZoomLevel zoomLevel, const PointD& tilePosN, const int32_t cellsPerTileSize,
		const float minDistance, const float maxBreakTangent, const bool diagonals, const bool simplify)
{
	if (!partSizes)
        return false;
	if (minDistance <= 0.0f)
        return false;
	auto verticesCount = vertices.size();
	if (verticesCount < 3)
        return false;
	auto indicesCount = indices != nullptr ? indices->size() : 0;
	float minDist = minDistance;
    auto tileSize = Utilities::getPowZoom(MaxZoomLevel - zoomLevel);
	float gridStepXY = tileSize / static_cast<double>(cellsPerTileSize);
	float gridPosX = -(tilePosN.x - std::floor(tilePosN.x)) * tileSize;
	float gridPosY = -(tilePosN.y - std::floor(tilePosN.y)) * tileSize;
	int32_t i, j, v;
	std::deque<VertexAdv> inObj;
	auto pv = vertices.data();
	auto pvf = pv;
	auto pvs = pv;
	auto pvt = pv;
	auto pvd = pv;
	switch (primitiveType)
	{
		// Unwind triangle fan
		case VectorMapSymbol::PrimitiveType::TriangleFan:
			if (indicesCount > 2)
			{
				v = (*indices)[0];
				if (v < verticesCount)
				{
					pvf = &vertices[v];
					v = (*indices)[1];
					if (v < verticesCount)
					{
						pvs = &vertices[v];
						for (i = 2; i < indicesCount; i++)
						{
							v = (*indices)[i];
							if (v < verticesCount)
							{
								pv = &vertices[v];
								pvt = pv;
								if (simplify)
									ccwTriangle(pvf, pvs, pvt);
								getVertex(pvf, inObj);
								getVertex(pvs, inObj);
								getVertex(pvt, inObj);
								pvs = pv;
							}
							else
								return false;
						}
					}
					else
						return false;
				}
				else
					return false;
			}
			else
			{
				pvf = pv;
				pv += 2;
				for (v = 2; v < verticesCount; v++)
				{
					pvs = pv - 1;
					pvt = pv;
					if (simplify)
						ccwTriangle(pvf, pvs, pvt);
					getVertex(pvf, inObj);
					getVertex(pvs, inObj);
					getVertex(pvt, inObj);
					pv++;
				}
			}
			break;
		// Unwind triangle strip
		case VectorMapSymbol::PrimitiveType::TriangleStrip:
			if (indicesCount > 2)
			{
				v = (*indices)[0];
				if (v < verticesCount)
				{
					pvf = &vertices[v];
					v = (*indices)[1];
					if (v < verticesCount)
					{
						pvs = &vertices[v];
						for (i = 2; i < indicesCount; i++)
						{
							v = (*indices)[i];
							if (v < verticesCount)
							{
								pv = &vertices[v];
								pvd = pvs;
								pvt = pv;
								if (simplify)
									ccwTriangle(pvf, pvd, pvt);
								getVertex(pvf, inObj);
								getVertex(pvd, inObj);
								getVertex(pvt, inObj);
								if (v % 2)
									pvs = pv;
								else
									pvf = pv;
							}
							else
								return false;
						}
					}
					else
						return false;
				}
				else
					return false;
			}
			else
			{
				pv += 2;
				for (v = 2; v < verticesCount; v++)
				{
					if (v % 2)
					{
						pvf = pv - 1;
						pvs = pv - 2;
					}
					else
					{
						pvf = pv - 2;
						pvs = pv - 1;
					}
					pvt = pv;
					if (simplify)
						ccwTriangle(pvf, pvs, pvt);
					getVertex(pvf, inObj);
					getVertex(pvs, inObj);
					getVertex(pvt, inObj);
					pv++;
				}
			}
			break;
		// Unwind triangles
		case VectorMapSymbol::PrimitiveType::Triangles:
			if (indicesCount > 2)
				for (i = 0; i < indicesCount; i += 3)
				{
					v = (*indices)[i];
					if (v < verticesCount)
					{
						pvf = &vertices[v];
						v = (*indices)[i + 1];
						if (v < verticesCount)
						{
							pvs = &vertices[v];
							v = (*indices)[i + 2];
							if (v < verticesCount)
							{
								pvt = &vertices[v];
								if (simplify)
									ccwTriangle(pvf, pvs, pvt);
								getVertex(pvf, inObj);
								getVertex(pvs, inObj);
								getVertex(pvt, inObj);
							}
							else
								return false;
						}
						else
							return false;
					}
					else
						return false;
				}
			else
				for (v = 0; v < verticesCount; v += 3)
				{
					pvf = pv++;
					pvs = pv++;
					pvt = pv++;
					if (simplify)
						ccwTriangle(pvf, pvs, pvt);
					getVertex(pvf, inObj);
					getVertex(pvs, inObj);
					getVertex(pvt, inObj);
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
	while (inObj.size() > 0)
	{
		xMin = std::numeric_limits<float>::max();
		xMax = -std::numeric_limits<float>::max();
		yMin = xMin;
		yMax = xMax;
		wMin = xMin;
		wMax = xMax;
		uMin = xMin;
		uMax = xMax;
		// get a triangle and find its boundaries:
		for (i = 0; i < 3; i++)
		{
			if (inObj.empty()) return false;
			tgl[i] = inObj.front();
			xMin = fmin(tgl[i].x, xMin);
			if (xMin == tgl[i].x) iMinX = i;
			xMax = fmax(tgl[i].x, xMax);
			if (xMax == tgl[i].x) iMaxX = i;
			yMin = fmin(tgl[i].y, yMin);
			if (yMin == tgl[i].y) iMinY = i;
			yMax = fmax(tgl[i].y, yMax);
			if (yMax == tgl[i].y) iMaxY = i;
			if (diagonals)
			{
				w = (tgl[i].x - tgl[i].y) * RSQRT2;
				wMin = fmin(w, wMin);
				if (wMin == w) iMinW = i;
				wMax = fmax(w, wMax);
				if (wMax == w) iMaxW = i;
				u = (tgl[i].x + tgl[i].y) * RSQRT2;
				uMin = fmin(u, uMin);
				if (uMin == u) iMinU = i;
				uMax = fmax(u, uMax);
				if (uMax == u) iMaxU = i;
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
		if (rod >= xMin + minDist && rod <= xMax - minDist)
		{
			if (rod >= tgl[i].x - minDist && rod <= tgl[i].x + minDist)
                prev = 2;
			cell = c * 10;
			cell += cell < 0 ? -1 : 1;
			next = 1;
		}
		else if (simplify)
		{
			if (xMin >= rod - minDist && xMin <= rod + minDist && xMax > rod + minDist)
			{
				if (rod >= tgl[i].x - minDist && rod <= tgl[i].x + minDist)
				{
					c *= 10;
					c += c < 0 ? -1 : 1;
					tgl[iMinX + i != 2 ? std::min(iMinX, i) : 2].g = c;
				}
			}
			else if (xMax >= rod - minDist && xMax <= rod + minDist && xMin < rod - minDist)
			{
				if (rod >= tgl[i].x - minDist && rod <= tgl[i].x + minDist)
				{
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
		if (w >= yMin + minDist && w <= yMax - minDist)
		{
			if (prev > 2 && w >= tgl[j].y - minDist && w <= tgl[j].y + minDist)
			{
				next = 0;
				prev = 2;
			}
			else if (prev == 2)
				std::swap(yMax, yMin);
			if (next == 0 || rate < yMax - yMin)
			{
				rate = yMax - yMin;
				cell = c * 10;
				cell += cell < 0 ? -2 : 2;
				rod = w;
				i = j;
				next = 2;
			}
		}
		else if (simplify)
		{
			if (yMin >= w - minDist && yMin <= w + minDist && yMax > w + minDist)
			{
				if (w >= tgl[j].y - minDist && w <= tgl[j].y + minDist)
				{
					c *= 10;
					c += c < 0 ? -2 : 2;
					tgl[iMinY + j != 2 ? std::min(iMinY, j) : 2].g = c;
				}
			}
			else if (yMax >= w - minDist && yMax <= w + minDist && yMin < w - minDist)
			{
				if (w >= tgl[j].y - minDist && w <= tgl[j].y + minDist)
				{
					c *= 10;
					c += c < 0 ? -2 : 2;
					tgl[j + iMaxY != 2 ? std::min(j, iMaxY) : 2].g = c;
				}
			}
		}
		if (diagonals)
		{
			// median diagonal line to cut (y=x):
			c = static_cast<int32_t>(round(((wMin + wMax) * 0.5f - gridPosW) / gridStepWU));
			w = c * gridStepWU + gridPosW;
			j = 3 - iMinW - iMaxW;
			if (w >= wMin + minDist && w <= wMax - minDist)
			{
				u = (tgl[j].x - tgl[j].y) * RSQRT2;
				if (prev > 2 && w >= u - minDist && w <= u + minDist)
				{
					next = 0;
					prev = 2;
				}
				else if (prev == 2)
					std::swap(wMax, wMin);
				if (next == 0 || rate < wMax - wMin)
				{
					rate = wMax - wMin;
					cell = c * 10;
					cell += cell < 0 ? -3 : 3;
					rod = w;
					i = j;
					next = 3;
				}
			}
			else if (simplify)
			{
				if (wMin >= w - minDist && wMin <= w + minDist && wMax > w + minDist)
				{
					u = (tgl[j].x - tgl[j].y) * RSQRT2;
					if (w >= u - minDist && w <= u + minDist)
					{
						c *= 10;
						c += c < 0 ? -3 : 3;
						tgl[iMinW + j != 2 ? std::min(iMinW, j) : 2].g = c;
					}
				}
				else if (wMax >= w - minDist && wMax <= w + minDist && wMin < w - minDist)
				{
					u = (tgl[j].x - tgl[j].y) * RSQRT2;
					if (w >= u - minDist && w <= u + minDist)
					{
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
			if (w >= uMin + minDist && w <= uMax - minDist)
			{
				u = (tgl[j].x + tgl[j].y) * RSQRT2;
				if (prev > 2 && w >= u - minDist && w <= u + minDist)
				{
					next = 0;
					prev = 2;
				}
				else if (prev == 2)
					std::swap(uMax, uMin);
				if (next == 0 || rate < uMax - uMin)
				{
					cell = c * 10;
					cell += cell < 0 ? -4 : 4;
					rod = w;
					i = j;
					next = 4;
				}
			}
			else if (simplify)
			{
				if (uMin >= w - minDist && uMin <= w + minDist && uMax > w + minDist)
				{
					u = (tgl[j].x + tgl[j].y) * RSQRT2;
					if (w >= u - minDist && w <= u + minDist)
					{
						c *= 10;
						c += c < 0 ? -4 : 4;
						tgl[iMinU + j != 2 ? std::min(iMinU, j) : 2].g = c;
					}
				}
				else if (uMax >= w - minDist && uMax <= w + minDist && uMin < w - minDist)
				{
					u = (tgl[j].x + tgl[j].y) * RSQRT2;
					if (w >= u - minDist && w <= u + minDist)
					{
						c *= 10;
						c += c < 0 ? -4 : 4;
						tgl[j + iMaxU != 2 ? std::min(j, iMaxU) : 2].g = c;
					}
				}
			}
		}
		switch (next)
		{
			case 1:
				prev = iMinX + iMaxX != 2 ? std::min(iMinX, iMaxX) : 2;
				next = iMinX + iMaxX != 2 ? std::max(iMinX, iMaxX) : 0;
				rate = (rod - xMin) / (xMax - xMin);
				point1 = {rod, tgl[iMinX].y + (tgl[iMaxX].y - tgl[iMinX].y) * rate,
					tgl[iMinX].z + (tgl[iMaxX].z - tgl[iMinX].z) * rate, 0,
                    middleColor(tgl[iMinX].color, tgl[iMaxX].color, rate)};
				intoTwo = false;
				if (rod < tgl[i].x - minDist)
				{
					rate = (rod - xMin) / (tgl[i].x - xMin);
					point2 = {rod, tgl[iMinX].y + (tgl[i].y - tgl[iMinX].y) * rate,
						tgl[iMinX].z + (tgl[i].z - tgl[iMinX].z) * rate, 0,
                        middleColor(tgl[iMinX].color, tgl[i].color, rate)};
				}
				else if (rod > tgl[i].x + minDist)
				{
					rate = (rod - tgl[i].x) / (xMax - tgl[i].x);
					point2 = {rod, tgl[i].y + (tgl[iMaxX].y - tgl[i].y) * rate,
						tgl[i].z + (tgl[iMaxX].z - tgl[i].z) * rate, 0,
                        middleColor(tgl[i].color, tgl[iMaxX].color, rate)};
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
				point1 = {tgl[iMinY].x + (tgl[iMaxY].x - tgl[iMinY].x) * rate, rod,
					tgl[iMinY].z + (tgl[iMaxY].z - tgl[iMinY].z) * rate, 0,
                    middleColor(tgl[iMinY].color, tgl[iMaxY].color, rate)};
				intoTwo = false;
				if (rod < tgl[i].y - minDist)
				{
					rate = (rod - yMin) / (tgl[i].y - yMin);
					point2 = {tgl[iMinY].x + (tgl[i].x - tgl[iMinY].x) * rate, rod,
						tgl[iMinY].z + (tgl[i].z - tgl[iMinY].z) * rate, 0,
                        middleColor(tgl[iMinY].color, tgl[i].color, rate)};
				}
				else if (rod > tgl[i].y + minDist)
				{
					rate = (rod - tgl[i].y) / (yMax - tgl[i].y);
					point2 = {tgl[i].x + (tgl[iMaxY].x - tgl[i].x) * rate, rod,
						tgl[i].z + (tgl[iMaxY].z - tgl[i].z) * rate, 0,
                        middleColor(tgl[i].color, tgl[iMaxY].color, rate)};
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
				point1 = {tgl[iMinW].x + (tgl[iMaxW].x - tgl[iMinW].x) * rate,
					tgl[iMinW].y + (tgl[iMaxW].y - tgl[iMinW].y) * rate,
					tgl[iMinW].z + (tgl[iMaxW].z - tgl[iMinW].z) * rate, 0,
                    middleColor(tgl[iMinW].color, tgl[iMaxW].color, rate)};
				w = (tgl[i].x - tgl[i].y) * RSQRT2;
				intoTwo = false;
				if (rod < w - minDist)
				{
					rate = (rod - wMin) / (w - wMin);
					point2 = {tgl[iMinW].x + (tgl[i].x - tgl[iMinW].x) * rate,
						tgl[iMinW].y + (tgl[i].y - tgl[iMinW].y) * rate,
						tgl[iMinW].z + (tgl[i].z - tgl[iMinW].z) * rate, 0,
                        middleColor(tgl[iMinW].color, tgl[i].color, rate)};
				}
				else if (rod > w + minDist)
				{
					rate = (rod - w) / (wMax - w);
					point2 = {tgl[i].x + (tgl[iMaxW].x - tgl[i].x) * rate,
						tgl[i].y + (tgl[iMaxW].y - tgl[i].y) * rate,
						tgl[i].z + (tgl[iMaxW].z - tgl[i].z) * rate, 0,
                        middleColor(tgl[i].color, tgl[iMaxW].color, rate)};
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
				point1 = {tgl[iMinU].x + (tgl[iMaxU].x - tgl[iMinU].x) * rate,
					tgl[iMinU].y + (tgl[iMaxU].y - tgl[iMinU].y) * rate,
					tgl[iMinU].z + (tgl[iMaxU].z - tgl[iMinU].z) * rate, 0,
                    middleColor(tgl[iMinU].color, tgl[iMaxU].color, rate)};
				u = (tgl[i].x + tgl[i].y) * RSQRT2;
				intoTwo = false;
				if (rod < u - minDist)
				{
					rate = (rod - uMin) / (u - uMin);
					point2 = {tgl[iMinU].x + (tgl[i].x - tgl[iMinU].x) * rate,
						tgl[iMinU].y + (tgl[i].y - tgl[iMinU].y) * rate,
						tgl[iMinU].z + (tgl[i].z - tgl[iMinU].z) * rate, 0,
                        middleColor(tgl[iMinU].color, tgl[i].color, rate)};
				}
				else if (rod > u + minDist)
				{
					rate = (rod - u) / (uMax - u);
					point2 = {tgl[i].x + (tgl[iMaxU].x - tgl[i].x) * rate,
						tgl[i].y + (tgl[iMaxU].y - tgl[i].y) * rate,
						tgl[i].z + (tgl[iMaxU].z - tgl[i].z) * rate, 0,
                        middleColor(tgl[i].color, tgl[iMaxU].color, rate)};
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
		// put the produced triangle (to the map of fragments or to results)
		intoTwo = !simplify || (tgl[0].g == 0 && tgl[1].g == 0 && tgl[2].g == 0) ||
				  (tgl[0].g != 0 && tgl[1].g != 0 && tgl[2].g != 0);
		if (!intoTwo)
		{
			for (i = 0; i < 3; i++)
				if (tgl[i].g != 0 && tgl[(i + 2) % 3].g == 0) break;
			triangle.A = tgl[i];
			i = (i + 1) % 3;
			triangle.B = tgl[i];
			i = (i + 1) % 3;
			triangle.C = tgl[i];
			makeSignature(signature, triangle.A.g, triangle.A, triangle.C);
			intoTwo = !fragments.insert({signature, triangle}).second;
		}
		if (intoTwo) putTriangle(meshes, tgl[0], tgl[1], tgl[2], tileSize, tilePosN);
	}
	// Merge fragments and store triangles
	if (simplify)
	{
		auto found = fragments.begin();
		FragSignature oldFrag;
		do
		{
			i = 0;
			j = 0;
			auto frag = fragments.begin();
			if (frag != fragments.end())
			{
				do
				{
					do
					{
						intoTwo = true;
						if (frag->second.B.g == 0)
						{
							makeSignature(signature, frag->second.A.g, frag->second.B, frag->second.C);
							if ((found = fragments.find(signature)) != frag && found != fragments.end() &&
								straightRod(frag->second.A, frag->second.B, found->second.B, maxBreakTangent))
							{
								frag->second.B = found->second.B;
								fragments.erase(found);
								intoTwo = false;
							}
						}
						else if (frag->second.C.g == 0)
						{
							makeSignature(signature, frag->second.B.g, frag->second.C, frag->second.A);
							if ((found = fragments.find(signature)) != frag && found != fragments.end() &&
								straightRod(frag->second.B, frag->second.C, found->second.B, maxBreakTangent))
							{
								frag->second.C = found->second.B;
								triangle = frag->second;
								fragments.erase(found);
								intoTwo = false;
								j = 1;
								if (triangle.C.g == 0) j++;
								oldFrag = frag->first;
							}
						}
					} while (!intoTwo);
					frag++;
					if (j > 0)
					{
						fragments.erase(oldFrag);
						if (j > 1)
						{
							makeSignature(signature, triangle.A.g, triangle.A, triangle.C);
							if (fragments.insert({signature, triangle}).second)
								i = 1;
							else
								j = 1;
						}
						if (j == 1) putTriangle(meshes, triangle.A, triangle.B, triangle.C, tileSize, tilePosN);
						j = 0;
					}
				} while (frag != fragments.end());
			}
		} while (i > 0);
		for (const auto& frag : fragments)
			putTriangle(meshes, frag.second.A, frag.second.B, frag.second.C, tileSize, tilePosN);
	}
	vertices.clear();
	partSizes->clear();
	for (const auto& mesh : meshes)
	{
		vertices.insert(vertices.end(), mesh.second.begin(), mesh.second.end());
		partSizes->push_back({mesh.first, mesh.second.size()});
	}
	return true;
}

// Create vertical plane for path, cutting it by tiles and grid cells
bool OsmAnd::GeometryModifiers::getTesselatedPlane(std::vector<VectorMapSymbol::Vertex>& vertices,
		const std::vector<OsmAnd::PointD>& points,
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
        const bool solidColors)
{
	if (minDistance <= 0.0f)
        return false;
	auto pointsCount = points.size();
	if (pointsCount < 2)
        return false;
	bool withTrace = topColor.a > 0.0f || bottomColor.a > 0.0f;
    if (roofHeight <= 0.0f && !withTrace)
        return false;
	float minDist = minDistance;
    auto tileSize = Utilities::getPowZoom(MaxZoomLevel - zoomLevel);
	float gridStepXY = tileSize / static_cast<double>(cellsPerTileSize);
	float gridPosX = -(tilePosN.x - std::floor(tilePosN.x)) * tileSize;
	float gridPosY = -(tilePosN.y - std::floor(tilePosN.y)) * tileSize;
	int32_t i, j;
	std::deque<VertexForPath> inObj;
    int shift = solidColors ? 0 : 1;
    for (i = 0; i < pointsCount - 1; i++)
    {
        auto &point1 = points[i];
        auto &point2 = points[i + 1];
        auto &height1 = heights[i];
        auto &height2 = heights[i + 1];
        auto &roofColor1 = i < colorizationMapping.size() ? colorizationMapping[i] : fillColor;
        auto &roofColor2 = i < colorizationMapping.size() - shift ? colorizationMapping[i + shift] : fillColor;
        auto &topColor1 = i < traceColorizationMapping.size() ? traceColorizationMapping[i] : topColor;
        auto &topColor2 = i < traceColorizationMapping.size() - shift ? traceColorizationMapping[i + shift] : topColor;
        inObj.push_back({static_cast<float>(point1.x), static_cast<float>(point1.y), height1, roofColor1, topColor1});
        inObj.push_back({static_cast<float>(point2.x), static_cast<float>(point2.y), height2, roofColor2, topColor2});
    }
	VertexForPath seg[2], point1;
	float RSQRT2 = 0.70710678118654752440084436210485L;
	float gridPosW = (gridPosX - gridPosY) * RSQRT2;
	float gridPosU = (gridPosX + gridPosY) * RSQRT2;
	float gridStepWU = gridStepXY * RSQRT2;
	float w, u, rod, rate, xMin, xMax, yMin, yMax, wMin, wMax, uMin, uMax;
	int32_t prev, next, c, iMinX, iMaxX, iMinY, iMaxY, iMinW, iMaxW, iMinU, iMaxU;
	bool withTraceColorMap = !traceColorizationMapping.isEmpty();
	float noHeight = VectorMapSymbol::_absentElevation;
	while (inObj.size() > 0)
	{
		xMin = std::numeric_limits<float>::max();
		xMax = -std::numeric_limits<float>::max();
		yMin = xMin;
		yMax = xMax;
		wMin = xMin;
		wMax = xMax;
		uMin = xMin;
		uMax = xMax;
		// get a triangle and find its boundaries:
		for (i = 0; i < 2; i++)
		{
			if (inObj.empty()) return false;
			seg[i] = inObj.front();
			xMin = fmin(seg[i].x, xMin);
			if (xMin == seg[i].x) iMinX = i;
			xMax = fmax(seg[i].x, xMax);
			if (xMax == seg[i].x) iMaxX = i;
			yMin = fmin(seg[i].y, yMin);
			if (yMin == seg[i].y) iMinY = i;
			yMax = fmax(seg[i].y, yMax);
			if (yMax == seg[i].y) iMaxY = i;
			if (diagonals)
			{
				w = (seg[i].x - seg[i].y) * RSQRT2;
				wMin = fmin(w, wMin);
				if (wMin == w) iMinW = i;
				wMax = fmax(w, wMax);
				if (wMax == w) iMaxW = i;
				u = (seg[i].x + seg[i].y) * RSQRT2;
				uMin = fmin(u, uMin);
				if (uMin == u) iMinU = i;
				uMax = fmax(u, uMax);
				if (uMax == u) iMaxU = i;
			}
			inObj.pop_front();
		}
		// increase minimal distance in accordance to precission of float:
		minDist = fmax(fmax(fmax(fabs(xMin), fabs(xMax)), fmax(fabs(yMin), fabs(yMax))) / 1000000.0f, minDist);
		next = 0;
		rate = xMax - xMin;
		// median vertical line to cut:
		c = static_cast<int32_t>(round(((xMin + xMax) * 0.5f - gridPosX) / gridStepXY));
		rod = c * gridStepXY + gridPosX;
		if (rod >= xMin + minDist && rod <= xMax - minDist)
			next = 1;
		// median horizontal line to cut:
		c = static_cast<int32_t>(round(((yMin + yMax) * 0.5f - gridPosY) / gridStepXY));
		w = c * gridStepXY + gridPosY;
		if (w >= yMin + minDist && w <= yMax - minDist && (next == 0 || rate < yMax - yMin))
		{
			rate = yMax - yMin;
			rod = w;
			next = 2;
		}
		if (diagonals)
		{
			// median diagonal line to cut (y=x):
			c = static_cast<int32_t>(round(((wMin + wMax) * 0.5f - gridPosW) / gridStepWU));
			w = c * gridStepWU + gridPosW;
			if (w >= wMin + minDist && w <= wMax - minDist)
			{
				if (next == 0 || rate < wMax - wMin)
				{
					rate = wMax - wMin;
					rod = w;
					next = 3;
				}
			}
			// median diagonal line to cut (y=-x):
			c = static_cast<int32_t>(round(((uMin + uMax) * 0.5f - gridPosU) / gridStepWU));
			w = c * gridStepWU + gridPosU;
			if (w >= uMin + minDist && w <= uMax - minDist)
			{
				if (next == 0 || rate < uMax - uMin)
				{
					rod = w;
					next = 4;
				}
			}
		}
		switch (next)
		{
			case 1:
				prev = std::min(iMinX, iMaxX);
				next = std::max(iMinX, iMaxX);
				rate = (rod - xMin) / (xMax - xMin);
				point1 = {rod, seg[iMinX].y + (seg[iMaxX].y - seg[iMinX].y) * rate,
					seg[iMinX].z + (seg[iMaxX].z - seg[iMinX].z) * rate,
                    middleColor(seg[iMinX].color, seg[iMaxX].color, rate),
                    middleColor(seg[iMinX].traceColor, seg[iMaxX].traceColor, rate)};
				break;
			case 2:
				prev = std::min(iMinY, iMaxY);
				next = std::max(iMinY, iMaxY);
				rate = (rod - yMin) / (yMax - yMin);
				point1 = {seg[iMinY].x + (seg[iMaxY].x - seg[iMinY].x) * rate, rod,
					seg[iMinY].z + (seg[iMaxY].z - seg[iMinY].z) * rate,
                    middleColor(seg[iMinY].color, seg[iMaxY].color, rate),
                    middleColor(seg[iMinY].traceColor, seg[iMaxY].traceColor, rate)};
				break;
			case 3:
				prev = std::min(iMinW, iMaxW);
				next = std::max(iMinW, iMaxW);
				rate = (rod - wMin) / (wMax - wMin);
				point1 = {seg[iMinW].x + (seg[iMaxW].x - seg[iMinW].x) * rate,
					seg[iMinW].y + (seg[iMaxW].y - seg[iMinW].y) * rate,
					seg[iMinW].z + (seg[iMaxW].z - seg[iMinW].z) * rate,
                    middleColor(seg[iMinW].color, seg[iMaxW].color, rate),
                    middleColor(seg[iMinW].traceColor, seg[iMaxW].traceColor, rate)};
				break;
			case 4:
				prev = std::min(iMinU, iMaxU);
				next = std::max(iMinU, iMaxU);
				rate = (rod - uMin) / (uMax - uMin);
				point1 = {seg[iMinU].x + (seg[iMaxU].x - seg[iMinU].x) * rate,
					seg[iMinU].y + (seg[iMaxU].y - seg[iMinU].y) * rate,
					seg[iMinU].z + (seg[iMaxU].z - seg[iMinU].z) * rate,
                    middleColor(seg[iMinU].color, seg[iMaxU].color, rate),
                    middleColor(seg[iMinU].traceColor, seg[iMaxU].traceColor, rate)};
				break;
			default:
                // generate result triangles
                auto topHeight0 = seg[0].z;
                auto topHeight1 = seg[1].z;
                if (roofHeight > 0.0f)
                {
                    auto tileIdY = floor(static_cast<double>(seg[0].y + seg[1].y) / 2.0 / tileSize + tilePosN.y);
                    auto offset0 = static_cast<double>(seg[0].y) / tileSize + tilePosN.y - tileIdY;
                    auto offset1 = static_cast<double>(seg[1].y) / tileSize + tilePosN.y - tileIdY;
                    auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(zoomLevel, tileIdY, tileSize);
                    auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(zoomLevel, tileIdY + 1.0, tileSize);
                    auto metersPerUnit0 = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offset0);
                    auto metersPerUnit1 = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offset1);
                    topHeight0 -= roofHeight * static_cast<float>(metersPerUnit0);
                    topHeight1 -= roofHeight * static_cast<float>(metersPerUnit1);
                    vertices.push_back({{seg[0].x, seg[0].z, seg[0].y}, seg[0].color});
                    vertices.push_back({{seg[1].x, seg[1].z, seg[1].y}, seg[1].color});
                    vertices.push_back({{seg[0].x, topHeight0, seg[0].y}, seg[0].color});
                    vertices.push_back({{seg[1].x, topHeight1, seg[1].y}, seg[1].color});
                    vertices.push_back({{seg[0].x, topHeight0, seg[0].y}, seg[0].color});
                    vertices.push_back({{seg[1].x, seg[1].z, seg[1].y}, seg[1].color});
                }
                if (withTrace)
                {
                    auto topColor0 = seg[0].traceColor;
                    auto topColor1 = seg[1].traceColor;
                    auto bottomColor0 = bottomColor;
                    auto bottomColor1 = bottomColor;
                    if (withTraceColorMap)
                    {
                        bottomColor0 *= topColor0;
                        bottomColor1 *= topColor1;
                        topColor0 *= topColor;
                        topColor1 *= topColor;
                    }
                    vertices.push_back({{seg[0].x, topHeight0, seg[0].y}, topColor0});
                    vertices.push_back({{seg[1].x, topHeight1, seg[1].y}, topColor1});
                    vertices.push_back({{seg[0].x, noHeight, seg[0].y}, bottomColor0});
                    vertices.push_back({{seg[1].x, noHeight, seg[1].y}, bottomColor1});
                    vertices.push_back({{seg[0].x, noHeight, seg[0].y}, bottomColor0});
                    vertices.push_back({{seg[1].x, topHeight1, seg[1].y}, topColor1});
                }
                continue;
		}
        inObj.push_back(seg[prev]);
        inObj.push_back(point1);
        inObj.push_back(point1);
        inObj.push_back(seg[next]);
}
	return true;
}