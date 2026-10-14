#include "pch.h"
#include "Jed.h"
#include "Enums.h"

namespace Jed
{
	float3 ReadVector(const tjedvector& value)
	{
		return {(float)value.v.s1.x, (float)value.v.s1.y, (float)value.v.s1.z};
	}

	tjedvector WriteVector(const float3& value)
	{
		tjedvector result{};
		result.v.s1.x = (double)value.x;
		result.v.s1.y = (double)value.y;
		result.v.s1.z = (double)value.z;
		return result;
	}

	bool QuerySurfaceVisible(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex)
	{
		tjedsurfacerec surface;
		memset(&surface, 0, sizeof(tjedsurfacerec));
		pJedLevel->GetSurface(nSectorIndex, nSurfaceIndex, &surface, sf_Material | sf_adjoin | sf_adjoinflags | sf_geo | sf_SurfFlags | sf_FaceFlags);

		// ignore geo mode 0 (not visible) and surfaces with no material
		if (surface.geo == 0 || !surface.material)
			return false;

		// ignore adjoins that block
		if (surface.adjoinsc >= 0 && !(surface.adjoinflags & EAdjoin_Visible))
			return false;

		// ignore skies
		if ((surface.surfflags & ESky_Horizon) != 0 || (surface.surfflags & ESky_Ceiling) != 0)
			return false;

		return true;
	}

	float3 QuerySurfaceNormal(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex)
	{
		tjedvector dnormal;
		if (FAILED(pJedLevel->GetSurfaceNormal(nSectorIndex, nSurfaceIndex, &dnormal)))
			return {0.0f, 0.0f, 0.0f};
		return ReadVector(dnormal);
	}

	float3 QuerySurfaceVertex(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex, int nVertexIndex)
	{
		// Get sector vertex index from surface vertex index
		int nVertex = pJedLevel->SurfaceGetVertexNum(nSectorIndex, nSurfaceIndex, nVertexIndex);

		tjedvector vertex;
		if (FAILED(pJedLevel->SectorGetVertex(nSectorIndex, nVertex, &vertex.v.s1.x, &vertex.v.s1.y, &vertex.v.s1.z)))
			return {0.0f, 0.0f, 0.0f};

		return ReadVector(vertex);
	}

	bool IsSectorValid(IJEDLevel* pJedLevel, int nSectorIndex)
	{
		return pJedLevel && nSectorIndex >= 0 && nSectorIndex < pJedLevel->NSectors();
	}
}