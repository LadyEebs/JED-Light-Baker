#include "pch.h"
#include "framework.h"
#include "Light Baker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

float3 SColormap::GetColor(uint32_t nIndex, int nLightLevel) const
{
	if (nLightLevel >= 0)
		nIndex = nLightLevels[nLightLevel * 256 + nIndex];

	float3 fillColor;
	fillColor.x = (float)colors[nIndex].r / 255.0f;
	fillColor.y = (float)colors[nIndex].g / 255.0f;
	fillColor.z = (float)colors[nIndex].b / 255.0f;
	return fillColor;
}

template <typename... Args>
void PrintMessage(IJED* pJed, uint32_t nType, const char* sFmt, Args&&... args)
{
	char msg[256];
	sprintf_s(msg, 256, sFmt, std::forward<Args>(args)...);
	pJed->PanMessage(nType, msg);
}

CLightBakerApp theApp;
static CLightBakerDlg* g_pLightBaker = nullptr;

BEGIN_MESSAGE_MAP(CLightBakerApp, CWinApp)
END_MESSAGE_MAP()

CLightBakerApp::CLightBakerApp()
{
}

BOOL CLightBakerApp::InitInstance()
{
	CWinApp::InitInstance();
	return TRUE;
}

int CLightBakerApp::ExitInstance()
{
	if (g_pLightBaker)
	{
		if (g_pLightBaker->GetSafeHwnd())
			g_pLightBaker->DestroyWindow();
		delete g_pLightBaker;
		g_pLightBaker = nullptr;
	}
	return CWinApp::ExitInstance();
}

CLightBakerDlg::CLightBakerDlg(IJED* pJed, CWnd* pParent)
	: CDialogEx(IDD_LIGHTBAKER_DLG, pParent)
	, m_pJed(pJed)
	, m_pJedLevel(nullptr)
	, m_pLastJedLevel(nullptr)
	, m_pDeviceD3D(nullptr)
	, m_pDeviceContextD3D(nullptr)
	, m_pSelectionBitmaskBuffer(nullptr)
	, m_pLayerBitmaskBuffer(nullptr)
	, m_pSectorBuffer(nullptr)
	, m_pSurfaceBuffer(nullptr)
	, m_pLightBuffer(nullptr)
	, m_pVertexBuffer(nullptr)
	, m_pAccumulationBuffer(nullptr)
	, m_pColorCurrResultBuffer(nullptr)
	, m_pColorLastResultBuffer(nullptr)
	, m_pBakeSunShader(nullptr)
	, m_pBakeDirectShader(nullptr)
	, m_pBakeSkyEmissiveShader(nullptr)
	, m_pBakeIndirectShader(nullptr)
	, m_pLevelInfoConstants(nullptr)
	, m_bBakePointLights(TRUE)
	, m_bBakeSunLight(TRUE)
	, m_bBakeSkyLight(TRUE)
	, m_bBakeEmissiveLight(TRUE)
	, m_bBakeIndirectLight(TRUE)
	, m_bGammaCorrect(TRUE)
	, m_bExtraLightEmissive(FALSE)
	, m_bPhysicalFalloff(TRUE)
	, m_bToneMap(FALSE)
	, m_nSkyEmissiveRayCount(256)
	, m_nIndirectRayCount(256)
	, m_nIndirectBounces(3)
	, m_nSunLightIndex(-1)
	, m_nSkyLightIndex(-1)
	, m_nAnchorLightIndex(-1)
	, m_nBakeFlags(0)
	, m_nNumSectors(0)
	, m_nNumQueuedSectors(0)
	, m_nNumLights(0)
	, m_nNumLayers(0)
	, m_nTotalSurfaces(0)
	, m_nTotalVertices(0)
{
}

CLightBakerDlg::~CLightBakerDlg()
{
	if (m_pBakeSunShader)
		m_pBakeSunShader->Release();
	m_pBakeSunShader = nullptr;

	if (m_pBakeDirectShader)
		m_pBakeDirectShader->Release();
	m_pBakeDirectShader = nullptr;

	if (m_pBakeIndirectShader)
		m_pBakeIndirectShader->Release();
	m_pBakeIndirectShader = nullptr;

	if (m_pBakeIndirectShader)
		m_pBakeIndirectShader->Release();
	m_pBakeIndirectShader = nullptr;

	if (m_pLevelInfoConstants)
		m_pLevelInfoConstants->Release();
	m_pLevelInfoConstants = nullptr;

	if (m_pDeviceContextD3D)
		m_pDeviceContextD3D->Release();
	m_pDeviceContextD3D = nullptr;

	if (m_pDeviceD3D)
		m_pDeviceD3D->Release();
	m_pDeviceD3D = nullptr;
}

