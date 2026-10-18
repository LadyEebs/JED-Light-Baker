#pragma once

#include "IJed.h"
#include "float3.h"
#include "Geometry.h"

#include <cstdint>

class CCleaveLight
{
public:
	CCleaveLight(IJED* pJed, IJEDLevel* pJedLevel, uint32_t nBakeFlags);

	void PrepareGeometry(const tjedlightrec& light, const std::vector<uint32_t>& selectedSectorBitmask);

private:
	bool IsLayerBaked(int nLayerIndex) const;

	void PrepareSunGeometry(const tjedlightrec& light, const float3& directionToSun, const std::vector<uint32_t>& selectedSectorBitmask);

	void ProcessSectorPlaneRelations(const tjedlightrec& light, const float3& traversalSource, int nSectorIndex, int nPreviousSector,
									 int nDepth, const SLightClipVolume& clipVolume, bool bDirectionalSource);

	void CleaveVisibleSurfaces(int nSectorIndex, const float3& sourcePosition, float lightRange, const SLightClipVolume& clipVolume, const std::vector<SPlane>& cleavePlanes, bool bDirectionalSource);

private:
	IJED* m_pJed;
	IJEDLevel* m_pJedLevel;
	uint32_t m_nBakeFlags;
};