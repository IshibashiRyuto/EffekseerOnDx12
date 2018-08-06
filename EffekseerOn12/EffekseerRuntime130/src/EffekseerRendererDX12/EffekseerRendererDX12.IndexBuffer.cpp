#include "EffekseerRendererDX12.IndexBuffer.h"

namespace EffekseerRendererDX12
{

	IndexBuffer::IndexBuffer(RendererImplemented* renderer, ComPtr<ID3D12Resource> buffer, const D3D12_INDEX_BUFFER_VIEW& bufferView, int maxCount, bool isDynamic)
		: DeviceObject(renderer)
		, IndexBufferBase(maxCount, isDynamic)
		, m_buffer(buffer)
		, m_bufferView(bufferView)
		, m_lockedResource ( nullptr )
	{
		m_lockedResource = new uint8_t[sizeof(uint16_t) * maxCount];
	}


	IndexBuffer::~IndexBuffer()
	{
	}

	IndexBuffer * IndexBuffer::Create(RendererImplemented * renderer, int maxCount, bool isDynamic)
	{

		D3D12_HEAP_PROPERTIES hHeapPropertie;
		//hHeapPropertie.Type = isDynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_CUSTOM;
		hHeapPropertie.Type = D3D12_HEAP_TYPE_UPLOAD;
		hHeapPropertie.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		hHeapPropertie.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		hHeapPropertie.CreationNodeMask = 1;
		hHeapPropertie.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC hBufferDesc;
		hBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		hBufferDesc.Alignment = 0;
		hBufferDesc.Width = sizeof(uint16_t) * maxCount;
		hBufferDesc.Height = 1;
		hBufferDesc.DepthOrArraySize = 1;
		hBufferDesc.MipLevels = 1;
		hBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		hBufferDesc.SampleDesc.Count = 1;
		hBufferDesc.SampleDesc.Quality = 0;
		hBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		hBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ComPtr<ID3D12Resource> ib{ nullptr };

		if (FAILED(renderer->GetDevice()->CreateCommittedResource(&hHeapPropertie, D3D12_HEAP_FLAG_NONE, &hBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ib))) )
		{
#ifdef _DEBUG
			MessageBox(nullptr, TEXT("Failed Create IndexBuffer."), TEXT("Failed"), MB_OK);
#endif
			return nullptr;
		}

		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = ib->GetGPUVirtualAddress();
		ibv.Format = DXGI_FORMAT_R16_UINT;
		ibv.SizeInBytes = (UINT)hBufferDesc.Width;

		return new IndexBuffer(renderer, ib, ibv, maxCount, isDynamic);
	}

	void IndexBuffer::OnLostDevice()
	{
	}

	void IndexBuffer::OnResetDevice()
	{
	}

	void IndexBuffer::Lock()
	{
		assert(!m_isLock);
		m_isLock = true;
		m_resource = (uint8_t*)m_lockedResource;
		m_indexCount = 0;
	}

	void IndexBuffer::Unlock()
	{
		assert(m_isLock);

		if (m_isDynamic)
		{
			// todo
			/*
			
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			GetRenderer()->GetContext()->Map(
				m_buffer,
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource );

			memcpy( mappedResource.pData, m_resource, sizeof(uint16_t) * GetMaxCount() );

			GetRenderer()->GetContext()->Unmap( m_buffer, 0 );
			*/
			char* buf;
			m_buffer->Map(0, nullptr, (void**)&buf);
			memcpy(buf, m_resource, sizeof(uint16_t) * GetMaxCount());
			m_buffer->Unmap(0, nullptr);

		}
		else
		{
			/*
			GetRenderer()->GetContext()->UpdateSubresource(
			m_buffer,
			0,
			NULL,
			m_resource,
			0,
			0 );
			*/
			// ‚Æ‚è‚ ‚¦‚¸isDynamic‚ð–³Ž‹‚·‚é

			char* buf;
			m_buffer->Map(0, nullptr, (void**)&buf);
			memcpy(buf, m_resource, sizeof(uint16_t) * GetMaxCount());
			m_buffer->Unmap(0, nullptr);

		}

		m_resource = nullptr;
		m_isLock = false;
	}

}