#pragma once

class CGpuBuffer
{
public:
	CGpuBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext, int numElements, int stride, DXGI_FORMAT format, int writeable, int readable, D3D11_RESOURCE_MISC_FLAG miscFlags);
	~CGpuBuffer();

	void* Map(D3D11_MAP mapping);
	void Unmap();

	template <typename T>
	T* Map(D3D11_MAP mapping)
	{
		return (T*)Map(mapping);
	}

	ID3D11Buffer* GetBuffer() { return m_pBuffer; }
	ID3D11ShaderResourceView* GetSRV() { return m_pShaderView; }
	ID3D11UnorderedAccessView* GetUAV() { return m_pUnorderedView; }

	void ClearUAV();

private:
	ID3D11Buffer*              m_pBuffer;
	ID3D11ShaderResourceView*  m_pShaderView;
	ID3D11UnorderedAccessView* m_pUnorderedView;

	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	D3D11_MAPPED_SUBRESOURCE   mapped;
};