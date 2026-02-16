#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "Resource.h"
#include "GpuBuffer.h"

// GPU mirrored structs
struct SSector
{
	uint32_t nFirstSurface;
	uint32_t nNumSurfaces;
	uint32_t nLayerIndex;
	uint32_t _padding;
	float4   center;
};

struct SSurface
{
	float4    albedo;
	float4    emissive;
	float4    normal;
	uint32_t  nFirstVertex;
	uint32_t  nNumVertices;
	int32_t   nAdjoinSector;
	uint32_t  nFlags;
};

struct SVertex
{
	uint32_t  nSectorIndex;
	uint32_t  nSurfaceIndex;
	uint32_t  nLocalSurfaceIndex;
	uint32_t  nLocalVertexIndex;
	float4    position;
};

struct SLight
{
	uint32_t  nFlags;
	int32_t   nSectorIndex;
	int32_t   nLayerIndex;
	float     range;
	float4    position;
	float4    color;
};

// Light bake configuration
enum ELightBakeFlags
{
	ELightBake_Lights             =   0x1,
	ELightBake_Sun                =   0x2,
	ELightBake_Sky                =   0x4,
	ELightBake_Emissive           =   0x8,
	ELightBake_Indirect           =  0x10,
	ELightBake_Selected           =  0x20,
	ELightBake_VisibleLayers      =  0x40,
	ELightBake_GammaCorrect       =  0x80,
	ELightBake_ExtraLightEmissive = 0x100,
	ELightBake_ToneMap            = 0x200,
	ELightBake_PhysicalFalloff    = 0x400,

	ELightBake_Direct = ELightBake_Lights | ELightBake_Sun | ELightBake_Sky | ELightBake_Emissive
};

// Gpu surface flags
enum ESurfaceFlags
{
	ESurface_IsSky         = 0x1,
	ESurface_IsVisible     = 0x2,
	ESurface_IsTranslucent = 0x4,
};

// Engine/editor sector flags
enum ESectorFlags
{
	// SED specific
	ESector_NoAmbientLightRGB = 0x20000000,
	ESector_NoAmbientLight    = 0x40000000
};

// Engine/editor adjoin flags
enum EAdjoinFlags
{
	EAdjoin_Visible     =        0x1,
	EAdjoin_BlocksLight = 0x80000000,
};

// Engine/editor face flags
enum EFaceFlags
{
	EFace_Translucent = 0x2,
};

// Engine/editor sky surface flags
enum ESkyFlags
{
	ESky_Horizon = 0x200,
	ESky_Ceiling = 0x400,
};

// Baker specific light flags
enum ELightFlags
{
	ELight_Sun    = 0x2,
	ELight_Sky    = 0x4,
	ELight_Anchor = 0x8,
};

// be careful of constant buffer padding here
struct SLevelInfo
{
	int32_t nSunLightIndex;
	int32_t nSkyLightIndex;
	int32_t nAnchorLightIndex;
	int32_t _padding0;

	int32_t nTotalSectors;
	int32_t nTotalSurfaces;
	int32_t nTotalVertices;
	int32_t nTotalLights;

	uint32_t nBakeFlags;
	int32_t  nSkyEmissiveRays;
	int32_t  nIndirectRays;
	float    normalSmoothCos;

	int32_t  _padding2[4];
};

// Minimal colormap support (for reading basic color and light table)
struct SColormapHeader
{
	char    magic[4]; // "CMP "
	int32_t nVersion;
	int32_t nFlags;
	float3  colorTint;
	int32_t _padding[10];
};

struct SColormap
{
	struct
	{
		uint8_t r, g, b;
	} colors[256];
	uint8_t nLightLevels[64 * 256];

	float3 GetColor(uint32_t nIndex, int nLightLevel) const;
};

// Minimal material support (for reading material fill colors)
struct SColorFormat
{
	uint32_t nColorMode = 0;
	uint32_t nBitsPerPixel = 0;
	uint32_t nRedBits = 0;
	uint32_t nGreenBits = 0;
	uint32_t nBlueBits = 0;
	uint32_t nRedShift = 0;
	uint32_t nGreenShift = 0;
	uint32_t nBlueShift = 0;
	uint32_t nRedDiff = 0;
	uint32_t nGreenDiff = 0;
	uint32_t nBlueDiff = 0;
	uint32_t nAlphaBits = 0;
	uint32_t nAlphaShift = 0;
	uint32_t nAlphaDiff = 0;
};

struct SMaterialFileHeader
{
	uint8_t      magic[4];
	uint32_t     nVersion = 0;
	uint32_t     nType = 0;
	uint32_t     nRecordCount = 0;
	uint32_t     nTextureCount = 0;
	SColorFormat colorFormat;
};

struct SMaterialRecordHeader
{
	uint32_t nTextureType = 0;
	uint32_t nFillColor = 0;
	uint32_t nUnknown1 = 0;
	uint32_t nUnknown2 = 0;
	uint32_t nUnknown3 = 0;
	uint32_t nUnknown4 = 0;
};

struct SMaterialRecordHeaderExt
{
	uint32_t nUnknown = 0;
	uint32_t nHeight = 0;
	uint32_t nTransparentColor = 0;
	uint32_t nTextureIndex = 0;
};

// todo: this is a monolithic class atm, can probably break it up
class CLightBakerDlg
	: public CDialogEx
{
public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LIGHTBAKER_DLG };
#endif

	CLightBakerDlg(IJED* pJed, CWnd* pParent = nullptr);
	~CLightBakerDlg();

	bool CreateDeviceD3D();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

