#pragma once

class CGpuBuffer
{
public:
	CGpuBuffer();
	~CGpuBuffer();

	bool Create(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext, int numElements, int stride, DXGI_FORMAT format, int writeable, int readable, D3D11_RESOURCE_MISC_FLAG miscFlags);
	void Release();

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

template <typename T>
class CGpuBufferMapping
{
public:
	CGpuBufferMapping(CGpuBuffer* pBuffer, D3D11_MAP mapping)
		: pBuffer(pBuffer)
		, paData(nullptr)
	{
		paData = pBuffer->Map<T>(mapping);
	}

	~CGpuBufferMapping()
	{
		if(paData)
		{
			pBuffer->Unmap();
			paData = nullptr;
		}
	}

	const T* RawData() const { return paData; }
	T* RawData() { return paData; }

	T operator[](size_t index) const
	{
		return paData[index];
	}

	T& operator[](size_t index)
	{
		return paData[index];
	}

	operator bool() const
	{
		return paData != nullptr;
	}

private:
	CGpuBuffer* pBuffer;
	T* paData;
};