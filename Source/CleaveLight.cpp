#include "pch.h"
#include "CleaveLight.h"
#include "Enums.h"
#include "Jed.h"

namespace 
{
	constexpr int kMaxGeometryDepth = 25;
	constexpr float kPointLightsize = 0.0075f;
}

CCleaveLight::CCleaveLight(IJED* pJed, IJEDLevel* pJedLevel, uint32_t nBakeFlags)
	: m_pJed(pJed)
	, m_pJedLevel(pJedLevel)
	, m_nBakeFlags(nBakeFlags)
{
}

bool CCleaveLight::IsLayerBaked(int nLayerIndex) const
{
	if (!(m_nBakeFlags & ELightBake_VisibleLayers))
		return true;

	return m_pJed->IsLayerVisible(nLayerIndex);
}

void CCleaveLight::PrepareGeometry(const tjedlightrec& light, const std::vector<uint32_t>& selectedSectorBitmask)
{
	if (!(light.flags & ELight_Cleave))
		return;

	if (light.flags & ELight_Sun)
	{
		if (!(m_nBakeFlags & (ELightBake_Sun | ELightBake_AllowSunCleave)))
			return;

		// find the first anchor if there is one
		// TODO: this is probably redundant since we also do this in CLightBakerDlg, but I don't want to pass that here for now
		int nLightAnchorIndex = -1;
		for (int i = 0; i < m_pJedLevel->NLights(); ++i)
		{
			tjedlightrec anchorLight;
			memset(&anchorLight, 0, sizeof(tjedlightrec));
			if (FAILED(m_pJedLevel->GetLight(i, &anchorLight, lt_flags)))
				continue;

			if (anchorLight.flags & ELight_Anchor)
			{
				nLightAnchorIndex = i;
				break;
			}
		}

		// compute the sun direction
		float3 directionToSun{(float)light.x, (float)light.y, (float)light.z};
		if (nLightAnchorIndex >= 0)
		{
			tjedlightrec anchorLight;
			memset(&anchorLight, 0, sizeof(tjedlightrec));
			if (SUCCEEDED(m_pJedLevel->GetLight(nLightAnchorIndex, &anchorLight, lt_position)))
			{
				float3 anchor = {(float)anchorLight.x, (float)anchorLight.y, (float)anchorLight.z};
				directionToSun -= anchor;
			}
		}

		directionToSun = normalize(directionToSun);
		PrepareSunGeometry(light, directionToSun, selectedSectorBitmask);
		return;
	}

	if (light.flags & (ELight_Sky | ELight_Anchor | ELight_Ambient) || light.range <= 1e-5f)
		return;

	if (!(m_nBakeFlags & (ELightBake_Direct | ELightBake_AllowLightCleave)))
		return;

	if (!IsLayerBaked(light.layer))
		return;

	const int nStartSector = m_pJed->FindSectorForXYZ(light.x, light.y, light.z);
	if (!Jed::IsSectorValid(m_pJedLevel, nStartSector))
		return;

	if ((m_nBakeFlags & ELightBake_Selected) && selectedSectorBitmask.size() > 0)
	{
		const int nBucketIndex = nStartSector / 32;
		const int nBucketPlace = nStartSector % 32;
		if (nBucketIndex >= (int)selectedSectorBitmask.size() || (selectedSectorBitmask[nBucketIndex] & (1u << nBucketPlace)) == 0)
			return;
	}

	const SLightClipVolume emptyState;
	ProcessSectorPlaneRelations(light, {(float)light.x, (float)light.y, (float)light.z}, nStartSector, nStartSector, 0, emptyState, false);
}

