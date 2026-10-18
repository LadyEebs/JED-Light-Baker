#pragma once

#include "float3.h"

#include <vector>

struct SPlane
{
	float3 normal;
	float3 point;
	bool bCanCleave = false;
};

inline float distance(const SPlane& plane, const float3& position)
{
	return dot(plane.normal, position - plane.point);
}

inline bool IsPointOnPlane(const SPlane& plane, const float3& point)
{
	return std::fabs(distance(plane, point)) < 1e-5f;
}

struct SPolygon
{
	void RemoveDuplicateVertices();
	bool PointInside(const float3& point) const;

	SPolygon ClipAgainstPlanes(const std::vector<SPlane>& planes, float epsilon = 1e-5f) const;
	bool CrossesPlane(const SPlane& plane) const;

	float3 Centroid() const;
	float Area() const;

	bool IsDegenerate(float epsilon = 1e-5f) const;

	std::vector<float3> vertices;
};

// clipping planes defining the light volume and shadow projection across adjoins
struct SLightClipVolume
{
	std::vector<SPlane> splitA; // planes for one side of the light's penumbra/volume
	std::vector<SPlane> splitB; // planes for the opposite side of the volume
	std::vector<SPlane> clip;   // main clipping planes used to cull geometry outside the light cone

	bool IsConsistent() const
	{
		return splitA.size() == splitB.size() && splitA.size() == clip.size();
	}
};

namespace Geo
{
	bool PointLiesOnLine(const float3& point, const float3& lineStart, const float3& lineEnd);

	bool BuildSplitPlanes(const float3& source, bool bDirectionalSource, const float3& edgeStart, const float3& edgeEnd,
						  const float3& adjoinInterior, bool bCanCleave, SPlane& splitA, SPlane& splitB, SPlane& clipPlane, float lightSize = 0.0075f);
}
