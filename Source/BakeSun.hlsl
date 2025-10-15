#include "Baking.hlsli"

[numthreads(256, 1, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int nVertexIndex = dispatchThreadID.x;
    if (nVertexIndex >= g_levelInfo.nTotalVertices)
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
    float3 normal = normalize(aVertexNormals[nVertexIndex].xyz);

	float3 sunPos = aLights[g_levelInfo.nSunLightIndex].position.xyz;
	if (g_levelInfo.nAnchorLightIndex >= 0)
		sunPos -= aLights[g_levelInfo.nAnchorLightIndex].position.xyz;

	float3 rayDir    = normalize(sunPos.xyz);
	float3 rayTarget = vertex + 100000.0f * rayDir;
	
	float4 color = float4(0,0,0,0);
	float ndotl      = dot(normal, rayDir);
	
	if (ndotl > 0)
	{
		SRayPayload payload = (SRayPayload)0;
		payload.attenuation = float4(1,1,1,1);

		int nStartSector = GetRayStartSector(nSectorIndex, nSurfaceIndex, normal, rayDir);
		bool bRayHit = TraceRay(payload, nStartSector, vertex, rayTarget);
		if (bRayHit && (aSurfaces[payload.nHitSurfaceIndex].nFlags & ESurface_IsSky))
		    color = ndotl * payload.attenuation * aLights[g_levelInfo.nSunLightIndex].color;
	}
	
	// Write the result
	aVertexAccumulation[nVertexIndex] = asuint(color);
}