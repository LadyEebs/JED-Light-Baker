#include "Baking.hlsli"

[numthreads(16, 16, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int nVertexIndex = dispatchThreadID.x;
    if (nVertexIndex >= g_levelInfo.nTotalVertices)
        return;

	int nLightIndex = dispatchThreadID.y;
	if (nLightIndex >= g_levelInfo.nTotalLights)
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

	int nLightSectorIndex = aLights[nLightIndex].nSectorIndex;
	if (nLightSectorIndex < 0 || (aLights[nLightIndex].nFlags & ELight_Sun) || (aLights[nLightIndex].nFlags & ELight_Sky)) // invalid sector or a sun/sky light
		return;
	
	const float3 vertex = aVertices[nVertexIndex].position.xyz;
	const float3 normal = normalize(aVertexNormals[nVertexIndex].xyz);

	const float3 lightDir = aLights[nLightIndex].position.xyz - vertex;
	const float ndotl = dot(lightDir, normal);
	if (ndotl <= 0.0f)
		return;

	const float rangeSqr = aLights[nLightIndex].range * aLights[nLightIndex].range;
	const float dist2 = dot(lightDir, lightDir);
	if (dist2 >= rangeSqr)
		return;
			
	const float dist = sqrt(dist2);
	const float3 lightVec = lightDir / dist;

	SRayPayload payload = (SRayPayload)0;
	payload.attenuation = float4(1,1,1,1);
	if (!(aLights[nLightIndex].nFlags & ELight_NotBlocked))
	{		
		if (nLightSectorIndex != nSectorIndex)
		{
			bool bRayHit = TraceRay(payload, nSectorIndex, vertex + lightDir * kRayBias, aLights[nLightIndex].position.xyz);
			if (bRayHit)
				return;
		}
	}

	float4 color = aLights[nLightIndex].color * payload.attenuation;

	if (g_levelInfo.nBakeFlags & ELightBake_PhysicalFalloff) // new hotness hybrid with inverse square falloff
	{
		float atten = 1.0f / max(dist2, 0.001f);
		float fade = saturate(1.0f - dist / aLights[nLightIndex].range);
		fade *= fade;
		atten *= fade;
		color *= atten * saturate(dot(lightVec, normal));
	}
	else // old'n'busted linear falloff
	{
		float atten = (aLights[nLightIndex].range - dist) / aLights[nLightIndex].range;
		atten *= atten;
		color *= atten;// * saturate(dot(lightVec, normal));
	}

	// Accumulate total
	WriteVertexLight(aVertexAccumulation, nVertexIndex, color);
}