bool CLightBakerDlg::CreateDeviceD3D()
{
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_1 };
	int feature_level = 0;
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &m_pDeviceD3D, (D3D_FEATURE_LEVEL*)&feature_level, &m_pDeviceContextD3D);
	if (FAILED(hr))
	{
		PrintMessage(m_pJed, msg_error, "Failed to create D3D11 device.");
		return false;
	}

	if (!CreateComputeShader(m_pDeviceD3D, &m_pBakeSunShader, IDR_BAKE_SUN_CSO))
	{
		PrintMessage(m_pJed, msg_error, "Failed to compile sun shader.");
		return false;
	}

	if (!CreateComputeShader(m_pDeviceD3D, &m_pBakeDirectShader, IDR_BAKE_DIRECT_CSO))
	{
		PrintMessage(m_pJed, msg_error, "Failed to compile light shader.");
		return false;
	}

	if (!CreateComputeShader(m_pDeviceD3D, &m_pBakeSkyEmissiveShader, IDR_BAKE_SKY_EMISSIVE_CSO))
	{
		PrintMessage(m_pJed, msg_error, "Failed to compile sky/emissive shader.");
		return false;
	}

	if (!CreateComputeShader(m_pDeviceD3D, &m_pBakeIndirectShader, IDR_BAKE_INDIRECT_CSO))
	{
		PrintMessage(m_pJed, msg_error, "Failed to compile indirect shader.");
		return false;
	}

	if(!CreateConstantBuffer(m_pDeviceD3D, &m_pLevelInfoConstants, sizeof(SLevelInfo)))
	{
		PrintMessage(m_pJed, msg_error, "Failed to created constant buffer.");
		return false;
	}

	return true;
}

void CLightBakerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_POINT, m_bBakePointLights);
	DDX_Check(pDX, IDC_CHECK_SUN, m_bBakeSunLight);
	DDX_Check(pDX, IDC_CHECK_SKY, m_bBakeSkyLight);
	DDX_Check(pDX, IDC_CHECK_EMISSIVE, m_bBakeEmissiveLight);
	DDX_Check(pDX, IDC_CHECK_INDIRECT, m_bBakeIndirectLight);
	DDX_Check(pDX, IDC_CHECK_GAMMA_CORRECT, m_bGammaCorrect);
	DDX_Check(pDX, IDC_CHECK_EXTRA_LIGHT_EMISSIVE, m_bExtraLightEmissive);
	DDX_Check(pDX, IDC_CHECK_PHYSICAL_FALLOFF, m_bPhysicalFalloff);
	DDX_Check(pDX, IDC_CHECK_TONE_MAP, m_bToneMap);

	DDX_Text(pDX, IDC_RAY_EDIT, m_nSkyEmissiveRayCount);
	DDX_Text(pDX, IDC_INDIRECT_RAY_EDIT, m_nIndirectRayCount);
	DDX_Text(pDX, IDC_BOUNCES_EDIT, m_nIndirectBounces);

	DDV_MinMaxInt(pDX, m_nSkyEmissiveRayCount, 16, 1024);	
	DDV_MinMaxInt(pDX, m_nIndirectRayCount, 16, 1024);	
	DDV_MinMaxInt(pDX, m_nIndirectBounces, 1, 10);
}

BOOL CLightBakerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	ModifyStyle(WS_THICKFRAME, 0, 0);
	SetWindowPos(&wndTop, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	m_nIndirectRayCount = 1024;
	m_nSkyEmissiveRayCount = 1024;
	m_nIndirectBounces = 3;
	UpdateData(FALSE);

	CSpinButtonCtrl* pSkyEmissiveRaySpin = (CSpinButtonCtrl*)GetDlgItem(IDC_RAY_SPIN);
	if (pSkyEmissiveRaySpin)
	{
		pSkyEmissiveRaySpin->SetRange(16, 4096);
		pSkyEmissiveRaySpin->SetPos(m_nSkyEmissiveRayCount);
	}

	CSpinButtonCtrl* pIndirectRaySpin = (CSpinButtonCtrl*)GetDlgItem(IDC_INDIRECT_RAY_SPIN);
	if (pIndirectRaySpin)
	{
		pIndirectRaySpin->SetRange(16, 4096);
		pIndirectRaySpin->SetPos(m_nIndirectRayCount);
	}

	CSpinButtonCtrl* pIndirectBouncesSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_BOUNCES_SPIN);
	if (pIndirectBouncesSpin)
	{
		pIndirectBouncesSpin->SetRange(1, 5);
		pIndirectBouncesSpin->SetPos(m_nIndirectBounces);
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CLightBakerDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_BAKE_ALL, &CLightBakerDlg::OnBnClickedBakeAll)
	ON_BN_CLICKED(IDC_BUTTON_BAKE_SEL, &CLightBakerDlg::OnBnClickedBakeSelected)
	ON_BN_CLICKED(IDC_BUTTON_BAKE_LAYERS, &CLightBakerDlg::OnBnClickedBakeVisibleLayers)
END_MESSAGE_MAP()

afx_msg void CLightBakerDlg::OnBnClickedBakeAll()
{
	UpdateData(TRUE);
	BakeLighting(0);
}

afx_msg void CLightBakerDlg::OnBnClickedBakeSelected()
{
	UpdateData(TRUE);
	BakeLighting(ELightBake_Selected);
}

afx_msg void CLightBakerDlg::OnBnClickedBakeVisibleLayers()
{
	UpdateData(TRUE);
	BakeLighting(ELightBake_VisibleLayers);
}

bool CLightBakerDlg::IsSurfaceVisible(int nSectorIndex, int nSurfaceIndex) const
{
	tjedsurfacerec surface;
	memset(&surface, 0, sizeof(tjedsurfacerec));
	m_pJedLevel->GetSurface(nSectorIndex, nSurfaceIndex, &surface, sf_Material | sf_adjoin | sf_adjoinflags | sf_geo | sf_SurfFlags | sf_FaceFlags);

	// ignore geo mode 0 (not visible) and surfaces with no material
	if (surface.geo == 0 || !surface.material)
		return false;

	// ignore adjoins that block
	if (surface.adjoinsc >= 0 && !(surface.adjoinflags & EAdjoin_Visible))
		return false;

	// ignore skies
	if((surface.surfflags & ESky_Horizon) != 0 || (surface.surfflags & ESky_Ceiling) != 0)
		return false;

	return true;
}

