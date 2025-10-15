// must match ELightBakeFlags
static const uint ELightBake_Lights             =   0x1;
static const uint ELightBake_Sun                =   0x2;
static const uint ELightBake_Sky                =   0x4;
static const uint ELightBake_Emissive           =   0x8;
static const uint ELightBake_Indirect           =  0x10;
static const uint ELightBake_Selected           =  0x20;
static const uint ELightBake_VisibleLayers      =  0x40;
static const uint ELightBake_GammaCorrect       =  0x80;
static const uint ELightBake_ExtraLightEmissive = 0x100;
static const uint ELightBake_ToneMap            = 0x200;
static const uint ELightBake_PhysicalFalloff    = 0x400;

// must match ESurfaceFlags
static const uint ESurface_IsSky         = 0x1;
static const uint ESurface_IsVisible     = 0x2;
static const uint ESurface_IsTranslucent = 0x4;

// must match ELightFlags
static const uint ELight_NotBlocked = 0x1;
static const uint ELight_Sun        = 0x2;
static const uint ELight_Sky        = 0x4;
static const uint ELight_Anchor     = 0x8;

static const float kSkyDistance = 512.0;

// standard JK alpha value
static const float kSurfaceAlpha = 90.0/255.0;

// bias the ray along the ray dir to avoid self intersection or stuff like that
static const float kRayBias = -1e-4;

// maximum number of adjoins to cross for a given ray (puts an upper bound on recursions for safety)
static const int kMaxRecursion = 1024; // should be more than enough?

// Ray payload, filled on hit
struct SRayPayload
{
	float4 attenuation;
	float4 reflection;
	float3 hitPos;
	int    nHitSurfaceIndex;
};

// CPU mirrored structs
struct SSector
{
	uint   nFirstSurface;
	uint   nNumSurfaces;
	int    nLayerIndex;
	uint   _padding;
	float4 center;
};

struct SSurface
{
	float4 albedo;
	float4 emissive;
	float4 normal;
	uint   nFirstVertex;
	uint   nNumVertices;
	int    nAdjoinSector;
	uint   nFlags;
};

struct SVertex
{
	uint   nSectorIndex;
	uint   nSurfaceIndex;
	uint2  _padding;
	float4 position;
};

struct SLight
{
	int    nSectorIndex;
	uint   nFlags;
	int    nLayerIndex;
	float  range;
	float4 position;
	float4 color;
};

struct SLevelInfo
{
	int  nSunLightIndex;
	int  nSkyLightIndex;
	int  nAnchorLightIndex;
	int _padding;

	int nTotalSectors;
	int nTotalSurfaces;
	int nTotalVertices;
	int nTotalLights;

	uint nBakeFlags;
	int  nSkyEmissiveRays;
	int  nIndirectRays;
	int  _padding1;

	int4 _padding2;
};

cbuffer CBLevelInfo : register( b0 )
{
	SLevelInfo g_levelInfo;
};

// Resources
Buffer<uint>               aSectorMasks   : register(t0);
Buffer<uint>               aLayerMasks    : register(t1);
StructuredBuffer<SSector>  aSectors       : register(t2);
StructuredBuffer<SSurface> aSurfaces      : register(t3);
StructuredBuffer<SVertex>  aVertices      : register(t4);
StructuredBuffer<SLight>   aLights        : register(t5);
StructuredBuffer<float4>   aVertexNormals : register(t6);
StructuredBuffer<float4>   aVertexColors  : register(t7);

RWStructuredBuffer<uint4> aVertexColorsWrite  : register(u0);
RWStructuredBuffer<uint4> aVertexAccumulation : register(u1);

// Test if a sector is in the sector bitmask
bool IsSectorVisible(int nSectorIndex)
{
	const uint nBucketIndex = nSectorIndex / 32u;
	const uint nBucketPlace = nSectorIndex % 32u;	
	return (aSectorMasks[nBucketIndex] & (1u << nBucketPlace));
}

// Test if a layer is in the layer bitmask
bool IsLayerVisible(int nLayerIndex)
{
	const uint nBucketIndex = nLayerIndex / 32u;
	const uint nBucketPlace = nLayerIndex % 32u;	
	return (aLayerMasks[nBucketIndex] & (1u << nBucketPlace));
}