void CCleaveLight::PrepareSunGeometry(const tjedlightrec& light, const float3& directionToSun, const std::vector<uint32_t>& selectedSectorBitmask)
{
	const int nSectorCount = m_pJedLevel->NSectors();
	for (int nSectorIndex = 0; nSectorIndex < nSectorCount; ++nSectorIndex)
	{

		if ((m_nBakeFlags & ELightBake_Selected) && selectedSectorBitmask.size() > 0)
		{
			const int nBucketIndex = nSectorIndex / 32;
			const int nBucketPlace = nSectorIndex % 32;
			if (nBucketIndex >= (int)selectedSectorBitmask.size() || (selectedSectorBitmask[nBucketIndex] & (1u << nBucketPlace)) == 0)
				return;
		}

		const int nInitialSurfaceCount = m_pJedLevel->SectorNSurfaces(nSectorIndex);
		for (int nSurfaceIndex = 0; nSurfaceIndex < nInitialSurfaceCount; ++nSurfaceIndex)
		{
			tjedsurfacerec record;
			memset(&record, 0, sizeof(tjedsurfacerec));
			if (FAILED(m_pJedLevel->GetSurface(nSectorIndex, nSurfaceIndex, &record, sf_all)))
				continue;

			if ((record.surfflags & ESurface_IsSky) == 0)
				continue;

			std::optional<float3> normalOpt = Jed::QuerySurfaceNormal(m_pJedLevel, nSectorIndex, nSurfaceIndex);
			if (!normalOpt.has_value())
				continue;
			float3 normal = normalOpt.value();
			
			std::optional<SPolygon> skyPolygonOpt = Jed::GetSurfacePolygon(m_pJedLevel, nSectorIndex, nSurfaceIndex);
			if (!skyPolygonOpt.has_value())
				continue;
			SPolygon& skyPolygon = skyPolygonOpt.value();

			// ignore surfaces that face the sun
			if (dot(normal, directionToSun) >= -1e-5f)
				continue;

			std::optional<SLightClipVolume> skyClipVolumeOpt = Jed::BuildSunClipVolume(directionToSun, skyPolygon);
			if (!skyClipVolumeOpt.has_value())
				continue;
			SLightClipVolume& skyClipVolume = skyClipVolumeOpt.value();

			ProcessSectorPlaneRelations(light, directionToSun, nSectorIndex, -1, 0, skyClipVolume, true);
		}
	}
}