float3 CLightBakerDlg::GetSurfaceNormal(int nSectorIndex, int nSurfaceIndex) const
{
	tjedvector dnormal;
	m_pJedLevel->GetSurfaceNormal(nSectorIndex, nSurfaceIndex, &dnormal);
	return { (float)dnormal.v.s1.x, (float)dnormal.v.s1.y, (float)dnormal.v.s1.z };
}

float3 CLightBakerDlg::GetSurfaceVertex(int nSectorIndex, int nSurfaceIndex, int nVertexIndex) const
{
	// Get sector vertex index from surface vertex index
	int nVertex = m_pJedLevel->SurfaceGetVertexNum(nSectorIndex, nSurfaceIndex, nVertexIndex);

	tjedvector vertex;
	m_pJedLevel->SectorGetVertex(nSectorIndex, nVertex, &vertex.v.s1.x, &vertex.v.s1.y, &vertex.v.s1.z);

	return { (float)vertex.v.s1.x, (float)vertex.v.s1.y, (float)vertex.v.s1.z };
}

bool CLightBakerDlg::BakeLighting(uint32_t nInitBakeFlags)
{
	m_nBakeFlags = nInitBakeFlags;
	m_nBakeFlags |= m_bBakePointLights ? ELightBake_Lights : 0;
	m_nBakeFlags |= m_bBakeSunLight ? ELightBake_Sun : 0;
	m_nBakeFlags |= m_bBakeSkyLight ? ELightBake_Sky : 0;
	m_nBakeFlags |= m_bBakeEmissiveLight ? ELightBake_Emissive : 0;
	m_nBakeFlags |= m_bBakeIndirectLight ? ELightBake_Indirect : 0;
	m_nBakeFlags |= m_bGammaCorrect ? ELightBake_GammaCorrect : 0;
	m_nBakeFlags |= m_bExtraLightEmissive ? ELightBake_ExtraLightEmissive : 0;
	m_nBakeFlags |= m_bPhysicalFalloff ? ELightBake_PhysicalFalloff : 0;
	m_nBakeFlags |= m_bToneMap ? ELightBake_ToneMap : 0;
	
	m_pJedLevel = m_pJed->GetLevel();
	if (!m_pJedLevel)
		return false;

	if (m_pLastJedLevel != m_pJedLevel)
	{
		// clear resource caches on any level changes so we can reload mising mats
		m_materialColorCache.clear();
		m_colormapCache.clear();
	}
	m_pLastJedLevel = m_pJedLevel;

	m_nNumLayers = m_pJedLevel->NLayers();

	m_nNumSectors = m_pJedLevel->NSectors();
	m_nNumQueuedSectors = (m_nBakeFlags & ELightBake_Selected) ? m_pJed->GetNMultiselected(MM_SC) : m_nNumSectors;
	if (!m_nNumQueuedSectors)
	{
		PrintMessage(m_pJed, msg_info, "No sectors selected for light baking.");
		return false;
	}

	m_nNumLights = m_pJedLevel->NLights();
	if (!m_nNumLights)
	{
		PrintMessage(m_pJed, msg_info, "No lights available for baking.");
		return false;
	}

	PrintMessage(m_pJed, msg_info, "%u Sectors in Level queued for baking.", m_nNumQueuedSectors);

	const auto startTime = std::chrono::high_resolution_clock::now();

	// we need to know how many vertices we have in total in the level
	m_nTotalSurfaces = 0;
	m_nTotalVertices = 0;
	for (int nSectorIndex = 0; nSectorIndex < m_nNumSectors; ++nSectorIndex)
	{
		const int nNumSurfaces = m_pJedLevel->SectorNSurfaces(nSectorIndex);
		m_nTotalSurfaces += nNumSurfaces;
		for (int nSurfaceIndex = 0; nSurfaceIndex < nNumSurfaces; ++nSurfaceIndex)
		{
			const int nNumVertices = m_pJedLevel->SurfaceNVertices(nSectorIndex, nSurfaceIndex);
			m_nTotalVertices += nNumVertices;
		}
	}
	PrintMessage(m_pJed, msg_info, "%u Vertices in Level queued for baking.", m_nTotalVertices);

	// preload the master colormap up front
	PreloadMasterCMP();

	AllocateBuffers();
	BuildSelectionBitmask();
	BuildLayerBitmask();
	BuildLights();
	BuildGeometry();

	// remove flags if no sun/sky were found
	if (m_nSunLightIndex < 0)
		m_nBakeFlags &= ~ELightBake_Sun;

	if (m_nSkyLightIndex < 0)
		m_nBakeFlags &= ~ELightBake_Sky;

	if (!m_nNumLights)
		m_nBakeFlags &= ~ELightBake_Lights;

	// only update level info after updating the bake flags because we write them
	UpdateLevelInfo();

	if (m_nBakeFlags & ELightBake_Direct)
		BakeDirectLighting();

	// - N Bounce passes (ping pong for readback between bounces, atomic add)
	if (m_nBakeFlags & ELightBake_Indirect)
		BakeIndirectLighting();
	
	m_pDeviceContextD3D->Flush();

	DownloadAndApplyToLevel();
	FreeBuffers();

	const auto endTime = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<float> deltaTime = endTime - startTime;

	PrintMessage(m_pJed, msg_info, "Finished light bake in %g seconds.", deltaTime.count());

	return true;
}

