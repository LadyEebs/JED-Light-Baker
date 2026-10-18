#pragma once

#include "Enums.h"
#include "float3.h"

// Minimal colormap support (for reading basic color and light table)
struct SColormapHeader
{
	char    magic[4]; // "CMP "
	int32_t nVersion;
	int32_t nFlags;
	float3  colorTint;
	int32_t _padding[10];
};

struct SColormap
{
	struct
	{
		uint8_t r, g, b;
	} colors[256];
	uint8_t nLightLevels[64 * 256];

	float3 GetColor(uint32_t nIndex, int nLightLevel) const;
};

// Minimal material support (for reading material fill colors)
struct SColorFormat
{
	uint32_t nColorMode = 0;
	uint32_t nBitsPerPixel = 0;
	uint32_t nRedBits = 0;
	uint32_t nGreenBits = 0;
	uint32_t nBlueBits = 0;
	uint32_t nRedShift = 0;
	uint32_t nGreenShift = 0;
	uint32_t nBlueShift = 0;
	uint32_t nRedDiff = 0;
	uint32_t nGreenDiff = 0;
	uint32_t nBlueDiff = 0;
	uint32_t nAlphaBits = 0;
	uint32_t nAlphaShift = 0;
	uint32_t nAlphaDiff = 0;
};

struct SMaterialFileHeader
{
	uint8_t      magic[4];
	uint32_t     nVersion = 0;
	uint32_t     nType = 0;
	uint32_t     nRecordCount = 0;
	uint32_t     nTextureCount = 0;
	SColorFormat colorFormat;
};

struct SMaterialRecordHeader
{
	uint32_t nTextureType = EMaterialRecordType_Color;
	uint32_t nFillColor = 0;
	uint32_t nUnknown1 = 0;
	uint32_t nUnknown2 = 0;
	uint32_t nUnknown3 = 0;
	uint32_t nUnknown4 = 0;
};

struct SMaterialRecordHeaderExt
{
	uint32_t nUnknown = 0;
	uint32_t nHeight = 0;
	uint32_t nTransparentColor = 0;
	uint32_t nTextureIndex = 0;
};

// Spatial vertex for normal smoothing
struct SSpatialVertex
{
	float x, y, z;
	uint32_t nSectorIndex;
	uint32_t nNextInCell;
};

// A cell in the normal smoothing grid, used for hashing vertices into spatial bins
struct SNormalCell
{
	int x, y, z;

	bool operator==(const SNormalCell& rhs) const noexcept
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}
};

// Hash function for SNormalCell
struct SNormalCellHash
{
	size_t operator()(const SNormalCell& c) const noexcept
	{
		size_t h = std::hash<int>{}(c.x);
		h ^= std::hash<int>{}(c.y) + size_t(0x9e3779b9) + (h << 6) + (h >> 2);
		h ^= std::hash<int>{}(c.z) + size_t(0x9e3779b9) + (h << 6) + (h >> 2);
		return h;
	}
};

// Map from SNormalCell to the index of the first vertex in that cell
using SNormalCellMap = std::unordered_map<SNormalCell, uint32_t, SNormalCellHash>;
