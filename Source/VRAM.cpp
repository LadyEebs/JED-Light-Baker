#include "pch.h"
#include "framework.h"

#include <assert.h>

#include "VRAM.h"

static constexpr int kVRAMSize = 128; // needs to match kVRAMSize in VRAM.hlsli

static constexpr int BitWidth(uint32_t val) noexcept
{
	return val == 0 ? 0 : std::bit_width(val) - 1;
}

void CVRAM::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_pDevice = pDevice;
	m_pDeviceContext = pContext;

	m_vram.Create(pDevice, pContext, kVRAMSize * 1024 * 1024, sizeof(uint8_t), DXGI_FORMAT_R8_UINT, 0, 0, (D3D11_RESOURCE_MISC_FLAG)0);
	m_descriptors.Create(pDevice, pContext, 256, sizeof(SVRAMDescriptor), DXGI_FORMAT_UNKNOWN, 1, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	
	//m_vram.ClearUAV();
	//m_descriptors.ClearUAV();

	m_vramBlocks = new SVRAMBlock;
	m_vramBlocks->nOffset = 0;
	m_vramBlocks->nSize = kVRAMSize * 1024 * 1024;
	m_vramBlocks->pNext = nullptr;
}

void CVRAM::Release()
{
	Unlock();

	SVRAMBlock* curr = m_vramBlocks;
	while (curr)
	{
		SVRAMBlock* pNext = curr->pNext;
		delete curr;
		curr = pNext;
	}
	m_vramBlocks = nullptr;

	m_vram.Release();
	m_descriptors.Release();
	m_descriptorData.clear();
	m_freeDescriptors = std::stack<int>();
	m_pVRAMData = nullptr;
}

void CVRAM::FreeSurfaces()
{
	SVRAMBlock* curr = m_vramBlocks;
	while (curr)
	{
		SVRAMBlock* pNext = curr->pNext;
		delete curr;
		curr = pNext;
	}
	m_vramBlocks = new SVRAMBlock;
	m_vramBlocks->nOffset = 0;
	m_vramBlocks->nSize = kVRAMSize * 1024 * 1024;
	m_vramBlocks->pNext = nullptr;
	m_descriptorData.clear();
	//m_descriptors.ClearUAV();
	m_freeDescriptors = std::stack<int>();
}

void CVRAM::UpdateDescriptor(uint32_t index, SVRAMDescriptor* pData)
{

	if (index < m_descriptorData.size())
	{
		m_descriptorData[index] = *pData;
	}
	else
	{
		m_descriptorData.push_back(*pData);
	}

	// Resize the descriptor buffer
	if (m_descriptorData.size() >= m_descriptors.GetNumElements())
	{
		m_descriptors.Release();
		m_descriptors.Create(m_pDevice, m_pDeviceContext, m_descriptorData.size() * 2, sizeof(SVRAMDescriptor), DXGI_FORMAT_UNKNOWN, 1, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED);
	
		// reupload the entire buffer
		m_pDeviceContext->UpdateSubresource(m_descriptors.GetBuffer(), 0, nullptr, pData, 0, 0);
	}
	else
	{
		D3D11_BOX box;
		memset(&box, 0, sizeof(D3D11_BOX));
		box.left = index * sizeof(SVRAMDescriptor);
		box.right = box.left + sizeof(SVRAMDescriptor);
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		m_pDeviceContext->UpdateSubresource(m_descriptors.GetBuffer(), 0, &box, pData, 0, 0);
	}
}

int CVRAM::CreateDescriptor(SVRAMDescriptor* pDescriptor)
{
	int idx;
	if (m_freeDescriptors.size())
	{
		idx = m_freeDescriptors.top();
		m_freeDescriptors.pop();
	}
	else
	{
		idx = m_descriptorData.size();
	}

	UpdateDescriptor(idx, pDescriptor);

	return idx;
}

void CVRAM::ReleaseDescriptor(int idx)
{
	m_freeDescriptors.push(idx);
}

bool CVRAM::Lock()
{
	m_pVRAMData = m_vram.Map(D3D11_MAP_WRITE);
	return m_pVRAMData != nullptr;
}

void CVRAM::Unlock()
{
	if (m_pVRAMData)
	{
		m_vram.Unmap();
		m_pVRAMData = nullptr;
	}
}

uint64_t CVRAM::CreateSurface(SSurfaceFormat* pSurfaceFormat)
{
	uint32_t size = pSurfaceFormat->nTextureSizeInBytes;

	SVRAMBlock** curr = &m_vramBlocks;
	while (*curr)
	{
		SVRAMBlock* block = *curr;
		uint32_t offset = block->nOffset;
		if (block->nSize >= size)
		{
			if (size == block->nSize)
			{
				// Exact fit, remove block
				*curr = block->pNext;
				delete block;
			}
			else
			{
				// Partial use of block
				block->nOffset = offset + size;
				block->nSize -= size;
			}

			SVRAMDescriptor desc(pSurfaceFormat, offset, size);
			int descriptor = CreateDescriptor(&desc);

			// todo: the descriptor is limited here to 10 bits and that's not enough
			return ((uint64_t)(descriptor & 0x3FF) << 54) | ((uint64_t)(size & 0x3FFFFF) << 32) | ((uint64_t)offset & 0xFFFFFFFF);
		}
		curr = &block->pNext;
	}
	return 0;
}