void CLightBakerDlg::PreloadMasterCMP()
{
	tlevelheader levelHeader;
	memset(&levelHeader, 0, sizeof(tlevelheader));
	m_pJedLevel->GetLevelHeader(&levelHeader, lh_all);
	LoadColormap(levelHeader.mastercmp);
}

bool CLightBakerDlg::LoadEmbeddedShader(UINT resourceID, const void** data, DWORD* size) const
{
	HRSRC hRes = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(resourceID), RT_RCDATA);
	if (!hRes)
		return false;

	HGLOBAL hData = LoadResource(AfxGetResourceHandle(), hRes);
	if (!hData)
		return false;

	*data = LockResource(hData);
	*size = SizeofResource(AfxGetResourceHandle(), hRes);
	return (*data && *size);
}

void CLightBakerDlg::AllocateBuffers()
{
	// todo: move the creation of the buffer data out of the constructor and check for failures
	m_pSelectionBitmaskBuffer = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, (m_nNumSectors + 31) / 32, sizeof(uint32_t), DXGI_FORMAT_R32_UINT, 0, 0, (D3D11_RESOURCE_MISC_FLAG)0);
	m_pLayerBitmaskBuffer     = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, (m_nNumLayers + 31) / 32, sizeof(uint32_t), DXGI_FORMAT_R32_UINT, 0, 0, (D3D11_RESOURCE_MISC_FLAG)0);
	m_pSectorBuffer           = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, m_nNumSectors,             sizeof(SSector), DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	m_pSurfaceBuffer          = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, m_nTotalSurfaces,          sizeof(SSurface), DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	m_pVertexBuffer           = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, m_nTotalVertices,          sizeof(SVertex), DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	m_pLightBuffer            = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, m_nNumLights,              sizeof(SLight), DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	m_pColorLastResultBuffer  = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, m_nTotalVertices,          sizeof(float4), DXGI_FORMAT_UNKNOWN, 1, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	m_pColorCurrResultBuffer  = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, m_nTotalVertices,          sizeof(float4), DXGI_FORMAT_UNKNOWN, 1, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	m_pAccumulationBuffer     = new CGpuBuffer(m_pDeviceD3D, m_pDeviceContextD3D, m_nTotalVertices,          sizeof(float4), DXGI_FORMAT_UNKNOWN, 1, 1, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);

	// clear the color buffers before we render anything to them
	m_pColorLastResultBuffer->ClearUAV();
	m_pColorCurrResultBuffer->ClearUAV();
	m_pAccumulationBuffer->ClearUAV();
}

void CLightBakerDlg::FreeBuffers()
{
	delete m_pSelectionBitmaskBuffer;
	delete m_pLayerBitmaskBuffer;
	delete m_pSectorBuffer;
	delete m_pSurfaceBuffer;
	delete m_pVertexBuffer;
	delete m_pLightBuffer;
	delete m_pColorLastResultBuffer;
	delete m_pColorCurrResultBuffer;
	delete m_pAccumulationBuffer;
}

void CLightBakerDlg::BuildSelectionBitmask()
{
	uint32_t* paSelectionMask = m_pSelectionBitmaskBuffer->Map<uint32_t>(D3D11_MAP_WRITE);
	memset(paSelectionMask, 0, sizeof(uint32_t) * ((m_nNumSectors + 31) / 32));
	for (int nQueuedSectorIndex = 0; nQueuedSectorIndex < m_nNumQueuedSectors; ++nQueuedSectorIndex)
	{
		const int nSectorIndex = (m_nBakeFlags & ELightBake_Selected) ? m_pJed->GetSelectedSC(nQueuedSectorIndex) : nQueuedSectorIndex;
		const uint32_t nBucketIndex = nSectorIndex / 32;
		const uint32_t nBucketPlace = nSectorIndex % 32;
		paSelectionMask[nBucketIndex] |= 1u << nBucketPlace;
	}
	m_pSelectionBitmaskBuffer->Unmap();
}

void CLightBakerDlg::BuildLayerBitmask()
{
	uint32_t* paLayerMask = m_pLayerBitmaskBuffer->Map<uint32_t>(D3D11_MAP_WRITE);
	memset(paLayerMask, 0, sizeof(uint32_t) * ((m_nNumLayers + 31) / 32));
	for (int nLayerIndex = 0; nLayerIndex < m_nNumLayers; ++nLayerIndex)
	{
		if (!(m_nBakeFlags & ELightBake_VisibleLayers) || m_pJed->IsLayerVisible(nLayerIndex))
		{
			const uint32_t nBucketIndex = nLayerIndex / 32;
			const uint32_t nBucketPlace = nLayerIndex % 32;
			paLayerMask[nBucketIndex] |= 1u << nBucketPlace;
		}
	}
	m_pLayerBitmaskBuffer->Unmap();
}