void CCleaveLight::ProcessSectorPlaneRelations(const tjedlightrec& light, const float3& traversalSource, int nSectorIndex, int nPreviousSector,
											   int nDepth, const SLightClipVolume& clipVolume, bool bDirectionalSource)
{
	if (!Jed::IsSectorValid(m_pJedLevel, nSectorIndex))
		return;

	const float lightRangeSq = (float)(light.range * light.range);

	const int nInitialSurfaceCount = m_pJedLevel->SectorNSurfaces(nSectorIndex);
	if (nDepth < kMaxGeometryDepth)
	{
		for (int nSurfaceIndex = 0; nSurfaceIndex < nInitialSurfaceCount; ++nSurfaceIndex)
		{
			tjedsurfacerec surfaceRecord;
			memset(&surfaceRecord, 0, sizeof(tjedsurfacerec));
			if (FAILED(m_pJedLevel->GetSurface(nSectorIndex, nSurfaceIndex, &surfaceRecord, sf_all)))
				continue;

			const int nAdjacentSector = (int)surfaceRecord.adjoinsc;
			const int nAdjacentSurface = (int)surfaceRecord.adjoinsf;
			if (!Jed::IsSectorValid(m_pJedLevel, nAdjacentSector) || nAdjacentSurface < 0 || nAdjacentSector == nPreviousSector)
				continue;

			// if this adjoin is marked as blocking light, then don't recurse through it
			if (surfaceRecord.adjoinflags & EAdjoin_BlocksLight)
				continue;

			std::optional<float3> normalOpt = Jed::QuerySurfaceNormal(m_pJedLevel, nSectorIndex, nSurfaceIndex);
			if (!normalOpt.has_value())
				continue;
			float3 normal = normalOpt.value();
			
			std::optional<SPolygon> polyOpt = Jed::GetSurfacePolygon(m_pJedLevel, nSectorIndex, nSurfaceIndex);
			if (!polyOpt.has_value())
				continue;
			SPolygon& poly = polyOpt.value();

			if (bDirectionalSource)
			{
				// directional sources only recurse through adjoins that face the source direction
				if (dot(normal, traversalSource) <= 1e-5f)
					continue;
			}
			else
			{
				const float planeDistance = dot(normal, poly.vertices.front());
				const float lightDistance = dot(normal, traversalSource);
				const float signedDistance = lightDistance - planeDistance;
				if (signedDistance < 0.0 || std::fabs(signedDistance) > light.range)
					continue;

				if (signedDistance < 1e-5f)
				{
					const float3 nudgedLight = traversalSource - normal * 1e-5f;
					ProcessSectorPlaneRelations(light, nudgedLight, nAdjacentSector, nSectorIndex, nDepth + 1, clipVolume, bDirectionalSource);
					continue;
				}
			}

			SPolygon clippedPoly = poly.ClipAgainstPlanes(clipVolume.clip, 1e-5f);
			if (clippedPoly.vertices.size() < 3)
				continue;

			// point lights need a poly intersection and range test
			if (!bDirectionalSource)
			{
				// project the light onto the polygon's plane.
				const float normalLengthSq = lengthSq(normal);
				const float3& planePoint = clippedPoly.vertices.front();
				const float planeDistance = dot(normal, traversalSource - planePoint) / normalLengthSq;
				const float3 projectedLight = traversalSource - normal * planeDistance;

				float closestDistanceSq;
				if (clippedPoly.PointInside(projectedLight))
				{
					// distance to the plane is the shortest distance when inside
					closestDistanceSq = lengthSq(traversalSource - projectedLight);
				}
				else
				{
					// outside, find the closest point on each edge
					closestDistanceSq = std::numeric_limits<float>::max();

					const size_t vertexCount = clippedPoly.vertices.size();
					for (size_t i = 0; i < vertexCount; ++i)
					{
						const float3& a = clippedPoly.vertices[i];
						const float3& b = clippedPoly.vertices[(i + 1) % vertexCount];
						const float3 edge = b - a;
						const float edgeLengthSq = lengthSq(edge);

						const float t = edgeLengthSq > 0.0f ? std::clamp(dot(traversalSource - a, edge) / edgeLengthSq, 0.0f, 1.0f) : 0.0f;

						const float3 closestPoint = a + edge * t;
						closestDistanceSq = std::min(closestDistanceSq, lengthSq(traversalSource - closestPoint));
					}
				}

				// reject the polygon when its nearest point is outside the light range
				if (closestDistanceSq >= lightRangeSq)
					continue;
			}

			auto childOpt = Jed::BuildChildClipVolume(m_pJed, clipVolume, nSectorIndex, nSurfaceIndex, nAdjacentSector, nAdjacentSurface,
										   traversalSource, bDirectionalSource, clippedPoly, kPointLightsize);
			if (!childOpt.has_value())
			{
				continue;
			}
			SLightClipVolume& childClipVolume = childOpt.value();

			ProcessSectorPlaneRelations(light, traversalSource, nAdjacentSector, nSectorIndex, nDepth + 1, childClipVolume, bDirectionalSource);
		}
	}

	if (!clipVolume.IsConsistent() || clipVolume.clip.empty())
		return;

	// traversal still recurses through sectors on other layers, but only sectors on a baked layer actually get cleaved
	tjedsectorrec sectorRecord;
	memset(&sectorRecord, 0, sizeof(tjedsectorrec));
	if (SUCCEEDED(m_pJedLevel->GetSector(nSectorIndex, &sectorRecord, s_layer)) && !IsLayerBaked(sectorRecord.layer))
		return;

	CleaveVisibleSurfaces(nSectorIndex, traversalSource, (float)light.range, clipVolume, clipVolume.splitA, bDirectionalSource);
	CleaveVisibleSurfaces(nSectorIndex, traversalSource, (float)light.range, clipVolume, clipVolume.splitB, bDirectionalSource);
}

