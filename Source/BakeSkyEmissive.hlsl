#include "Baking.hlsli"

[numthreads(16, 16, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int nVertexIndex = dispatchThreadID.x;
    if (nVertexIndex >= g_levelInfo.nTotalVertices)
        return;

	int nRayIndex = dispatchThreadID.y;
	if (nRayIndex >= g_levelInfo.nSkyEmissiveRays)
		return;

	int nSectorIndex = aVertices[nVertexIndex].nSectorIndex;	
	if (!IsSectorVisible(nSectorIndex))
		return;
		
	int nLayerIndex = aSectors[nSectorIndex].nLayerIndex;
	if (!IsLayerVisible(nLayerIndex))
		return;

	int nSurfaceIndex = aVertices[nVertexIndex].nSurfaceIndex;
	if(!(aSurfaces[nSurfaceIndex].nFlags & ESurface_IsVisible))
		return;

    float3 vertex = aVertices[nVertexIndex].position.xyz;
   	AdjustWorldPos(nSectorIndex, vertex);

	const float3 normal = normalize(aVertexNormals[nVertexIndex].xyz);
	const float3x3 frame = GenerateTangentFrame(normal);
	
	float4 color = float4(0,0,0,0);
	float3 rayDir = GenRay(nRayIndex, g_levelInfo.nSkyEmissiveRays);
	rayDir = mul(rayDir, frame);
	
	const float3 rayTarget = vertex + 100000.0f * rayDir;

	SRayPayload payload = (SRayPayload)0;
	payload.attenuation = float4(1,1,1,1);

	bool bRayHit = TraceRay(payload, nSectorIndex, vertex + rayDir * kRayBias, rayTarget);
	if (bRayHit)
	{
		uint nSurfaceFlags = aSurfaces[payload.nHitSurfaceIndex].nFlags;
		if ((g_levelInfo.nBakeFlags & ELightBake_Sky) && (nSurfaceFlags & ESurface_IsSky))
		{
			color = aLights[g_levelInfo.nSkyLightIndex].color;
		}
		else if ((g_levelInfo.nBakeFlags & ELightBake_Emissive) && (nSurfaceFlags & ESurface_IsVisible))
		{
			color = aSurfaces[payload.nHitSurfaceIndex].emissive;
		}
	}
	color *= payload.attenuation;

	color /= float(g_levelInfo.nSkyEmissiveRays);

	// Accumulate total
	WriteVertexLight(aVertexAccumulation, nVertexIndex, color);
}