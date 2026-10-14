#include "Baking.hlsli"

#define LIGHTS_PER_GROUP 256

groupshared float4 g_sharedAcc[LIGHTS_PER_GROUP];

[numthreads(LIGHTS_PER_GROUP, 1, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID,
		  int3 groupThreadID : SV_GroupThreadID,
		  int3 groupID : SV_GroupID)
{
	SVertexData vertexData;
	if (!GetVertexData(vertexData, groupID.x))
		return;

	// each thread processes g_levelInfo.nTotalLights / LIGHTS_PER_GROUP lights
	float4 localAcc = float4(1, 1, 1, 1);
	for (int nLightIndex = groupThreadID.x; nLightIndex < g_levelInfo.nTotalLights; nLightIndex += LIGHTS_PER_GROUP)
	{
		int nLightSectorIndex = aLights[nLightIndex].nSectorIndex;
		if (nLightSectorIndex < 0 || (aLights[nLightIndex].nFlags & (ELight_Sun | ELight_Sky))) // invalid sector or a sun/sky light
			continue;
        
        // only process ambient lights in this pass
		if (!(aLights[nLightIndex].nFlags & ELight_Ambient))
			continue;

		if (!IsSectorVisible(nLightSectorIndex))
			continue;

		if (!IsLayerVisible(aLights[nLightIndex].nLayerIndex))
			continue;

		const float3 lightDir = aLights[nLightIndex].position.xyz - vertexData.vertex;
		const float ndotl = dot(lightDir, vertexData.normal);
		if (ndotl <= 0.0f)
			continue;

		const float rangeSqr = aLights[nLightIndex].range * aLights[nLightIndex].range;
		const float dist2 = dot(lightDir, lightDir);
		if (dist2 >= rangeSqr)
			continue;
			
		const float dist = sqrt(dist2);
		const float3 lightVec = lightDir / dist;

		SRayPayload payload = (SRayPayload) 0;
		payload.attenuation = float4(1, 1, 1, 1);
		if (!(aLights[nLightIndex].nFlags & ELight_NotBlocked))
		{
			if (nLightSectorIndex != vertexData.nSectorIndex)
			{
				bool bRayHit = TraceRay(payload, vertexData.nSectorIndex, vertexData.vertex + lightDir * kRayBias, aLights[nLightIndex].position.xyz);
				if (bRayHit)
					continue;
			}
		}

		float4 color = aLights[nLightIndex].color * payload.attenuation;
		// always using linear falloff
		float atten = (aLights[nLightIndex].range - dist) / aLights[nLightIndex].range;
		atten *= atten;
		color *= atten;
				
		localAcc *= color;
	}

	// write the result to shared memory
	g_sharedAcc[groupThreadID.x] = localAcc;
	GroupMemoryBarrierWithGroupSync();

	// tree reduction in shared memory
	for (int stride = LIGHTS_PER_GROUP / 2; stride > 0; stride >>= 1)
	{
		if (groupThreadID.x < stride)
			g_sharedAcc[groupThreadID.x] *= g_sharedAcc[groupThreadID.x + stride];
		GroupMemoryBarrierWithGroupSync();
	}

	// only thread 0 writes final result for this vertex
	if (groupThreadID.x == 0)
	{
		float4 prevResult = aVertexAccumulation[vertexData.nVertexIndex];
        // ambient lights are multiplicative
		aVertexAccumulation[vertexData.nVertexIndex] = prevResult * g_sharedAcc[0];
	}
}