void CVRAM::ReleaseSurface(uint64_t handle)
{
	SVRAMBlock* new_block = new SVRAMBlock;
	if (!new_block)
		return;

	uint32_t offset = (uint32_t)(handle & 0xFFFFFFFF);
	uint32_t size = (uint32_t)(handle >> 32) & 0x3FFFFF;
	int descriptor = (int)(handle >> 54);

	ReleaseDescriptor(descriptor);

	new_block->nOffset = offset;
	new_block->nSize = size;
	new_block->pNext = nullptr;

	SVRAMBlock** curr = &m_vramBlocks;
	while (*curr && (*curr)->nOffset < offset)
		curr = &(*curr)->pNext;

	new_block->pNext = *curr;
	*curr = new_block;

	// Coalesce adjacent blocks
	SVRAMBlock* prev = nullptr;
	SVRAMBlock* iter = m_vramBlocks;

	while (iter && iter->pNext)
	{
		if ((iter->nOffset + iter->nSize) == iter->pNext->nOffset)
		{
			SVRAMBlock* to_merge = iter->pNext;
			iter->nSize += to_merge->nSize;
			iter->pNext = to_merge->pNext;
			delete to_merge;
		}
		else
		{
			iter = iter->pNext;
		}
	}
}

void* CVRAM::GetSurfacePointer(uint64_t handle)
{
	if (!m_pDevice || !m_pVRAMData || handle == -1)
		return nullptr;

	uint32_t offset = (uint32_t)(handle & 0xFFFFFFFF);
	return (uint8_t*)m_pVRAMData + offset;
}

ID3D11ShaderResourceView* CVRAM::GetSRV()
{
	return m_vram.GetSRV();
}

ID3D11ShaderResourceView* CVRAM::GetDescriptorSRV()
{
	return m_descriptors.GetSRV();
}


CVBuffer::CVBuffer(CVBuffer&& other) noexcept
	: m_pVRAM(other.m_pVRAM)
	, m_bIsLocked(other.m_bIsLocked)
	, m_nLockCount(other.m_nLockCount)
	, m_format(other.m_format)
	, m_mappedData(other.m_mappedData)
	, m_hSurfaceHandle(other.m_hSurfaceHandle)
	, m_nWidthBitCount(other.m_nWidthBitCount)
{
	other.m_pVRAM = nullptr;
	other.m_bIsLocked = false;
	other.m_nLockCount = 0;
	other.m_mappedData = nullptr;
}

CVBuffer& CVBuffer::operator=(CVBuffer&& other) noexcept
{
	if (this != &other)
	{
		Release();
		m_pVRAM = other.m_pVRAM;
		m_bIsLocked = other.m_bIsLocked;
		m_nLockCount = other.m_nLockCount;
		m_format = other.m_format;
		m_mappedData = other.m_mappedData;
		m_hSurfaceHandle = other.m_hSurfaceHandle;
		m_nWidthBitCount = other.m_nWidthBitCount;
		other.m_pVRAM = nullptr;
		other.m_bIsLocked = false;
		other.m_nLockCount = 0;
		other.m_mappedData = nullptr;
	}
	return *this;
}

bool CVBuffer::Create(CVRAM* pVRAM, SSurfaceFormat* pFormat)
{
	assert(pVRAM && "Invalid VRAM pointer");

	m_pVRAM = pVRAM;
	m_format = *pFormat;
	m_mappedData = nullptr;
	m_bIsLocked = false;
	m_nLockCount = 0;
	m_nWidthBitCount = BitWidth(m_format.nWidth);

	uint32_t nBytesPerPixel = m_format.format.nBitsPerPixel / 8;
	m_format.nWidthInBytes = m_format.nWidth * nBytesPerPixel;
	m_format.nWidthInPixels = m_format.nWidth;
	m_format.nTextureSizeInBytes = m_format.nHeight * m_format.nWidth * nBytesPerPixel;

	m_bIsLocked = true;
	m_hSurfaceHandle = m_pVRAM->CreateSurface(&m_format);
	if (m_hSurfaceHandle == 0)
	{
		printf("%d: Error when creating a DirectDraw vbuffer surface.\n", __LINE__);
		return false;
	}
	return true;
}

void CVBuffer::Release()
{
	Unlock();

	if (m_bIsLocked)
	{
		m_pVRAM->ReleaseSurface(m_hSurfaceHandle);
		m_hSurfaceHandle = 0;
		m_bIsLocked = false;
	}
}

void* CVBuffer::Lock()
{
	if (!m_pVRAM)
		return nullptr;

	if (m_nLockCount > 0)
		return nullptr;

	if (m_bIsLocked)
		m_mappedData = m_pVRAM->GetSurfacePointer(m_hSurfaceHandle);

	if (!m_mappedData)
		return nullptr;

	++m_nLockCount;

	return m_mappedData;
}

void CVBuffer::Unlock()
{
	if (!m_pVRAM)
		return;

	if (m_nLockCount <= 0)
		return;

	--m_nLockCount;
	if (m_nLockCount == 0)
	{
		if (m_bIsLocked)
		{
			m_mappedData = nullptr;
		}
	}
}