void CLightBakerDlg::BuildLights()
{
	SLight* paLights = m_pLightBuffer->Map<SLight>(D3D11_MAP_WRITE);

	// build lights
	m_nSunLightIndex = -1;
	m_nSkyLightIndex = -1;
	m_nAnchorLightIndex = -1;
	for (int nLightIndex = 0; nLightIndex < m_nNumLights; ++nLightIndex)
	{
		tjedlightrec light;
		memset(&light, 0, sizeof(tjedlightrec));
		m_pJedLevel->GetLight(nLightIndex, &light, lt_all);

		SLight* pLight = &paLights[nLightIndex];
		pLight->nSectorIndex = m_pJed->FindSectorForXYZ(light.x, light.y, light.z);
		pLight->nFlags = light.flags;
		pLight->range = (float)light.range;
		pLight->nLayerIndex = light.layer;
		pLight->color = { (float)(light.r * light.rgbintensity), (float)(light.g * light.rgbintensity), (float)(light.b * light.rgbintensity), (float)light.intensity };
		pLight->position = { (float)light.x, (float)light.y, (float)light.z, 1.0f };

		if (light.flags & ELight_Sun) // todo: more than 1?
			m_nSunLightIndex = nLightIndex;
		else if (light.flags & ELight_Sky)
			m_nSkyLightIndex = nLightIndex;
		else if (light.flags & ELight_Anchor)
			m_nAnchorLightIndex = nLightIndex;

		if (m_nBakeFlags & ELightBake_GammaCorrect)
		{
			light.r = ToLinear(light.r);
			light.g = ToLinear(light.g);
			light.b = ToLinear(light.b);
		}
	}

	m_pLightBuffer->Unmap();
}

void CLightBakerDlg::BuildGeometry()
{
	SSector*  paSectors  = m_pSectorBuffer->Map<SSector>(D3D11_MAP_WRITE);
	SSurface* paSurfaces = m_pSurfaceBuffer->Map<SSurface>(D3D11_MAP_WRITE);
	SVertex*  paVertices = m_pVertexBuffer->Map<SVertex>(D3D11_MAP_WRITE);

	uint32_t nSurfaceOffset = 0;
	uint32_t nVertexOffset = 0;
	for (int nSectorIndex = 0; nSectorIndex < m_nNumSectors; ++nSectorIndex)
	{
		const int nNumSurfaces = m_pJedLevel->SectorNSurfaces(nSectorIndex);

		tjedsectorrec sector;
		memset(&sector, 0, sizeof(tjedsectorrec));
		m_pJedLevel->GetSector(nSectorIndex, &sector, s_all);
		SColormap* pColormap = LoadColormap(sector.colormap);

		SSector* pSector = &paSectors[nSectorIndex];
		pSector->nFirstSurface = nSurfaceOffset;
		pSector->nNumSurfaces = nNumSurfaces;
		pSector->nLayerIndex = sector.layer;

		for (int nSurfaceIndex = 0; nSurfaceIndex < nNumSurfaces; ++nSurfaceIndex)
		{
			const int nNumVertices = m_pJedLevel->SurfaceNVertices(nSectorIndex, nSurfaceIndex);

			SSurface* pSurface = &paSurfaces[nSurfaceOffset];
			pSurface->nFirstVertex = nVertexOffset;
			pSurface->nNumVertices = nNumVertices;
			for (int nVertexIndex = 0; nVertexIndex < nNumVertices; ++nVertexIndex)
			{
				float3 vertex = GetSurfaceVertex(nSectorIndex, nSurfaceIndex, nVertexIndex);

				SVertex* pVertex = &paVertices[nVertexOffset++];
				pVertex->position.x = vertex.x;
				pVertex->position.y = vertex.y;
				pVertex->position.z = vertex.z;
				pVertex->nSectorIndex = nSectorIndex;
				pVertex->nSurfaceIndex = nSurfaceOffset;
			}
			++nSurfaceOffset;

			tjedsurfacerec surface;
			memset(&surface, 0, sizeof(tjedsurfacerec));
			m_pJedLevel->GetSurface(nSectorIndex, nSurfaceIndex, &surface, sf_all);

			float3 albedo = { 0.5f,0.5f,0.5f };
			float3 emissive = { 0,0,0 };
			if (pColormap)
			{
				int emissiveLightLevel = 0;

				// use the light level so we can gracefully go back to 0 and still use regular emissives
				if (m_nBakeFlags & ELightBake_ExtraLightEmissive)
					emissiveLightLevel = int(std::min(std::max((float)surface.extralight, 0.0f), 1.0f) * 63.0f);

				uint32_t nFillColor = LoadMaterialFillColor(surface.material, pColormap);
				albedo = pColormap->GetColor(nFillColor, -1);
				emissive = pColormap->GetColor(nFillColor, emissiveLightLevel);
				if (m_nBakeFlags & ELightBake_GammaCorrect)
				{
					albedo = ToLinear(albedo);
					emissive = ToLinear(emissive);
				}

				// allow the extra light to increase the intensity
				if (m_nBakeFlags & ELightBake_ExtraLightEmissive)
				{
					emissive.x = std::max(emissive.x, (float)surface.extralight * emissive.x);
					emissive.y = std::max(emissive.y, (float)surface.extralight * emissive.y);
					emissive.z = std::max(emissive.z, (float)surface.extralight * emissive.z);
				}
			}
			pSurface->albedo.x = albedo.x;
			pSurface->albedo.y = albedo.y;
			pSurface->albedo.z = albedo.z;
			pSurface->albedo.w = (albedo.x + albedo.y + albedo.z) / 3.0f;

			pSurface->emissive.x = emissive.x;
			pSurface->emissive.y = emissive.y;
			pSurface->emissive.z = emissive.z;
			pSurface->emissive.w = (emissive.x + emissive.y + emissive.z) / 3.0f;

			float3 normal = GetSurfaceNormal(nSectorIndex, nSurfaceIndex);
			pSurface->normal.x = normal.x;
			pSurface->normal.y = normal.y;
			pSurface->normal.z = normal.z;

			// if a surface is set to block light, then ignore the adjoin value
			pSurface->nAdjoinSector = (surface.adjoinflags & EAdjoin_BlocksLight) ? -1 : surface.adjoinsc;
			pSurface->nFlags = IsSurfaceVisible(nSectorIndex, nSurfaceIndex) ? ESurface_IsVisible : 0;
			if (((surface.surfflags & ESky_Horizon) != 0) || ((surface.surfflags & ESky_Ceiling) != 0))
				pSurface->nFlags |= ESurface_IsSky;
			if (surface.faceflags & EFace_Translucent)
				pSurface->nFlags |= ESurface_IsTranslucent;
		}
	}

	m_pSectorBuffer->Unmap();
	m_pSurfaceBuffer->Unmap();
	m_pVertexBuffer->Unmap();
}

