#include "Baking.hlsli"

#define RAYS_PER_GROUP 256

groupshared float4 g_sharedAcc[RAYS_PER_GROUP];

[numthreads(RAYS_PER_GROUP, 1, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID,
		  int3 groupThreadID    : SV_GroupThreadID,
		  int3 groupID          : SV_GroupID)
{
	SVertexData vertexData;
	if (!GetVertexData(vertexData, groupID.x))
		return;

	const float3x3 frame = GenerateTangentFrame(vertexData.normal);

	// each thread processes g_levelInfo.nSkyEmissiveRays / RAYS_PER_GROUP rays
	float4 localAcc = float4(0,0,0,0);
	for(int nRayIndex = groupThreadID.x; nRayIndex < g_levelInfo.nSkyEmissiveRays; nRayIndex += RAYS_PER_GROUP)
	{
		float3 rayDir = GenRay(nRayIndex, g_levelInfo.nSkyEmissiveRays);
		rayDir = mul(rayDir, frame);

		const float3 rayTarget = kSkyDistance * rayDir + vertexData.vertex;

		SRayPayload payload = (SRayPayload)0;
		payload.attenuation = float4(1,1,1,1);

		bool bRayHit = TraceRay(payload, vertexData.nSectorIndex, vertexData.vertex + rayDir * kRayBias, rayTarget);
		if(bRayHit)
		{
			float4 color = float4(0,0,0,0);
			uint hitFlags = aSurfaces[payload.nHitSurfaceIndex].nFlags;
			if((g_levelInfo.nBakeFlags & ELightBake_Sky) && (hitFlags & ESurface_IsSky))
			{
				color = aLights[g_levelInfo.nSkyLightIndex].color;
			}
			else if((g_levelInfo.nBakeFlags & ELightBake_Emissive) && (hitFlags & ESurface_IsVisible))
			{
				color = aSurfaces[payload.nHitSurfaceIndex].emissive;
			}

			localAcc += color * payload.attenuation;
		}
	}

	// write the result to shared memory
	g_sharedAcc[groupThreadID.x] = localAcc;
	GroupMemoryBarrierWithGroupSync();

	// tree reduction in shared memory
	for(int stride = RAYS_PER_GROUP / 2; stride > 0; stride >>= 1)
	{
		if(groupThreadID.x < stride)
			g_sharedAcc[groupThreadID.x] += g_sharedAcc[groupThreadID.x + stride];
		GroupMemoryBarrierWithGroupSync();
	}

	// only thread 0 writes final result for this vertex
	if(groupThreadID.x == 0)
	{
		float4 result = g_sharedAcc[0] / (float)g_levelInfo.nSkyEmissiveRays;
		float4 prevResult = aVertexAccumulation[vertexData.nVertexIndex];
		aVertexAccumulation[vertexData.nVertexIndex] = prevResult + result;
	}
}
