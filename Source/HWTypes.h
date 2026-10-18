#pragma once

// be careful of constant buffer padding here

// GPU mirrored structs
struct SSector
{
	uint32_t nFirstSurface;
	uint32_t nNumSurfaces;
	uint32_t nLayerIndex;
	uint32_t _padding;
	float4   center;
};

struct SSurface
{
	float4    albedo;
	float4    emissive;
	float4    normal;
	uint32_t  nFirstVertex;
	uint32_t  nNumVertices;
	int32_t   nAdjoinSector;
	uint32_t  nFlags;
};

struct SVertex
{
	uint32_t  nSectorIndex;
	uint32_t  nSurfaceIndex;
	uint32_t  nLocalSurfaceIndex;
	uint32_t  nLocalVertexIndex;
	float4    position;
};

struct SNormalVertexPair
{
	uint32_t nVertex0;
	uint32_t nVertex1;
};

struct SLight
{
	uint32_t  nFlags;
	int32_t   nSectorIndex;
	int32_t   nLayerIndex;
	float     range;
	float4    position;
	float4    color;
};

struct SLevelInfo
{
	int32_t nSunLightIndex;
	int32_t nSkyLightIndex;
	int32_t nAnchorLightIndex;
	int32_t _padding0;

	int32_t nTotalSectors;
	int32_t nTotalSurfaces;
	int32_t nTotalVertices;
	int32_t nTotalLights;

	uint32_t nBakeFlags;
	int32_t  nSkyEmissiveRays;
	int32_t  nIndirectRays;
	float    normalSmoothCos;

	int32_t  _padding2[4];
};