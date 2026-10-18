#include "Baking.hlsli"

RWStructuredBuffer<int4> aNormalsWrite : register(u0);

StructuredBuffer<uint> aNormalNeighborOffsets : register(t5);
StructuredBuffer<uint> aNormalNeighborIndices : register(t6);

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	const uint nVertexIndex = dispatchThreadID.x;

	uint nVertexCount, nVertexStride;
	aVertices.GetDimensions(nVertexCount, nVertexStride);

	if (nVertexIndex >= nVertexCount)
		return;

	const uint nSurfaceIndex0 = aVertices[nVertexIndex].nSurfaceIndex;
	const float3 normal0 = aSurfaces[nSurfaceIndex0].normal.xyz;

	int4 accumulated = int4(int3(normal0 * 1024.0), 1);
	
	const uint nBegin = aNormalNeighborOffsets[nVertexIndex];
	const uint nEnd = aNormalNeighborOffsets[nVertexIndex + 1];
	for (uint i = nBegin; i < nEnd; ++i)
	{
		const uint nNeighborIndex = aNormalNeighborIndices[i];
		const uint nNeighborSurfaceIndex = aVertices[nNeighborIndex].nSurfaceIndex;

		const float3 neighborNormal = aSurfaces[nNeighborSurfaceIndex].normal.xyz;

		if (dot(normal0, neighborNormal) > g_levelInfo.normalSmoothCos)
		{
			accumulated.xyz += int3(neighborNormal * 1024.0);
		}
	}

	aNormalsWrite[nVertexIndex] = accumulated;
}