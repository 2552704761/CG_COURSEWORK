#pragma once
#include <cfloat>
#include "Maths.h"

struct LineVertex
{
	Vec3 pos;
	Vec3 color;
};

struct AABB
{
	Vec3 min;
	Vec3 max;

	bool intersects(const AABB& other) const
	{
		if (max.x < other.min.x || min.x > other.max.x) return false;
		if (max.y < other.min.y || min.y > other.max.y) return false;
		if (max.z < other.min.z || min.z > other.max.z) return false;
		return true;
	}
	AABB transformed(const Matrix& m) const;
};
void getAABBCorners(const AABB& aabb, Vec3 out[8])
{
	Vec3 min = aabb.min;
	Vec3 max = aabb.max;

	out[0] = Vec3(min.x, min.y, min.z);
	out[1] = Vec3(max.x, min.y, min.z);
	out[2] = Vec3(max.x, min.y, max.z);
	out[3] = Vec3(min.x, min.y, max.z);

	out[4] = Vec3(min.x, max.y, min.z);
	out[5] = Vec3(max.x, max.y, min.z);
	out[6] = Vec3(max.x, max.y, max.z);
	out[7] = Vec3(min.x, max.y, max.z);
}
static int edges[24] =
{
	0,1, 1,2, 2,3, 3,0,   // bottom
	4,5, 5,6, 6,7, 7,4,   // top
	0,4, 1,5, 2,6, 3,7    // vertical
};

void buildAABBLineVertices(const AABB& aabb,
	std::vector<LineVertex>& out,
	Vec3 color)
{
	Vec3 corners[8];
	getAABBCorners(aabb, corners);

	for (int i = 0; i < 24; i += 2)
	{
		out.push_back({ corners[edges[i]],   color });
		out.push_back({ corners[edges[i + 1]], color });
	}
}
inline AABB AABB::transformed(const Matrix& m) const
{
	Vec3 corners[8];
	getAABBCorners(*this, corners);

	AABB out;
	out.min = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	out.max = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int i = 0; i < 8; ++i)
	{
		Vec3 p = m.mulPoint(corners[i]);
		out.min.x = std::min(out.min.x, p.x);
		out.min.y = std::min(out.min.y, p.y);
		out.min.z = std::min(out.min.z, p.z);

		out.max.x = std::max(out.max.x, p.x);
		out.max.y = std::max(out.max.y, p.y);
		out.max.z = std::max(out.max.z, p.z);
	}
	return out;
}