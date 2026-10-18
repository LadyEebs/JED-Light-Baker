#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "GpuBuffer.h"
#include "Types.h"
#include "HWTypes.h"

// TODO: this is a monolithic class atm, can probably break it up
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
	BOOL m_bAllowLightCleave;
	BOOL m_bAllowSunLightCleave;
	BOOL m_bSkyColorFromMaterials;

	int m_nSkyEmissiveRayCount;
	int m_nIndirectRayCount;
	int m_nIndirectBounces;
	int m_nNormalSmoothingAngle;

	afx_msg void OnBnClickedBakeAll();
	afx_msg void OnBnClickedBakeSelected();
	afx_msg void OnBnClickedBakeVisibleLayers();
	afx_msg void OnCbnSelchangeComboRays();
	afx_msg void OnCbnSelchangeComboIndirectRays();
	afx_msg void OnBnClickedSunlightButton();
	afx_msg void OnBnClickedSkylightButton();
	afx_msg void OnBnClickedAnchorlightButton();
	afx_msg void OnBnClickedCleaveAllButton();
	afx_msg void OnBnClickedClearCachesButton();

private:
	// main entry point for baking process, everything below assumes flags etc are set
	bool BakeLighting(uint32_t nInitBakeFlags);

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
	bool BuildNormalAdjacency(std::vector<SSpatialVertex>& vertices, const std::vector<uint64_t>& adjoinEdges, std::vector<uint32_t>& offsets, std::vector<uint32_t>& indices);

	// helpers for shaders and buffers
	int CreateComputeShader(ID3D11Device* pDevice, ID3D11ComputeShader** pShader, UINT resourceID) const;
	int CreateConstantBuffer(ID3D11Device* pDevice, ID3D11Buffer** pConstantBuffer, int byteWidth) const;

	// updates the level info constant buffer for all passes
	void UpdateLevelInfo();
	template<typename T>
	bool UploadStructuredBuffer(CGpuBuffer& buffer, const std::vector<T>& data);

	// helper for dispatching a bake pass, Z is ignored since we're only doing 1d and 2d dispatches
	void DispatchBakePass(int nDispatchX, int nDispatchY, ID3D11ComputeShader* pShader, CGpuBuffer* pReadBuffer, CGpuBuffer* pWriteBuffer);
	
	// baking functions that actually call DispatchBakePass
	void BakeDirectLighting();
	void BakeIndirectLighting();
	void ComputeSmoothNormals();

	// downloads the results from the GPU and writes them back to the level
	void DownloadAndApplyToLevel();

	void SelectOrCreateLight(int nLightFlags, float intensity, const float3& rgb, float rgbIntensity);

	bool CleavePrepass(bool bWarnUser);
	void SplitLargeSurfaces();

	void FixupCogLinkedSurfaces();

private:
	// Jed
	IJED*      m_pJed;
	IJEDLevel* m_pJedLevel;
	IJEDLevel* m_pLastJedLevel;

	// D3D
	ID3D11Device*        m_pDeviceD3D;
	ID3D11DeviceContext* m_pDeviceContextD3D;

	CGpuBuffer m_selectionBitmaskBuffer;
	CGpuBuffer m_layerBitmaskBuffer;
	CGpuBuffer m_sectorBuffer;
	CGpuBuffer m_surfaceBuffer;
	CGpuBuffer m_vertexBuffer;
	CGpuBuffer m_lightBuffer;
	CGpuBuffer m_normalBuffer;
	CGpuBuffer m_colorLastResultBuffer;
	CGpuBuffer m_colorCurrResultBuffer;
	CGpuBuffer m_accumulationBuffer;
	CGpuBuffer m_normalNeighborOffsetsBuffer;
	CGpuBuffer m_normalNeighborIndicesBuffer;

	ID3D11ComputeShader* m_pBakeSunShader;
	ID3D11ComputeShader* m_pBakeDirectShader;
	ID3D11ComputeShader* m_pBakeSkyEmissiveShader;
	ID3D11ComputeShader* m_pBakeIndirectShader;
	ID3D11ComputeShader* m_pBakeAmbientShader;
	ID3D11ComputeShader* m_pGenNormalsShader;

	ID3D11Buffer* m_pLevelInfoConstants;

	// State
	int m_nSunLightIndex, m_nSkyLightIndex, m_nAnchorLightIndex;
	uint32_t m_nBakeFlags;
	std::vector<uint32_t> m_selectedSectorBitmask;

	// Resource counts, only valid during BakeLighting
	int m_nNumSectors, m_nNumQueuedSectors, m_nNumLights, m_nNumLayers;
	int m_nTotalSurfaces, m_nTotalVertices;

	// Resource caching
	std::unordered_map<std::wstring, SColormap> m_colormapCache;
	std::unordered_map<std::wstring, uint32_t>  m_materialColorCache;
};

template<typename T>
bool CLightBakerDlg::UploadStructuredBuffer(CGpuBuffer& buffer, const std::vector<T>& data)
{
	if (data.size() > (size_t)INT_MAX)
		return false;

	buffer.Release();
	const int nElements = std::max(1, (int)data.size());

	if (!buffer.Create(m_pDeviceD3D, m_pDeviceContextD3D, nElements, sizeof(T), DXGI_FORMAT_UNKNOWN, 0, 1, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED))
		return false;

	CGpuBufferMapping<T> mapped(&buffer, D3D11_MAP_WRITE);

	if (!mapped)
	{
		buffer.Release();
		return false;
	}

	if (data.empty())
		mapped[0] = T{};
	else
		std::memcpy(mapped.RawData(), data.data(), data.size() * sizeof(T));

	return true;
}

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