void CCleaveLight::CleaveVisibleSurfaces(int nSectorIndex, const float3& sourcePosition, float lightRange, const SLightClipVolume& clipVolume, const std::vector<SPlane>& cleavePlanes, bool bDirectionalSource)
{
	const size_t nPlaneCount = std::min(cleavePlanes.size(), clipVolume.clip.size());
	if (nPlaneCount == 0)
		return;

	int nSurfacesAdded = 0; // track how many surfaces were added to avoid spinning forever
	int nSurfaceCount = m_pJedLevel->SectorNSurfaces(nSectorIndex);
	for (int nSurfaceIndex = 0; nSurfaceIndex < nSurfaceCount && nSurfacesAdded < 1000; ++nSurfaceIndex)
	{
		tjedsurfacerec source;
		memset(&source, 0, sizeof(tjedsurfacerec));
		if (FAILED(m_pJedLevel->GetSurface(nSectorIndex, nSurfaceIndex, &source, sf_all)))
			continue;

		// ignore surfaces that can't be seen or are not diffuse/gouraud
		// TODO: should we actually cleave solid color surfaces too?
		if (source.geo < EGeoMode_Textured || source.light < ELightMode_Diffuse)
			continue;

		// ignore cog-linked and sky surfaces (cog linked surfaces are for game logic and cleaving them would break the cogs)
		if ((source.surfflags & ESurface_IsSky) || (source.surfflags & ESurface_CogLinked))
			continue;

		std::optional<SPolygon> currentPolygonOpt = Jed::GetSurfacePolygon(m_pJedLevel, nSectorIndex, nSurfaceIndex);
		if (!currentPolygonOpt.has_value())
			continue;
		SPolygon& currentPolygon = currentPolygonOpt.value();

		// if the surface has a really small area, then don't bother cleaving it
		if (currentPolygon.Area() < 1e-3f)
			continue;

		std::optional<float3> normalOpt = Jed::QuerySurfaceNormal(m_pJedLevel, nSectorIndex, nSurfaceIndex);
		if (!normalOpt.has_value())
			continue;
		float3 normal = normalOpt.value();
		
		if (bDirectionalSource)
		{
			if (dot(normal, sourcePosition) < -1e-5f)
				continue;
		}
		else
		{
			const float surfaceDistance = dot(normal, currentPolygon.vertices.front());
			const float lightDistance = dot(normal, sourcePosition);
			if (std::fabs(surfaceDistance - lightDistance) > lightRange || lightDistance < surfaceDistance)
				continue;
		}

		// the surface has to be at least partially lit through the adjoin to be worth splitting
		const SPolygon clippedSurface = currentPolygon.ClipAgainstPlanes(clipVolume.clip, 1e-5f);
		if (clippedSurface.vertices.size() < 3)
			continue;

		bool bCleaved = false;
		for (size_t nPlaneIndex = 0; nPlaneIndex < nPlaneCount; ++nPlaneIndex)
		{
			if (!clipVolume.clip[nPlaneIndex].bCanCleave)
				continue;

			// the shadow boundary is the clip plane, so that decides when a split is needed
			if (!currentPolygon.CrossesPlane(clipVolume.clip[nPlaneIndex]))
				continue;

			// Re-fetch the polygon because a previous plane may have cleaved it.
			auto polygonOpt = Jed::GetSurfacePolygon(m_pJedLevel, nSectorIndex, nSurfaceIndex);
			if (!polygonOpt.has_value())
				continue;

			SPolygon& polygon = polygonOpt.value();

			const SPlane& cleave = cleavePlanes[nPlaneIndex];

			SPlane oppositeCleave = cleave;
			oppositeCleave.normal = -oppositeCleave.normal;

			const SPolygon positivePart = currentPolygon.ClipAgainstPlanes({cleave}, 1e-5f);
			const SPolygon negativePart = currentPolygon.ClipAgainstPlanes({oppositeCleave}, 1e-5f);

			// reject cleaves that produce an edge, degenerate polygon, or tiny sliver
			constexpr float kMinCleaveArea = 1e-4f;
			if (positivePart.vertices.size() < 3 || negativePart.vertices.size() < 3 || positivePart.Area() < kMinCleaveArea || negativePart.Area() < kMinCleaveArea)
			{
				continue;
			}

			tjedvector cleaveNormal = Jed::WriteVector(cleave.normal);

			const int nNewSurface = m_pJed->CleaveSurface(nSectorIndex, nSurfaceIndex, &cleaveNormal, cleave.point.x, cleave.point.y, cleave.point.z);
			if (nNewSurface >= 0)
			{
				Jed::PropagateGeoAfterCleave(m_pJedLevel, nSectorIndex, nSurfaceIndex, nNewSurface, source);
				bCleaved = true;
				nSurfacesAdded++;
			}
		}

		// pick up the new surfaces that were added to the sector so they get the remaining planes applied
		if (bCleaved)
		{
			nSurfaceCount = m_pJedLevel->SectorNSurfaces(nSectorIndex);
		}
	}
}
