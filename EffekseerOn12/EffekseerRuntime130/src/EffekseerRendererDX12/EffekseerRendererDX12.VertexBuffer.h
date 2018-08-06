
#pragma once
///	Include
#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererCommon\EffekseerRenderer.VertexBufferBase.h"
#include "EffekseerRendererDX12.DeviceObject.h"

namespace EffekseerRendererDX12
{
	class VertexBuffer
		: public DeviceObject
		, public ::EffekseerRenderer::VertexBufferBase
	{
	private:
		ComPtr<ID3D12Resource>		m_buffer;
		D3D12_VERTEX_BUFFER_VIEW m_bufferView;
		void*				m_lockedResource;

		uint32_t			m_vertexRingOffset;
		bool				m_ringBufferLock;

		int32_t				m_ringLockedOffset;
		int32_t				m_ringLockedSize;

		VertexBuffer(RendererImplemented* renderer, ComPtr<ID3D12Resource> buffer, const D3D12_VERTEX_BUFFER_VIEW& bufferView, int size, bool isDynamic);
	public:
		virtual ~VertexBuffer();

		static VertexBuffer* Create(RendererImplemented* renderer, int size, bool isDynamic);

		D3D12_VERTEX_BUFFER_VIEW* GetInterface() { return &m_bufferView; }

		virtual void OnLostDevice();
		virtual void OnResetDevice();

		void Lock();
		bool RingBufferLock(int32_t size, int32_t& offset, void*& data);
		bool TryRingBufferLock(int32_t size, int32_t& offset, void*& data);
		void Unlock();

	};
}