// Generates an arbitrary tangent frame around a normal
float3x3 GenerateTangentFrame(float3 normal)
{
	const float3 up = abs(normal.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
	const float3 tangent = normalize(cross(up, normal));
	const float3 binormal = cross(normal, tangent);
	return float3x3(tangent, binormal, normal);
}

// Generate a cosine weighted ray using a hammersley sequence
float3 GenRay(int N, int M)
{
	uint bits = N;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float v = (float)bits * 2.3283064365386963e-10f; // / 0x100000000
	float u = (float)N / (float)M;

	u = lerp(0.0001, 1.0, u);
	const float phi = v * 2.0f * 3.141592f;
	const float cosTheta = sqrt(1.0f - u);
	const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// Shoddy interpolation of vertex color over a surface, but has better properties than triangle interpolation
float4 InterpolateSurfaceLight(int nSurfaceIndex, float3 pos)
{
	const uint nFirstVertex = aSurfaces[nSurfaceIndex].nFirstVertex;
    const uint nNumVertices = aSurfaces[nSurfaceIndex].nNumVertices;

    float4 vertexLight = float4(0,0,0,0);
    float totalWeight = 0.0;

    [loop]
    for (uint nVertexIndex = 0; nVertexIndex < nNumVertices; ++nVertexIndex)
    {
        // Previous, current, next vertex indices
        const uint nPrevIndex = (nVertexIndex + nNumVertices - 1) % nNumVertices;
        const uint nCurrIndex = nVertexIndex;
        const uint nNextIndex = (nVertexIndex + 1) % nNumVertices;

        const float3 vPrev = aVertices[nFirstVertex + nPrevIndex].position.xyz;
        const float3 vCurr = aVertices[nFirstVertex + nCurrIndex].position.xyz;
        const float3 vNext = aVertices[nFirstVertex + nNextIndex].position.xyz;

        // Vectors from pos to vertices
		float3 wPrev = vPrev - pos;
		float3 wCurr = vCurr - pos;
		float3 wNext = vNext - pos;

        const float lenPrev = length(wPrev) + 1e-6;
        const float lenCurr = length(wCurr) + 1e-6;
        const float lenNext = length(wNext) + 1e-6;

        wPrev /= lenPrev;
        wCurr /= lenCurr;
        wNext /= lenNext;

        // Compute tan(theta/2) for previous and next angles
        const float sinThetaPrev = length(cross(wCurr, wPrev));
        const float cosThetaPrev = dot(wCurr, wPrev);
        const float tanHalfPrev = sinThetaPrev / (1.0 + cosThetaPrev + 1e-6);

        const float sinThetaNext = length(cross(wNext, wCurr));
        const float cosThetaNext = dot(wNext, wCurr);
        const float tanHalfNext = sinThetaNext / (1.0 + cosThetaNext + 1e-6);

        // Weight for this vertex
        const float weight = (tanHalfPrev + tanHalfNext) / lenCurr;

        vertexLight += aVertexColors[nFirstVertex + nCurrIndex] * weight;
        totalWeight += weight;
    }

    return totalWeight < 1e-6 ? float4(0,0,0,0) : vertexLight / totalWeight;
}

// Test if a point is within a surfaces edges
bool IsPointOnSurface(int nSurfaceIndex, float3 position)
{
	const float3 normal = aSurfaces[nSurfaceIndex].normal.xyz;

	const uint nFirstVertex = aSurfaces[nSurfaceIndex].nFirstVertex;
	const uint nNumVertices = aSurfaces[nSurfaceIndex].nNumVertices;

	bool bResult = true;

	[loop]
	for (uint i = 0; i < nNumVertices; ++i)
	{
		const float3 vertex1 = aVertices[nFirstVertex + i].position.xyz;
		const float3 vertex2 = aVertices[nFirstVertex + (i + 1) % nNumVertices].position.xyz;
		const float dist = dot(cross(normal, vertex2 - vertex1), position - vertex1);
		if (dist < -1e-3)
		{
			bResult = false;
			break;
		}
	}

	return bResult;
}

// Test if a segment/line/ray crossed a surface
bool IsSurfCrossed(int nSurfaceIndex, float3 start, float3 end, inout float3 hitPos)
{
	// initialize out parameter
	hitPos = float3(0,0,0);
	bool bResult = false;

	const int nNumVertices = aSurfaces[nSurfaceIndex].nNumVertices;
	
	[branch]
	if (nNumVertices > 0)
	{
		const int nFirstVertex = aSurfaces[nSurfaceIndex].nFirstVertex;

		const float3 normal = aSurfaces[nSurfaceIndex].normal.xyz;
		const float3 vertex = aVertices[nFirstVertex].position.xyz;

		const float distToStart = dot(normal, start - vertex);
		const float distToEnd   = dot(normal, end - vertex);

		const bool bAllOutsidePositive = (distToStart > 0.0001 && distToEnd > 0.0001);
		const bool bAllOutsideNegative = (distToStart < -0.0001 && distToEnd < -0.0001);
		const bool bBothOnPlane        = (abs(distToStart) <= 0.0001 && abs(distToEnd) <= 0.0001);

		[branch]
		if (!bAllOutsidePositive && !bAllOutsideNegative)
		{
			[branch]
			if (bBothOnPlane)
			{
				hitPos = start;
				bResult = true;
			}
			else
			{
				float s = distToStart / (distToStart - distToEnd);
				hitPos = start + s * (end - start);
				bResult = IsPointOnSurface(nSurfaceIndex, hitPos);
			}
		}
	}
	return bResult;
}

// Test all the surfaces in a sector, return true if something was hit as well as hit position and surface index
// todo: split adjoins and surfaces into their own data so we're not doing a dumb check for all of them
bool TraceSurfaces(int nSectorIndex, float3 start, float3 end, inout float3 hitPos, inout int nHitSurfaceIndex)
{
	bool bResult = false;

	const uint nFirstSurface = aSectors[nSectorIndex].nFirstSurface;
	const uint nLastSurface  = nFirstSurface + aSectors[nSectorIndex].nNumSurfaces;

	uint nSurfaceIndex;

	[loop]
	for (nSurfaceIndex = nFirstSurface; nSurfaceIndex < nLastSurface; ++nSurfaceIndex)
	{
		if (aSurfaces[nSurfaceIndex].nAdjoinSector < 0)
			continue;

		const float3 normal = aSurfaces[nSurfaceIndex].normal.xyz;
		const float dotVal  = dot(normal, end - start);
		if (dotVal < 0)
		{
			if (IsSurfCrossed(nSurfaceIndex, start, end, hitPos))
			{
				nHitSurfaceIndex = nSurfaceIndex;
				bResult = true;
				break;
			}
		}
	}

	if (!bResult)
	{	
		[loop]
		for (nSurfaceIndex = nFirstSurface; nSurfaceIndex < nLastSurface; ++nSurfaceIndex)
		{
			if (aSurfaces[nSurfaceIndex].nAdjoinSector >= 0)
				continue;

			const float3 normal = aSurfaces[nSurfaceIndex].normal.xyz;
			const float dotVal  = dot(normal, end - start);
			if (dotVal < 0)
			{
				if (IsSurfCrossed(nSurfaceIndex, start, end, hitPos))
				{
					nHitSurfaceIndex = nSurfaceIndex;
					bResult = true;
					break;
				}
			}
		}
	}
	return bResult;
}

bool TraceRay(inout SRayPayload payload, int nSectorIndex, float3 start, float3 end)
{
	int nRecurseLevel  = 0;
	int nCurrentSector = nSectorIndex;
	int nPreviousSector = -1;

	payload.nHitSurfaceIndex = -1;
	payload.hitPos = start;
	payload.attenuation = float4(1,1,1,1);

	[loop]
	while (nCurrentSector >= 0 && nCurrentSector != nPreviousSector && nRecurseLevel < kMaxRecursion)
	{
		float3 hitPos = start;
		int nHitSurfaceIndex = -1;
		const bool bRayHit = TraceSurfaces(nCurrentSector, start, end, hitPos, nHitSurfaceIndex);
		
		// didn't hit anything
		if (nHitSurfaceIndex < 0)
			return false;

		// we hit something
		payload.nHitSurfaceIndex = nHitSurfaceIndex;
		payload.hitPos = hitPos;
		
		// move to next sector (if available)
		nPreviousSector = nCurrentSector;
		nCurrentSector = aSurfaces[nHitSurfaceIndex].nAdjoinSector;
		if (nCurrentSector >= 0)
		{
			// add transparent contributions
			uint nSurfaceFlags = aSurfaces[nHitSurfaceIndex].nFlags;
			if (nSurfaceFlags & ESurface_IsVisible)
			{
				float4 albedo = aSurfaces[nHitSurfaceIndex].albedo;
				if (nSurfaceFlags & ESurface_IsTranslucent)
					albedo *= kSurfaceAlpha;

				const float4 surfaceLight = InterpolateSurfaceLight(nHitSurfaceIndex, hitPos);
				payload.reflection += albedo * payload.attenuation * surfaceLight;

				if (nSurfaceFlags & ESurface_IsTranslucent)
					payload.attenuation *= albedo + (1.0 - kSurfaceAlpha);
			}
			++nRecurseLevel;
		}
	}

	return payload.nHitSurfaceIndex >= 0;
}

// Uses a spinlock to add results to a vertex (one for each channel)
void WriteVertexLight(in RWStructuredBuffer<uint4> buffer, int nVertexIndex, float4 light)
{
	[unroll]
	for (int nChannel = 0; nChannel < 4; ++nChannel)
	{
		uint nNewValue = asuint(light[nChannel]);
		uint nTmp0 = 0;
		uint nTmp1;

		[allow_uav_condition]
		while(true)
		{
			InterlockedCompareExchange(buffer[nVertexIndex][nChannel], nTmp0, nNewValue, nTmp1);
			if (nTmp1 == nTmp0)
				break;
			nTmp0 = nTmp1;
			nNewValue = asuint(light[nChannel] + asfloat(nTmp1));
		}
	}
}