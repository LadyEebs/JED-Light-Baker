#include "Baking.hlsli"

RWStructuredBuffer<int4> aNormalsWrite : register(u0);

// Smooth contribution between two vertices
void SmoothPair(uint nVertexIndex0, uint nVertexIndex1)
{
    const float3 vertex0 = aVertices[nVertexIndex0].position.xyz;
    const float3 vertex1 = aVertices[nVertexIndex1].position.xyz;

    const float3 normal0 = aSurfaces[aVertices[nVertexIndex0].nSurfaceIndex].normal.xyz;
    const float3 normal1 = aSurfaces[aVertices[nVertexIndex1].nSurfaceIndex].normal.xyz;

    const float3 diff = vertex1 - vertex0;
    if (dot(diff, diff) < 1e-5 && dot(normal0, normal1) > g_levelInfo.normalSmoothCos)
    {
        int old;
        InterlockedAdd(aNormalsWrite[nVertexIndex0].x, int(normal1.x * 1024.0), old);
        InterlockedAdd(aNormalsWrite[nVertexIndex0].y, int(normal1.y * 1024.0), old);
        InterlockedAdd(aNormalsWrite[nVertexIndex0].z, int(normal1.z * 1024.0), old);
        InterlockedAdd(aNormalsWrite[nVertexIndex1].x, int(normal0.x * 1024.0), old);
        InterlockedAdd(aNormalsWrite[nVertexIndex1].y, int(normal0.y * 1024.0), old);
        InterlockedAdd(aNormalsWrite[nVertexIndex1].z, int(normal0.z * 1024.0), old);
    }
}

// Smooth contribution from neighbor vertex only
void SmoothAdjoin(uint nVertexIndex0, uint nVertexIndex1)
{
    const float3 vertex0 = aVertices[nVertexIndex0].position.xyz;
    const float3 vertex1 = aVertices[nVertexIndex1].position.xyz;

    const float3 normal0 = aSurfaces[aVertices[nVertexIndex0].nSurfaceIndex].normal.xyz;
    const float3 normal1 = aSurfaces[aVertices[nVertexIndex1].nSurfaceIndex].normal.xyz;

    const float3 diff = vertex1 - vertex0;
    if (dot(diff, diff) < 1e-5 && dot(normal0, normal1) > g_levelInfo.normalSmoothCos)
    {
        int old;
        InterlockedAdd(aNormalsWrite[nVertexIndex0].x, int(normal1.x * 1024.0), old);
        InterlockedAdd(aNormalsWrite[nVertexIndex0].y, int(normal1.y * 1024.0), old);
        InterlockedAdd(aNormalsWrite[nVertexIndex0].z, int(normal1.z * 1024.0), old);
    }
}

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const int nSectorIndex = dispatchThreadID.x;
    if (nSectorIndex >= g_levelInfo.nTotalSectors) return;

    const uint nFirstSurface0 = aSectors[nSectorIndex].nFirstSurface;
    const uint nLastSurface0 = nFirstSurface0 + aSectors[nSectorIndex].nNumSurfaces;

    // Compare within sector
    for (uint nSurfaceIndex0 = nFirstSurface0; nSurfaceIndex0 < nLastSurface0; ++nSurfaceIndex0)
    {
        const uint nFirstVertex0 = aSurfaces[nSurfaceIndex0].nFirstVertex;
        const uint nLastVertex0 = nFirstVertex0 + aSurfaces[nSurfaceIndex0].nNumVertices;

        for (uint nVertexIndex0 = nFirstVertex0; nVertexIndex0 < nLastVertex0; ++nVertexIndex0)
        {
            for (uint nSurfaceIndex1 = nSurfaceIndex0; nSurfaceIndex1 < nLastSurface0; ++nSurfaceIndex1)
            {
                const uint nFirstVertex1 = aSurfaces[nSurfaceIndex1].nFirstVertex;
                const uint nLastVertex1 = nFirstVertex1 + aSurfaces[nSurfaceIndex1].nNumVertices;

                const uint nStart = (nSurfaceIndex0 == nSurfaceIndex1) ? nVertexIndex0 + 1 : nFirstVertex1;
                for (uint nVertexIndex1 = nStart; nVertexIndex1 < nLastVertex1; ++nVertexIndex1)
                    SmoothPair(nVertexIndex0, nVertexIndex1);
            }

            // Compare with adjoined sector
            for (nSurfaceIndex1 = nFirstSurface0; nSurfaceIndex1 < nLastSurface0; ++nSurfaceIndex1)
            {
                int nAdjoinSector = aSurfaces[nSurfaceIndex1].nAdjoinSector;
                if (nAdjoinSector < 0) continue;

                const uint nFirstSurface1 = aSectors[nAdjoinSector].nFirstSurface;
                const uint nLastSurface1 = nFirstSurface1 + aSectors[nAdjoinSector].nNumSurfaces;

                for (uint nSurfaceIndex1Adj = nFirstSurface1; nSurfaceIndex1Adj < nLastSurface1; ++nSurfaceIndex1Adj)
                {
                    const uint nFirstVertex1 = aSurfaces[nSurfaceIndex1Adj].nFirstVertex;
                    const uint nLastVertex1 = nFirstVertex1 + aSurfaces[nSurfaceIndex1Adj].nNumVertices;

                    for (uint nVertexIndex1 = nFirstVertex1; nVertexIndex1 < nLastVertex1; ++nVertexIndex1)
                        SmoothAdjoin(nVertexIndex0, nVertexIndex1);
                }
            }
        }
    }
}
