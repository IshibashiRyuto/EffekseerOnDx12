#include "EffekseerRendererDX12.VertexBuffer.h"

namespace EffekseerRendererDX12
{
	VertexBuffer::VertexBuffer(RendererImplemented* renderer, ComPtr<ID3D12Resource> buffer, const D3D12_VERTEX_BUFFER_VIEW& bufferView, int size, bool isDynamic)
		: DeviceObject(renderer)
		, VertexBufferBase(size, isDynamic)
		, m_buffer(buffer)
		, m_bufferView(bufferView)
		, m_vertexRingOffset(0)
		, m_ringBufferLock(false)
		, m_ringLockedOffset(0)
		, m_ringLockedSize(0)
	{
		m_lockedResource = new uint8_t[size];
	}

	VertexBuffer::~VertexBuffer()
	{
		ES_SAFE_DELETE_ARRAY(m_lockedResource);
	}

	VertexBuffer* VertexBuffer::Create(RendererImplemented* renderer, int size, bool isDynamic)
	{

		// Ç∆ÇËÇ†Ç¶Ç∏dynamicÇ©Ç«Ç§Ç©Ç…Ç©Ç©ÇÌÇÁÇ∏UploadÇ≈çÏê¨
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
		hBufferDesc.Width = size;
		hBufferDesc.Height = 1;
		hBufferDesc.DepthOrArraySize = 1;
		hBufferDesc.MipLevels = 1;
		hBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		hBufferDesc.SampleDesc.Count = 1;
		hBufferDesc.SampleDesc.Quality = 0;
		hBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		hBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ComPtr<ID3D12Resource> vb{ nullptr };

		if (FAILED(renderer->GetDevice()->CreateCommittedResource(&hHeapPropertie, D3D12_HEAP_FLAG_NONE, &hBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vb))))
		{
#ifdef _DEBUG
			MessageBox(nullptr, TEXT("Failed Create IndexBuffer."), TEXT("Failed"), MB_OK);
#endif
			return nullptr;
		}

		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = vb->GetGPUVirtualAddress();
		vbv.StrideInBytes = sizeof(float)*12;
		vbv.SizeInBytes = size;

		return new VertexBuffer(renderer, vb, vbv, size, isDynamic);
	}

	void VertexBuffer::OnLostDevice()
	{
	}

	void VertexBuffer::OnResetDevice()
	{
	}

	void VertexBuffer::Lock()
	{
		assert(!m_isLock);
		assert(!m_ringBufferLock);
		
		m_isLock = true;
		m_resource = (uint8_t*)m_lockedResource;
		m_offset = 0;

		m_vertexRingOffset = m_size;
	}

	bool VertexBuffer::RingBufferLock(int32_t size, int32_t & offset, void *& data)
	{
		assert(!m_isLock);
		assert(!m_ringBufferLock);
		assert(this->m_isDynamic);

		if (size > m_size) return false;

		if ((int32_t)m_vertexRingOffset + size > m_size)
		{
			offset = 0;
			m_ringLockedOffset = 0;
			m_ringLockedSize = size;
			m_vertexRingOffset = size;
		}
		else
		{
			offset = m_vertexRingOffset;
			m_ringLockedOffset = offset;
			m_ringLockedSize = size;

			m_vertexRingOffset += size;
		}

		data = (uint8_t*)m_lockedResource;
		m_resource = (uint8_t*)m_lockedResource;
		m_ringBufferLock = true;

		return true;
	}

	bool VertexBuffer::TryRingBufferLock(int32_t size, int32_t & offset, void *& data)
	{
		if ((int32_t)m_vertexRingOffset + size > m_size) return false;

		return RingBufferLock(size, offset, data);
	}

	void VertexBuffer::Unlock()
	{
		assert(m_isLock || m_ringBufferLock);

		if (m_isLock)
		{
			char* buf = nullptr;
			m_buffer->Map(0, nullptr, (void**)&buf);
			memcpy(buf, m_resource, m_size);
			m_buffer->Unmap(0, nullptr);
		}

		if (m_ringBufferLock)
		{
			char* buf = nullptr;
			auto result = m_buffer->Map(0, nullptr, (void**)&buf);

			uint8_t* dst = (uint8_t*)buf;
			dst += m_ringLockedOffset;

			uint8_t* src = (uint8_t*)m_resource;

			memcpy(dst, src, m_ringLockedSize);

			m_buffer->Unmap(0, nullptr);
		}

		m_resource = nullptr;
		m_isLock = false;
		m_ringBufferLock = false;
	}
}

