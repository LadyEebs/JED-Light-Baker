#include "Baking.hlsli"

[numthreads(16, 16, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID)
{
    const int nVertexIndex = dispatchThreadID.x;
    if (nVertexIndex >= g_levelInfo.nTotalVertices)
        return;

	const int nRayIndex = dispatchThreadID.y;
	if (nRayIndex >= g_levelInfo.nIndirectRays)
		return;

	const int nSectorIndex = aVertices[nVertexIndex].nSectorIndex;	
	if (!IsSectorVisible(nSectorIndex))
		return;
		
	const int nLayerIndex = aSectors[nSectorIndex].nLayerIndex;
	if (!IsLayerVisible(nLayerIndex))
		return;

	const int nSurfaceIndex = aVertices[nVertexIndex].nSurfaceIndex;
	if(!(aSurfaces[nSurfaceIndex].nFlags & ESurface_IsVisible))
		return;

    const float3 vertex = aVertices[nVertexIndex].position.xyz;
    const float3 normal = normalize(aVertexNormals[nVertexIndex].xyz);
	const float3x3 frame = GenerateTangentFrame(normal);

	float4 color = float4(0,0,0,0);

	float3 rayDir = GenRay(nRayIndex, g_levelInfo.nIndirectRays);
	rayDir = mul(rayDir, frame);
	
	float3 rayTarget = vertex + 100000.0f * rayDir;

	SRayPayload payload = (SRayPayload)0;
	payload.attenuation = float4(1,1,1,1);

	int nStartSector = GetRayStartSector(nSectorIndex, nSurfaceIndex, normal, rayDir);
	const bool bRayHit = TraceRay(payload, nStartSector, vertex + rayDir * kRayBias, rayTarget);
	if (bRayHit && (aSurfaces[payload.nHitSurfaceIndex].nFlags & ESurface_IsVisible))
	{
		const float4 surfaceLight = InterpolateSurfaceLight(payload.nHitSurfaceIndex, payload.hitPos);
		color = aSurfaces[payload.nHitSurfaceIndex].albedo * surfaceLight;
	}
	color = color * payload.attenuation + payload.reflection;

	color /= float(g_levelInfo.nIndirectRays);

	// Write the bounce result
	WriteVertexLight(aVertexColorsWrite, nVertexIndex, color);

	// Accumulate total
	// todo: do this all at once in an separate pass without atomics
	WriteVertexLight(aVertexAccumulation, nVertexIndex, color);
}