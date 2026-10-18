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
		if ((surface.surfflags & ESurface_Horizon) != 0 || (surface.surfflags & ESurface_Ceiling) != 0)
			return false;

		return true;
	}

	std::optional<float3> QuerySurfaceNormal(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex)
	{
		tjedvector dnormal;
		if (FAILED(pJedLevel->GetSurfaceNormal(nSectorIndex, nSurfaceIndex, &dnormal)))
			return std::nullopt;
		return ReadVector(dnormal);
	}

	std::optional<float3> QuerySurfaceVertex(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex, int nVertexIndex)
	{
		// Get sector vertex index from surface vertex index
		int nVertex = pJedLevel->SurfaceGetVertexNum(nSectorIndex, nSurfaceIndex, nVertexIndex);

		tjedvector vertex;
		if (FAILED(pJedLevel->SectorGetVertex(nSectorIndex, nVertex, &vertex.v.s1.x, &vertex.v.s1.y, &vertex.v.s1.z)))
			return std::nullopt;

		return ReadVector(vertex);
	}

	bool IsSectorValid(IJEDLevel* pJedLevel, int nSectorIndex)
	{
		return pJedLevel && nSectorIndex >= 0 && nSectorIndex < pJedLevel->NSectors();
	}

	std::optional<SPolygon> GetSurfacePolygon(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex)
	{
		const int nVertexCount = pJedLevel->SurfaceNVertices(nSectorIndex, nSurfaceIndex);
		if (nVertexCount < 3)
			return std::nullopt;

		SPolygon result;
		result.vertices.reserve((size_t)nVertexCount);
		for (int i = 0; i < nVertexCount; ++i)
		{
			std::optional<float3> vertexOpt = QuerySurfaceVertex(pJedLevel, nSectorIndex, nSurfaceIndex, i);
			if (!vertexOpt.has_value())
				continue;
			float3 vertex = vertexOpt.value();
			result.vertices.push_back(vertex);
		}

		result.RemoveDuplicateVertices();
		if (result.vertices.size() < 3)
			return std::nullopt;

		return result;
	}

	std::optional<std::pair<int, int>> GetSectorSurfaceFromGlobalSurface(IJEDLevel* pJedLevel, int nGlobalSurfaceIndex)
	{
		if (nGlobalSurfaceIndex < 0)
			return {};

		int nCursor = 0;
		const int nNumSectors = pJedLevel->NSectors();
		for (int nSectorIndex = 0; nSectorIndex < nNumSectors; ++nSectorIndex)
		{
			int nNumSurfaces = pJedLevel->SectorNSurfaces(nSectorIndex);
			if (nGlobalSurfaceIndex >= nCursor && nGlobalSurfaceIndex < nCursor + nNumSurfaces)
				return std::make_pair(nSectorIndex, nGlobalSurfaceIndex - nCursor);

			nCursor += nNumSurfaces;
		}

		return {};
	}

	bool CommonEdgeLiesOnLine(IJED* pJed, int nSectorA, int nSurfaceA, int nSectorB, int nSurfaceB,
							  const float3& lineStart, const float3& lineEnd, bool bProjectLineOntoCommonEdge)
	{
		int nFirstA = -1;
		int nSecondA = -1;
		int nFirstB = -1;
		int nSecondB = -1;
		if (!pJed->FindCommonEdges(nSectorA, nSurfaceA, nSectorB, nSurfaceB, &nFirstA, &nSecondA, &nFirstB, &nSecondB))
		{
			return false;
		}

		std::optional<float3> endpoint0Opt = QuerySurfaceVertex(pJed->GetLevel(), nSectorB, nSurfaceB, nFirstB);
		std::optional<float3> endpoint1Opt = QuerySurfaceVertex(pJed->GetLevel(), nSectorB, nSurfaceB, nSecondB);
		if (!endpoint0Opt.has_value() || !endpoint1Opt.has_value())
			return false;
		float3 endpoint0 = endpoint0Opt.value();
		float3 endpoint1 = endpoint1Opt.value();
		if (bProjectLineOntoCommonEdge)
			return Geo::PointLiesOnLine(lineStart, endpoint0, endpoint1) && Geo::PointLiesOnLine(lineEnd, endpoint0, endpoint1);

		return Geo::PointLiesOnLine(endpoint0, lineStart, lineEnd) && Geo::PointLiesOnLine(endpoint1, lineStart, lineEnd);
	}

	bool HasCleaveRelevantSharedEdge(IJED* pJed, int nSectorIndex, int nAdjoinSurface, int nAdjacentSector, int nAdjacentSurface,
									 const float3& edgeStart, const float3& edgeEnd)
	{
		int nSurfaceCount = pJed->GetLevel()->SectorNSurfaces(nSectorIndex);
		for (int nOtherSurface = 0; nOtherSurface < nSurfaceCount; ++nOtherSurface)
		{
			if (nOtherSurface == nAdjoinSurface)
				continue;

			tjedsurfacerec record;
			memset(&record, 0, sizeof(tjedsurfacerec));
			if (FAILED(pJed->GetLevel()->GetSurface(nSectorIndex, nOtherSurface, &record, sf_adjoin)))
				continue;

			if (record.adjoinsc != -1)
				continue;

			if (CommonEdgeLiesOnLine(pJed, nSectorIndex, nAdjoinSurface, nSectorIndex, nOtherSurface, edgeStart, edgeEnd, true))
				return true;
		}

		if (nAdjacentSector < 0 || nAdjacentSurface < 0)
			return false;

		nSurfaceCount = pJed->GetLevel()->SectorNSurfaces(nAdjacentSector);
		for (int nOtherSurface = 0; nOtherSurface < nSurfaceCount; ++nOtherSurface)
		{
			if (nOtherSurface == nAdjacentSurface)
				continue;

			tjedsurfacerec record;
			memset(&record, 0, sizeof(tjedsurfacerec));
			if (FAILED(pJed->GetLevel()->GetSurface(nAdjacentSector, nOtherSurface, &record, sf_adjoin)))
				continue;

			if (record.adjoinsc != -1)
				continue;

			if (CommonEdgeLiesOnLine(pJed, nAdjacentSector, nAdjacentSurface, nAdjacentSector, nOtherSurface, edgeStart, edgeEnd, false))
				return true;
		}

		return false;
	}

	std::optional<SLightClipVolume> BuildChildClipVolume(IJED* pJed, const SLightClipVolume& parent, int nSector, int nAdjoinSurface, int nAdjacentSector, int nAdjacentSurface,
														 const float3& source, bool bDirectionalSource, const SPolygon& clippedPoly, float lightSize)
	{
		SLightClipVolume child = {};
		if (clippedPoly.vertices.size() < 3)
			return std::nullopt;

		float3 center = clippedPoly.Centroid();

		child.splitA.reserve(clippedPoly.vertices.size());
		child.splitB.reserve(clippedPoly.vertices.size());
		child.clip.reserve(clippedPoly.vertices.size());

		for (size_t nEdgeIndex = 0; nEdgeIndex < clippedPoly.vertices.size(); ++nEdgeIndex)
		{
			const float3& edgeStart = clippedPoly.vertices[nEdgeIndex];
			const float3& edgeEnd = clippedPoly.vertices[(nEdgeIndex + 1) % clippedPoly.vertices.size()];
			if (length(edgeEnd - edgeStart) < 1e-5f)
				continue;

			// keep the matching planes if this is already on an ancestor plane.
			bool bReusedParentBoundary = false;
			if (parent.IsConsistent())
			{
				for (size_t i = 0; i < parent.clip.size(); ++i)
				{
					if (IsPointOnPlane(parent.clip[i], edgeStart) && IsPointOnPlane(parent.clip[i], edgeEnd))
					{
						child.splitA.push_back(parent.splitA[i]);
						child.splitB.push_back(parent.splitB[i]);
						child.clip.push_back(parent.clip[i]);
						bReusedParentBoundary = true;
						break;
					}
				}
			}

			if (bReusedParentBoundary)
				continue;

			// check if this edge is shared with another surface in the same sector or in the adjacent sector, would prevent cleaving.
			const bool bCanCleave = HasCleaveRelevantSharedEdge(pJed, nSector, nAdjoinSurface, nAdjacentSector, nAdjacentSurface, edgeStart, edgeEnd);

			SPlane splitA, splitB, clipPlane;
			if (!Geo::BuildSplitPlanes(source, bDirectionalSource, edgeStart, edgeEnd, center, bCanCleave, splitA, splitB, clipPlane, lightSize))
			{
				child = {};
				return std::nullopt;
			}

			child.splitA.push_back(splitA);
			child.splitB.push_back(splitB);
			child.clip.push_back(clipPlane);
		}

		if (!child.IsConsistent() || child.clip.empty())
			return std::nullopt;

		return child;
	}

	std::optional<SLightClipVolume> BuildSunClipVolume(const float3& directionToSun, const SPolygon& skyPolygon)
	{
		SLightClipVolume result;
		if (skyPolygon.vertices.size() < 3)
			return std::nullopt;

		float3 center = skyPolygon.Centroid();
		result.splitA.reserve(skyPolygon.vertices.size());
		result.splitB.reserve(skyPolygon.vertices.size());
		result.clip.reserve(skyPolygon.vertices.size());

		for (size_t i = 0; i < skyPolygon.vertices.size(); ++i)
		{
			const float3& edgeStart = skyPolygon.vertices[i];
			const float3& edgeEnd = skyPolygon.vertices[(i + 1) % skyPolygon.vertices.size()];
			if (lengthSq(edgeEnd - edgeStart) < 1e-5f*1e-5f)
				continue;

			SPlane splitA, splitB, clip;
			if (!Geo::BuildSplitPlanes(directionToSun, true, edgeStart, edgeEnd, center, true, splitA, splitB, clip, 0.005f))
			{
				result = {};
				return std::nullopt;
			}
			result.splitA.push_back(splitA);
			result.splitB.push_back(splitB);
			result.clip.push_back(clip);
		}

		if (!result.IsConsistent() || result.clip.empty())
			return std::nullopt;

		return result;
	}

	void PropagateGeoAfterCleave(IJEDLevel* pJedLevel, int nSectorIndex, int nSurfaceIndex, int nNewSurfaceIndex, tjedsurfacerec& original)
	{
		if (nNewSurfaceIndex < 0 || original.adjoinsc == -1)
			return;

		// preserve the geo field on both halves and on both sides of an existing adjoin
		pJedLevel->SetSurface(nSectorIndex, nSurfaceIndex, &original, sf_geo);
		pJedLevel->SetSurface(nSectorIndex, nNewSurfaceIndex, &original, sf_geo);
		pJedLevel->SetSurface((int)original.adjoinsc, (int)original.adjoinsf, &original, sf_geo);

		tjedsurfacerec splitRecord;
		memset(&splitRecord, 0, sizeof(tjedsurfacerec));
		if (SUCCEEDED(pJedLevel->GetSurface(nSectorIndex, nNewSurfaceIndex, &splitRecord, sf_adjoin)) && splitRecord.adjoinsc >= 0 && splitRecord.adjoinsf >= 0)
		{
			pJedLevel->SetSurface((int)splitRecord.adjoinsc, (int)splitRecord.adjoinsf, &original, sf_geo);
		}
	}
}