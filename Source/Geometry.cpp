#include "pch.h"
#include "Geometry.h"

#include <cmath>
#include <limits>
#include <numbers>

namespace
{
	constexpr float kSplitRayDisplacementScale = 1.0f / 0.75f;
	constexpr float kSplitPlaneAnchorScale = 0.075f;
}

void SPolygon::RemoveDuplicateVertices()
{
	if (vertices.empty())
		return;

	const float epsilonSquared = 1e-5f * 1e-5f;

	std::vector<float3> unique;
	unique.reserve(vertices.size());
	for (const float3& vertex : vertices)
	{
		if (unique.empty() || lengthSq(vertex - unique.back()) > epsilonSquared)
			unique.push_back(vertex);
	}

	if (unique.size() > 1 && lengthSq(unique.front() - unique.back()) <= epsilonSquared)
	{
		unique.pop_back();
	}
	vertices.swap(unique);
}

bool SPolygon::PointInside(const float3& point) const
{
	const size_t nNumVertices = vertices.size();
	if (nNumVertices < 3)
		return false;

	const float3 normal = normalize(cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
	for (size_t nVertexIndex = 0; nVertexIndex < nNumVertices; ++nVertexIndex)
	{
		const float3& a = vertices[nVertexIndex];
		const float3& b = vertices[(nVertexIndex + 1) % nNumVertices];

		if (dot(cross(normal, b - a), point - a) < -1e-4f)
			return false;
	}

	return true;
}

SPolygon SPolygon::ClipAgainstPlanes(const std::vector<SPlane>& planes, float epsilon) const
{
	SPolygon polygon = *this;
	SPolygon output;

	for (const SPlane& plane : planes)
	{
		if (polygon.vertices.empty())
			break;

		output.vertices.clear();
		output.vertices.reserve(polygon.vertices.size() + 2);

		float3 previous = polygon.vertices.back();
		float previousDistance = distance(plane, previous);
		bool previousInside = previousDistance >= -epsilon;

		for (const float3& current : polygon.vertices)
		{
			const float currentDistance = distance(plane, current);
			const bool currentInside = currentDistance >= -epsilon;

			if (currentInside != previousInside)
			{
				const float denominator = previousDistance - currentDistance;
				if (std::fabs(denominator) > std::numeric_limits<float>::epsilon())
				{
					float t = previousDistance / denominator;
					t = std::max(0.0f, std::min(1.0f, t));
					output.vertices.push_back(previous + (current - previous) * t);
				}
			}

			if (currentInside)
				output.vertices.push_back(current);

			previous = current;
			previousDistance = currentDistance;
			previousInside = currentInside;
		}

		output.RemoveDuplicateVertices();
		std::swap(polygon, output);
	}

	return polygon;
}

bool SPolygon::CrossesPlane(const SPlane& plane) const
{
	if (vertices.empty())
		return false;

	float minimum = std::numeric_limits<float>::max();
	float maximum = std::numeric_limits<float>::lowest();
	for (const float3& vertex : vertices)
	{
		const float dist = distance(plane, vertex);
		minimum = std::min(minimum, dist);
		maximum = std::max(maximum, dist);
	}
	return minimum < -1e-5f && maximum > 1e-5f;
}

float3 SPolygon::Centroid() const
{
	float3 center{0, 0, 0};
	if (vertices.empty())
		return center;

	for (const float3& p : vertices)
		center += p;

	center = center / (float)(vertices.size());
	return center;
}

float SPolygon::Area() const
{
	if (vertices.size() < 3)
		return 0.0f;

	float area = 0.0f;
	const float3& origin = vertices[0];
	for (size_t i = 1; i < vertices.size() - 1; ++i)
	{
		const float3& a = vertices[i] - origin;
		const float3& b = vertices[i + 1] - origin;
		area += length(cross(a, b)) * 0.5f;
	}
	return area;
}

namespace Geo
{
	bool PointLiesOnLine(const float3& point, const float3& lineStart, const float3& lineEnd)
	{
		const float3 direction = lineEnd - lineStart;
		const float lengthSquared = lengthSq(direction);
		if (lengthSquared <= 1e-5f * 1e-5f)
			return false;

		const float t = dot(point - lineStart, direction) / lengthSquared;
		const float3 projected = lineStart + direction * t;
		return length(projected - point) < 1e-5f;
	}

	bool BuildSplitPlanes(const float3& source, bool bDirectionalSource, const float3& edgeStart, const float3& edgeEnd,
						  const float3& adjoinInterior, bool bCanCleave, SPlane& splitA, SPlane& splitB, SPlane& clipPlane, float lightSize)
	{
		const float3 edge = edgeEnd - edgeStart;

		// sun rays are parallel, so the edge plane is defined by the edge and the sun direction
		const float3 sourceToEdge = bDirectionalSource ? source : source - edgeStart;
		float3 clipNormal = normalize(cross(sourceToEdge, edge));

		// ensure the clip normal points into the adjoin interior
		const float3 refPoint = bDirectionalSource ? edgeStart : source;
		if (dot(clipNormal, adjoinInterior - refPoint) < 0.0f)
			clipNormal = clipNormal * -1.0f;

		const float rayDisplacement = lightSize * kSplitRayDisplacementScale;
		const float anchorDisplacement = lightSize * kSplitPlaneAnchorScale;

		// offset of first plane (one side of the penumbra)
		const float3 firstRay = sourceToEdge + clipNormal * rayDisplacement;
		const float3 firstNormal = normalize(cross(firstRay, edge));
		const float3 firstPoint = edgeStart - firstNormal * anchorDisplacement;

		// offset of the second plane (the other side of the penumbra)
		const float3 secondRay = sourceToEdge - clipNormal * rayDisplacement;
		const float3 secondNormal = normalize(cross(secondRay, edge));
		const float3 secondPoint = edgeStart + secondNormal * anchorDisplacement;

		splitA = {firstNormal, firstPoint, bCanCleave};
		splitB = {secondNormal, secondPoint, bCanCleave};
		clipPlane = {clipNormal, bDirectionalSource ? edgeStart : source, bCanCleave};
		return true;
	}
}