int CLightBakerDlg::CreateComputeShader(ID3D11Device* pDevice, ID3D11ComputeShader** pShader, UINT resourceID) const
{
	const void* shaderData = nullptr;
	DWORD shaderSize = 0;

	if (LoadEmbeddedShader(resourceID, &shaderData, &shaderSize))
	{
		HRESULT res = pDevice->CreateComputeShader((void*)shaderData, shaderSize, NULL, pShader);
		return res == S_OK;
	}
	return false;
}

int CLightBakerDlg::CreateConstantBuffer(ID3D11Device* pDevice, ID3D11Buffer** pConstantBuffer, int byteWidth) const
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = byteWidth;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	HRESULT res = pDevice->CreateBuffer(&desc, nullptr, pConstantBuffer);
	return res == S_OK;
}

void CLightBakerDlg::UpdateLevelInfo()
{
	// run the bake
	SLevelInfo levelInfo;
	levelInfo.nTotalSectors     = m_nNumSectors;
	levelInfo.nTotalSurfaces    = m_nTotalSurfaces;
	levelInfo.nTotalVertices    = m_nTotalVertices;
	levelInfo.nTotalLights      = m_nNumLights;
	levelInfo.nBakeFlags        = m_nBakeFlags;
	levelInfo.nSkyEmissiveRays  = m_nSkyEmissiveRayCount;
	levelInfo.nIndirectRays     = m_nIndirectRayCount;
	levelInfo.nSunLightIndex    = m_nSunLightIndex;
	levelInfo.nSkyLightIndex    = m_nSkyLightIndex;
	levelInfo.nAnchorLightIndex = m_nAnchorLightIndex;
	m_pDeviceContextD3D->UpdateSubresource(m_pLevelInfoConstants, 0, nullptr, &levelInfo, 0, 0);
}

void CLightBakerDlg::DispatchBakePass(int nDispatchX, int nDispatchY, ID3D11ComputeShader* pShader, CGpuBuffer* pReadBuffer, CGpuBuffer* pWriteBuffer)
{
	ID3D11ShaderResourceView* apShaderResources[] =
	{
		m_pSelectionBitmaskBuffer->GetSRV(),
		m_pLayerBitmaskBuffer->GetSRV(),
		m_pSectorBuffer->GetSRV(),
		m_pSurfaceBuffer->GetSRV(),
		m_pVertexBuffer->GetSRV(),
		m_pLightBuffer->GetSRV(),
		pReadBuffer->GetSRV()
	};
	
	ID3D11UnorderedAccessView* apUnorderedResources[] =
	{
		pWriteBuffer->GetUAV(),
		m_pAccumulationBuffer->GetUAV()
	};

	ID3D11Buffer* apConstantBuffers[] = { m_pLevelInfoConstants };

	m_pDeviceContextD3D->CSSetShader(pShader, nullptr, 0);
	m_pDeviceContextD3D->CSSetConstantBuffers(0, 1, apConstantBuffers);
	m_pDeviceContextD3D->CSSetShaderResources(0, 7, apShaderResources);
	m_pDeviceContextD3D->CSSetUnorderedAccessViews(0, 2, apUnorderedResources, 0);
	m_pDeviceContextD3D->Dispatch(nDispatchX, nDispatchY, 1);

	ID3D11Buffer* nullBuf[] = { nullptr };
	ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAV[] = { nullptr, nullptr };

	m_pDeviceContextD3D->CSSetConstantBuffers(0, 1, nullBuf);
	m_pDeviceContextD3D->CSSetShaderResources(0, 7, nullSRV);
	m_pDeviceContextD3D->CSSetUnorderedAccessViews(0, 2, nullUAV, 0);
}

