#include "Baking.hlsli"

[numthreads(256, 1, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID)
{
	SVertexData vertexData;
	if (!GetVertexData(vertexData, dispatchThreadID.x))
		return;

	float3 sunPos = aLights[g_levelInfo.nSunLightIndex].position.xyz;
	if (g_levelInfo.nAnchorLightIndex >= 0)
		sunPos -= aLights[g_levelInfo.nAnchorLightIndex].position.xyz;

	float3 rayDir    = normalize(sunPos.xyz);
	float3 rayTarget = kSkyDistance * rayDir + vertexData.vertex;
	
	float4 color = float4(0,0,0,0);
	float ndotl  = dot(vertexData.normal, rayDir);	
	if (ndotl > 0)
	{
		SRayPayload payload = (SRayPayload)0;
		payload.attenuation = float4(1,1,1,1);

		bool bRayHit = TraceRay(payload, vertexData.nSectorIndex, vertexData.vertex + rayDir * kRayBias, rayTarget);
		if (bRayHit && (aSurfaces[payload.nHitSurfaceIndex].nFlags & ESurface_IsSky))
			color = ndotl * payload.attenuation * aLights[g_levelInfo.nSunLightIndex].color;
	
		// sun is the first pass so don't bother reading the previous result
		aVertexAccumulation[vertexData.nVertexIndex] = color;
	}
}