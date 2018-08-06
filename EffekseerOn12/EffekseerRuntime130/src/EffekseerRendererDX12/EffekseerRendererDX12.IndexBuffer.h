#pragma once

#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererCommon/EffekseerRenderer.IndexBufferBase.h"
#include "EffekseerRendererDX12.DeviceObject.h"

namespace EffekseerRendererDX12
{
	class IndexBuffer
		: public ::EffekseerRenderer::IndexBufferBase
		, public DeviceObject
	{
	private:
		ComPtr<ID3D12Resource>		m_buffer;
		D3D12_INDEX_BUFFER_VIEW m_bufferView;
		void*				m_lockedResource;

		IndexBuffer(RendererImplemented* renderer, ComPtr<ID3D12Resource> buffer, const D3D12_INDEX_BUFFER_VIEW& bufferView, int maxCount, bool isDynamic);
	public:
		virtual ~IndexBuffer();

		static IndexBuffer* Create(RendererImplemented* renderer, int maxCount, bool isDynamic);

		//ID3D12Resource* GetnInterface() { return m_buffer; }
		D3D12_INDEX_BUFFER_VIEW* GetInterface() { return &m_bufferView; }

		virtual void OnLostDevice();
		virtual void OnResetDevice();

		void Lock();
		void Unlock();
	};
}