void CLightBakerDlg::BakeDirectLighting()
{
	// Passes are in order:
	// - Sun (replaces vertex data, no atomics)
	// - Direct lights (atomic add)
	// - Sky and emissive (atomic add)
	if (m_nBakeFlags & ELightBake_Sun)
		DispatchBakePass((m_nTotalVertices + 255) / 256, 1, m_pBakeSunShader, m_pColorLastResultBuffer, m_pColorCurrResultBuffer);

	if (m_nBakeFlags & ELightBake_Lights)
		DispatchBakePass((m_nTotalVertices + 15) / 16, (m_nNumLights + 15) / 16, m_pBakeDirectShader, m_pColorLastResultBuffer, m_pColorCurrResultBuffer);

	if ((m_nBakeFlags & ELightBake_Sky) || (m_nBakeFlags & ELightBake_Emissive))
		DispatchBakePass((m_nTotalVertices + 15) / 16, (m_nSkyEmissiveRayCount + 15) / 16, m_pBakeSkyEmissiveShader, m_pColorLastResultBuffer, m_pColorCurrResultBuffer);

	//m_pDeviceContextD3D->Flush();
}

void CLightBakerDlg::BakeIndirectLighting()
{
	CGpuBuffer* pReadBuffer = m_pColorLastResultBuffer;
	CGpuBuffer* pWriteBuffer = m_pColorCurrResultBuffer;

	// Copy the direct light result for the first bounce
	m_pDeviceContextD3D->CopyResource(pReadBuffer->GetBuffer(), m_pAccumulationBuffer->GetBuffer());
//	m_pDeviceContextD3D->Flush();

	for (int nBounce = 0; nBounce < m_nIndirectBounces; ++nBounce)
	{
		// clear the buffer for the next bounce accumulation
		pWriteBuffer->ClearUAV();

		DispatchBakePass((m_nTotalVertices + 15) / 16, (m_nIndirectRayCount + 15) / 16, m_pBakeIndirectShader, pReadBuffer, pWriteBuffer);

		//m_pDeviceContextD3D->Flush();
		std::swap(pReadBuffer, pWriteBuffer);
	}
}

void CLightBakerDlg::DownloadAndApplyToLevel()
{
	float4* pVertexData = m_pAccumulationBuffer->Map<float4>(D3D11_MAP_READ);
	if (pVertexData)
	{
		// it's easier to just traverse by sector again than to try and reverse index from the vertex
		int nVertexOffset = 0;
		for (int nSectorIndex = 0; nSectorIndex < m_nNumSectors; ++nSectorIndex)
		{
			// todo: might be better to actually also trace probes at the center of sectors to get better ambient
			float4 ambientColor = {0,0,0,0};
			float totalAmbient = 0.0f;

			const int nNumSurfaces = m_pJedLevel->SectorNSurfaces(nSectorIndex);
			for (int nSurfaceIndex = 0; nSurfaceIndex < nNumSurfaces; ++nSurfaceIndex)
			{
				const int nNumVertices = m_pJedLevel->SurfaceNVertices(nSectorIndex, nSurfaceIndex);
				for (int nVertexIndex = 0; nVertexIndex < nNumVertices; ++nVertexIndex)
				{
					float4 color = pVertexData[nVertexOffset++];

					if (m_nBakeFlags & ELightBake_ToneMap)
					{
						float a = 2.51f;
						float b = 0.03f;
						float c = 2.43f;
						float d = 0.59f;
						float e = 0.14f;
						color = ((color * (a * color + b)) / (color * (c * color + d) + e));
					

						// account for 2x overbright
						//color *= 0.5f;
						//color = color / (color + 1.0f);
						//color *= 2.0f;
					}

					if(m_nBakeFlags & ELightBake_GammaCorrect)
						color = ToSRGB(color);
					m_pJedLevel->SurfaceSetVertexLight(nSectorIndex, nSurfaceIndex, nVertexIndex, color.w, color.x, color.y, color.z);

					ambientColor += color;
					totalAmbient += 1.0f;
				}
				m_pJedLevel->SurfaceUpdate(nSectorIndex, nSurfaceIndex, 0);
			}

			// update sector ambient if needed
			tjedsectorrec sector;
			memset(&sector, 0, sizeof(tjedsectorrec));
			m_pJedLevel->GetSector(nSectorIndex, &sector, s_flags | s_ambient);
			if (!(sector.flags & ESector_NoAmbientLightRGB) && !(sector.flags & ESector_NoAmbientLight))
			{
				float invTotalAmbient = totalAmbient < 1e-5f ? 0.0f : 1.0f / totalAmbient;
				ambientColor *= invTotalAmbient;

				// todo: SED has rgb ambient for Jones?
				sector.ambient = ambientColor.w;
				m_pJedLevel->SetSector(nSectorIndex, &sector, s_ambient);
			}

			m_pJedLevel->SectorUpdate(nSectorIndex);
		}
	}
	m_pAccumulationBuffer->Unmap();
}

