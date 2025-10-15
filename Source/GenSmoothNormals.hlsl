#include "Baking.hlsli"

[numthreads(16, 16, 1)]
void main(int3 dispatchThreadID : SV_DispatchThreadID)
{
	const int nVertexIndex0 = dispatchThreadID.x;
	if (nVertexIndex0 >= g_levelInfo.nTotalVertices)
		return;

	const int nVertexIndex1 = dispatchThreadID.y;
	if (nVertexIndex1 >= g_levelInfo.nTotalVertices)
		return;
	
	// avoid duplicate pairs
	if (nVertexIndex0 >= nVertexIndex1)
		return;

	const float3 vertex0 = aVertices[nVertexIndex0].position.xyz;
	const float3 vertex1 = aVertices[nVertexIndex1].position.xyz;
	
	const float3 normal0 = aSurfaces[aVertices[nVertexIndex0].nSurfaceIndex].normal.xyz;
	const float3 normal1 = aSurfaces[aVertices[nVertexIndex1].nSurfaceIndex].normal.xyz;

	const float3 diff = vertex1 - vertex0;
	if (dot(diff, diff) < 1e-5) // vertices are close, consider them the same
	{
		if (dot(normal0, normal1) > cos(25.0 * 3.141592/180.0)) // faces are pretty similar in angle, smooth them
		{
			// reusing some functions and bindings
			WriteVertexLight(aVertexColorsWrite, nVertexIndex0, float4(normal1, 1.0));
			WriteVertexLight(aVertexColorsWrite, nVertexIndex1, float4(normal0, 1.0));
		}
	}
}