#pragma once

#include "IJed.h"
#include "float3.h"
#include "Geometry.h"

namespace Jed
{
	template <typename... Args>
	void PrintMessage(IJED* pJed, uint32_t nType, const char* sFmt, Args&&... args)
	{
		char msg[256];
		sprintf_s(msg, 256, sFmt, std::forward<Args>(args)...);
		pJed->PanMessage(nType, msg);
	}

	float3     ReadVector(const tjedvector& value);
	tjedvector WriteVector(const float3& value);

	bool   QuerySurfaceVisible(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex);
	std::optional<float3> QuerySurfaceNormal(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex);
	std::optional<float3> QuerySurfaceVertex(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex, int nVertexIndex);

	std::optional<SPolygon> GetSurfacePolygon(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex);

	// the Jed API doesn't expose anything for working with the global surface numbers, but cog values return the global number..
	std::optional<std::pair<int, int>> GetSectorSurfaceFromGlobalSurface(IJEDLevel* pJedLevel, int nGlobalSurfaceIndex);

	bool IsSectorValid(IJEDLevel* pJedLevel, int nSectorIndex);

	bool CommonEdgeLiesOnLine(IJED* pJed, int nSectorA, int nSurfaceA, int nSectorB, int nSurfaceB,
							  const float3& lineStart, const float3& lineEnd, bool bProjectLineOntoCommonEdge);

	bool HasCleaveRelevantSharedEdge(IJED* pJed, int nSector, int nAdjoinSurface, int nAdjacentSector, int nAdjacentSurface,
									 const float3& edgeStart, const float3& edgeEnd);

	std::optional<SLightClipVolume> BuildChildClipVolume(IJED* pJed, const SLightClipVolume& parent, int nSector, int nAdjoinSurface, int nAdjacentSector, int nAdjacentSurface,
														 const float3& source, bool bDirectionalSource, const SPolygon& clippedPoly, float lightSize);

	std::optional<SLightClipVolume> BuildSunClipVolume(const float3& directionToSun, const SPolygon& skyPolygon);

	void PropagateGeoAfterCleave(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex, int nNewSurfaceIndex, tjedsurfacerec& original);
}