SColormap* CLightBakerDlg::LoadColormap(const wchar_t* sFileName)
{
	if (!sFileName || sFileName[0] == '\0')
		return nullptr;

	auto it = m_colormapCache.find(sFileName);
	if (it != m_colormapCache.end())
		return &it->second;

	int nFileHandle = m_pJed->OpenGameFile((char*)sFileName);
	if (nFileHandle < 0)
	{
		// the file wasn't found, to avoid console spam and slowdowns, insert a blank value
		m_colormapCache.emplace(sFileName, SColormap());
		return nullptr;
	}

	SColormapHeader colormapHeader;
	if (m_pJed->ReadFile(nFileHandle, &colormapHeader, sizeof(SColormapHeader)) != sizeof(SColormapHeader))
	{
		PrintMessage(m_pJed, msg_error, "Failed to read file '%ls'.", sFileName);
		m_pJed->CloseFile(nFileHandle);
		return nullptr;
	}

	if (strncmp(colormapHeader.magic, "CMP ", 4) != 0)
	{
		PrintMessage(m_pJed, msg_error, "Bad colormap header in file '%ls'.", sFileName);
		m_pJed->CloseFile(nFileHandle);
		return nullptr;
	}

	if (colormapHeader.nVersion < 30)
	{
		PrintMessage(m_pJed, msg_error, "Unsupported colormap version for '%ls'.", sFileName);
		m_pJed->CloseFile(nFileHandle);
		return nullptr;
	}

	uint8_t* paColors = new uint8_t[3 * 256];
	if (m_pJed->ReadFile(nFileHandle, paColors, (3 * 256)) != (3 * 256))
	{
		PrintMessage(m_pJed, msg_error, "Failed to read colors from '%ls'.", sFileName);
		m_pJed->CloseFile(nFileHandle);
		delete[] paColors;
		return nullptr;
	}

	uint8_t* paLightLevels = new uint8_t[64 * 256];
	if (m_pJed->ReadFile(nFileHandle, paLightLevels, (64 * 256)) != (64 * 256))
	{
		PrintMessage(m_pJed, msg_error, "Failed to read light levels from '%ls'.", sFileName);
		delete[] paColors;
		delete[] paLightLevels;
		m_pJed->CloseFile(nFileHandle);
		return nullptr;
	}

	SColormap& colormap = m_colormapCache[sFileName];
	memcpy(colormap.colors, paColors, 3 * 256);
	memcpy(colormap.nLightLevels, paLightLevels, 64 * 256);

	delete[] paColors;
	delete[] paLightLevels;

	return &colormap;
}

uint32_t CLightBakerDlg::LoadMaterialFillColor(const wchar_t* sFileName, const SColormap* pColormap)
{
	if (!sFileName || sFileName[0] == '\0')
		return 0;

	auto it = m_materialColorCache.find(sFileName);
	if (it != m_materialColorCache.end())
		return it->second;

	int nFileHandle = m_pJed->OpenGameFile((char*)sFileName);
	if (nFileHandle < 0)
	{
		// the file wasn't found, to avoid console spam and slowdowns, insert a blank value
		m_materialColorCache.emplace(sFileName, 0);
		return 0;
	}

	SMaterialFileHeader materialHeader;
	if (m_pJed->ReadFile(nFileHandle, &materialHeader, sizeof(SMaterialFileHeader)) != sizeof(SMaterialFileHeader))
	{
		PrintMessage(m_pJed, msg_error, "Failed to read file '%ls'.", sFileName);
		m_pJed->CloseFile(nFileHandle);
		return 0;
	}

	if (memcmp(materialHeader.magic, "MAT ", 4u))
	{
		PrintMessage(m_pJed, msg_error, "Bad material header in file '%ls'.", sFileName);
		m_pJed->CloseFile(nFileHandle);
		return 0;
	}

	if (materialHeader.nVersion != 0x32)
	{
		PrintMessage(m_pJed, msg_error, "Unsupported material version for '%ls'.", sFileName);
		m_pJed->CloseFile(nFileHandle);
		return 0;
	}

	// read the last header (emissive signs and such start at broken, we want the cel with illumination)
	SMaterialRecordHeader recordHeader;
	for (uint32_t i = 0; i < materialHeader.nRecordCount; ++i)
	{
		if (m_pJed->ReadFile(nFileHandle, &recordHeader, sizeof(SMaterialRecordHeader)) != sizeof(SMaterialRecordHeader))
		{
			PrintMessage(m_pJed, msg_error, "Failed to read material record header from '%ls'.", sFileName);
			m_pJed->CloseFile(nFileHandle);
			return 0;
		}
		if (recordHeader.nTextureType & 8)
		{
			SMaterialRecordHeaderExt ext;
			if (m_pJed->ReadFile(nFileHandle, &ext, sizeof(SMaterialRecordHeaderExt)) != sizeof(SMaterialRecordHeaderExt))
			{
				PrintMessage(m_pJed, msg_error, "Failed to read material texture header from '%ls'.", sFileName);
				m_pJed->CloseFile(nFileHandle);
				return 0;
			}
		}
	}
	m_materialColorCache.emplace(sFileName, recordHeader.nFillColor);
	return recordHeader.nFillColor;
}

extern "C"
{
	__declspec(dllexport) bool __stdcall JEDPluginLoadStdCall(IJED* pJed)
	{
		static bool bInit = false;

		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		if (!g_pLightBaker)
		{
			CWnd* pJedWindow = CWnd::FromHandle((HWND)pJed->GetJedWindow(jw_Main));
			g_pLightBaker = new CLightBakerDlg(pJed, pJedWindow);
			g_pLightBaker->Create(IDD_LIGHTBAKER_DLG, pJedWindow);
			if(!g_pLightBaker->CreateDeviceD3D())
			{
				PrintMessage(pJed, msg_error, "Failed to initialize 3D device.");
				return false;
			}
			bInit = true;
		}

		if (!bInit)
		{
			PrintMessage(pJed, msg_error, "Light Baker failed to initialize.");
			return false;
		}

		g_pLightBaker->ShowWindow(SW_SHOW);
		return true;
	}

	// workaround for name export, JED expects JEDPluginLoadStdCall
	#pragma comment(linker, "/EXPORT:JEDPluginLoadStdCall=_JEDPluginLoadStdCall@4")
}
