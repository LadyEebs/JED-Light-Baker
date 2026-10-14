#pragma once

#include "IJed.h"
#include "float3.h"

namespace Jed
{
	float3     ReadVector(const tjedvector& value);
	tjedvector WriteVector(const float3& value);

	bool   QuerySurfaceVisible(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex);
	float3 QuerySurfaceNormal(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex);
	float3 QuerySurfaceVertex(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex, int nVertexIndex);

	bool IsSectorValid(IJEDLevel* pJedLevel, int nSectorIndex);
}
