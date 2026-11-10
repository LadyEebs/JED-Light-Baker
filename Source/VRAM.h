#pragma once

#include <stdint.h>
#include <stack>
#include <vector>

#include "GpuBuffer.h"
#include "Types.h"

// Allocation block (points into vram buffer)
struct SVRAMBlock
{
	uint32_t    nOffset;
	uint32_t    nSize;
	SVRAMBlock* pNext;
};

// Packed form of rdTexformat for GPU access
struct SVRAMDescriptorFormat
{
	SVRAMDescriptorFormat(const SColorFormat* pFormat)
	{
		nColorMode = pFormat->nColorMode;
		nBitsPerPixel = pFormat->nBitsPerPixel;
		nRedBits = pFormat->nRedBits;
		nRedShift = pFormat->nRedShift;
		nRedBitDiff = pFormat->nRedDiff;
		nGreenBits = pFormat->nGreenBits;
		nGreenShift = pFormat->nGreenShift;
		nGreenBitDiff = pFormat->nGreenDiff;
		nBlueBits = pFormat->nBlueBits;
		nBlueShift = pFormat->nBlueShift;
		nBlueBitDiff = pFormat->nBlueDiff;
		nAlphaBits = pFormat->nAlphaBits;
		nAlphaShift = pFormat->nAlphaShift;
		nAlphaBitDiff = pFormat->nAlphaDiff;
	}

	// low 32
	uint32_t nColorMode : 2;
	uint32_t nBitsPerPixel : 6;
	uint32_t nRedBits: 4;
	uint32_t nRedShift : 6;
	uint32_t nRedBitDiff : 4;
	uint32_t nGreenBits : 4;
	uint32_t nGreenShift : 6;

	// high 32
	uint32_t nGreenBitDiff : 4;
	uint32_t nBlueBits : 4;
	uint32_t nBlueShift : 6;
	uint32_t nBlueBitDiff : 4;
	uint32_t nAlphaBits : 4;
	uint32_t nAlphaShift : 6;
	uint32_t nAlphaBitDiff : 4;
};
static_assert(sizeof(SVRAMDescriptorFormat) == sizeof(uint64_t), "SVRAMDescriptorFormat must be 64 bits in size");

// Texture descriptor, describes a texture living in the vram buffer
struct SVRAMDescriptor
{
	SVRAMDescriptor(const SColorFormat* pFormat) : format(pFormat) {}

	uint32_t              nWidth, nHeight;
	uint32_t              nPadding0;
	uint32_t              nTotalSize;
	uint32_t              nOffset; // offset into vram
	uint32_t              nRowStride;
	SVRAMDescriptorFormat format;
};
static_assert(sizeof(SVRAMDescriptor) == sizeof(uint32_t) * 8, "SVRAMDescriptor must be 32 bytes in size");

struct SSurfaceFormat
{
	int32_t nWidth;
	int32_t nHeight;
	uint32_t nTextureSizeInBytes;
	uint32_t nWidthInBytes;
	uint32_t nWidthInPixels;
	SColorFormat format;
};

class CVRAM
{
public:
	~CVRAM() { Release(); }

	void Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Release();

	void UpdateDescriptor(uint32_t index, SVRAMDescriptor* pData);
	int  CreateDescriptor(SVRAMDescriptor* pDescriptor);
	void ReleaseDescriptor(int idx);

	uint64_t CreateSurface(SSurfaceFormat* pTexFormat);
	void ReleaseSurface(uint64_t handle);
	void* LockSurface(uint64_t handle);
	void UnlockSurface(uint64_t handle);

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;

	// VRAM allocation blocks and GPU storage
	SVRAMBlock* m_vramBlocks = nullptr;
	int         m_vramMapCount = 0; // vram map status flag, increments on every map, decrements on every unmap, only does map/unmap once
	CGpuBuffer  m_vram;
	void*       m_pVRAMData = nullptr; // pointer to mapped VRAM data for CPU access

	// Texture descriptor storage
	CGpuBuffer  m_descriptors;
	std::vector<SVRAMDescriptor> m_descriptorData;
	std::stack<int> m_freeDescriptors;
};

class CVBuffer
{
public:
	~CVBuffer() { Release(); }

	CVBuffer() = default;
	CVBuffer(const CVBuffer&) = delete;
	CVBuffer& operator=(const CVBuffer&) = delete;
	CVBuffer(CVBuffer&& other) noexcept;
	CVBuffer& operator=(CVBuffer&& other) noexcept;

	bool Create(CVRAM* pVRAM, SSurfaceFormat* pFormat);
	void Release();

	void* Lock();
	void Unlock();

	uint32_t GetWidth() const { return m_format.nWidth; }
	uint32_t GetHeight() const { return m_format.nHeight; }
	const SSurfaceFormat& GetFormat() const { return m_format; }
	const SColorFormat& GetColorFormat() const { return m_format.format; }

	uint32_t GetWidthBitCount() const { return m_nWidthBitCount; }
	uint64_t GetSurfaceHandle() const { return m_hSurfaceHandle; }

private:
	CVRAM* m_pVRAM = nullptr;
	bool m_bIsLocked = false;
	uint32_t m_nLockCount = 0;
	SSurfaceFormat m_format;
	void* m_mappedData = nullptr;
	uint32_t m_nWidthBitCount = 0;
	uint64_t m_hSurfaceHandle = 0;
};