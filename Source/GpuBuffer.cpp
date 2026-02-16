#include "pch.h"
#include "framework.h"

#include "GpuBuffer.h"

// todo: move it out of the constructor is probably a good idea
CGpuBuffer::CGpuBuffer()
	: m_pDevice(nullptr)
	, m_pDeviceContext(nullptr)
	, m_pBuffer(nullptr)
	, m_pShaderView(nullptr)
	, m_pUnorderedView(nullptr)
{
	memset(&mapped, 0, sizeof(mapped));
}

CGpuBuffer::~CGpuBuffer()
{
	Release();
}

bool CGpuBuffer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext, int numElements, int stride, DXGI_FORMAT format, int writeable, int readable, D3D11_RESOURCE_MISC_FLAG miscFlags)
{
	m_pDevice = pDevice;
	m_pDeviceContext = pDeviceContext;
	m_pBuffer = nullptr;
	m_pShaderView = nullptr;
	m_pUnorderedView = nullptr;

	memset(&mapped, 0, sizeof(mapped));

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Usage = D3D11_USAGE_DEFAULT;//(writeable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | (readable ? D3D11_CPU_ACCESS_READ : 0);
	desc.MiscFlags = miscFlags;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (writeable ? D3D11_BIND_UNORDERED_ACCESS : 0);
	desc.ByteWidth = numElements * stride;
	if (miscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
		desc.StructureByteStride = stride;
	pDevice->CreateBuffer(&desc, NULL, &m_pBuffer);

	if (!m_pBuffer)
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	//srvDesc.Buffer.FirstElement = 0;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.Format = format;
	if (miscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
	{
		srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		srvDesc.BufferEx.NumElements = desc.ByteWidth / 4;
	}
	else
	{
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.NumElements = numElements;
	}
	pDevice->CreateShaderResourceView(m_pBuffer, &srvDesc, &m_pShaderView);

	if (!m_pShaderView)
		return false;

	if (writeable)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory(&uavDesc, sizeof(uavDesc));
		uavDesc.Format = format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		if (miscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		{
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			uavDesc.Buffer.NumElements = desc.ByteWidth / 4;
		}
		else
		{
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = numElements;
		}
		pDevice->CreateUnorderedAccessView(m_pBuffer, &uavDesc, &m_pUnorderedView);
		if (!m_pUnorderedView)
			return false;
	}

	return true;
}

void CGpuBuffer::Release()
{
	if (m_pBuffer)
		m_pBuffer->Release();
	m_pBuffer = NULL;

	if (m_pShaderView)
		m_pShaderView->Release();
	m_pShaderView = NULL;

	if (m_pUnorderedView)
		m_pUnorderedView->Release();
	m_pUnorderedView = NULL;
}

void* CGpuBuffer::Map(D3D11_MAP mapping)
{
	m_pDeviceContext->Map(m_pBuffer, 0, mapping, 0, &mapped);
	return mapped.pData;
}

void CGpuBuffer::Unmap()
{
	if (mapped.pData)
		m_pDeviceContext->Unmap(m_pBuffer, 0);
	mapped.pData = 0;
}

void CGpuBuffer::ClearUAV()
{
	if (!m_pUnorderedView)
		return;
	UINT clear[4] = { 0, 0, 0, 0 };
	m_pDeviceContext->ClearUnorderedAccessViewUint(m_pUnorderedView, clear);
}