public:
	// MFC GUI stuff
	CComboBox m_skyEmissionRayCombo;
	CComboBox m_indirectRaysCombo;

	BOOL m_bBakePointLights;
	BOOL m_bBakeSunLight;
	BOOL m_bBakeSkyLight;
	BOOL m_bBakeEmissiveLight;
	BOOL m_bBakeIndirectLight;
	BOOL m_bGammaCorrect;
	BOOL m_bExtraLightEmissive;
	BOOL m_bPhysicalFalloff;
	BOOL m_bToneMap;

	int m_nSkyEmissiveRayCount;
	int m_nIndirectRayCount;
	int m_nIndirectBounces;
	int m_nNormalSmoothingAngle;

	afx_msg void OnBnClickedBakeAll();
	afx_msg void OnBnClickedBakeSelected();
	afx_msg void OnBnClickedBakeVisibleLayers();
	afx_msg void OnCbnSelchangeComboRays();
	afx_msg void OnCbnSelchangeComboIndirectRays();

private:
	// main entry point for baking process, everything below assumes flags etc are set
	bool BakeLighting(uint32_t nInitBakeFlags);

	// Jed helpers/utils (todo: move me)
	bool       QuerySurfaceVisible(int nSectorIndex, int nSurfaceIndex) const;
	float3     QuerySurfaceNormal(int nSectorIndex, int nSurfaceIndex) const;
	float3     QuerySurfaceVertex(int nSectorIndex, int nSurfaceIndex, int nVertexIndex) const;

	// fetches the level header and preloads the master cmp if there is one
	void       PreloadMasterCMP();

	// attempt to load and cache a colormap, if the filename was already loaded, the cached result is returned
	SColormap* LoadColormap(const wchar_t* sFileName);

	// attempt to load and cache a material, if the filename was already loaded, the cached result is returned
	uint32_t   LoadMaterialFillColor(const wchar_t* sFileName, const SColormap* pColormap);

	// load an embedded resource (usually shader blobs)
	bool LoadEmbeddedShader(UINT resourceID, const void** data, DWORD* size) const;

	// GPU buffer management, creates and frees various buffers
	void AllocateBuffers();
	void FreeBuffers();

	// builds the scene for the GPU
	void BuildSelectionBitmask();
	void BuildLayerBitmask();
	void BuildLights();
	void BuildGeometry();

	// helpers for shaders and buffers
	int CreateComputeShader(ID3D11Device* pDevice, ID3D11ComputeShader** pShader, UINT resourceID) const;
	int CreateConstantBuffer(ID3D11Device* pDevice, ID3D11Buffer** pConstantBuffer, int byteWidth) const;

	// updates the level info constant buffer for all passes
	void UpdateLevelInfo();

	// helper for dispatching a bake pass, Z is ignored since we're only doing 1d and 2d dispatches
	void DispatchBakePass(int nDispatchX, int nDispatchY, ID3D11ComputeShader* pShader, CGpuBuffer* pReadBuffer, CGpuBuffer* pWriteBuffer);
	
	// baking functions that actually call DispatchBakePass
	void BakeDirectLighting();
	void BakeIndirectLighting();
	void ComputeSmoothNormals();

	// downloads the results from the GPU and writes them back to the level
	void DownloadAndApplyToLevel();

private:
	// Jed
	IJED*      m_pJed;
	IJEDLevel* m_pJedLevel;
	IJEDLevel* m_pLastJedLevel;

	// D3D
	ID3D11Device*        m_pDeviceD3D;
	ID3D11DeviceContext* m_pDeviceContextD3D;

	CGpuBuffer* m_pSelectionBitmaskBuffer;
	CGpuBuffer* m_pLayerBitmaskBuffer;
	CGpuBuffer* m_pSectorBuffer;
	CGpuBuffer* m_pSurfaceBuffer;
	CGpuBuffer* m_pVertexBuffer;
	CGpuBuffer* m_pLightBuffer;
	CGpuBuffer* m_pNormalBuffer;
	CGpuBuffer* m_pColorLastResultBuffer;
	CGpuBuffer* m_pColorCurrResultBuffer;
	CGpuBuffer* m_pAccumulationBuffer;

	ID3D11ComputeShader* m_pBakeSunShader;
	ID3D11ComputeShader* m_pBakeDirectShader;
	ID3D11ComputeShader* m_pBakeSkyEmissiveShader;
	ID3D11ComputeShader* m_pBakeIndirectShader;
	ID3D11ComputeShader* m_pGenNormalsShader;

	ID3D11Buffer* m_pLevelInfoConstants;

	// State
	int m_nSunLightIndex, m_nSkyLightIndex, m_nAnchorLightIndex;
	uint32_t m_nBakeFlags;

	// Resource counts, only valid during BakeLighting
	int m_nNumSectors, m_nNumQueuedSectors, m_nNumLights, m_nNumLayers;
	int m_nTotalSurfaces, m_nTotalVertices;

	// Resource caching
	std::unordered_map<std::wstring, SColormap> m_colormapCache;
	std::unordered_map<std::wstring, uint32_t>  m_materialColorCache;
};

class CLightBakerApp
	: public CWinApp
{
public:
	CLightBakerApp();

public:
	virtual BOOL InitInstance() override;
	virtual int  ExitInstance() override;

	DECLARE_MESSAGE_MAP()
};
