#pragma once

#include"EffekseerRendererDX12.RendererImplemented.h"
#include"EffekseerRendererDX12.DeviceObject.h"
#include <wrl.h>
#include "EffekseerRendererDX12.TextureData.h"

using Microsoft::WRL::ComPtr;

namespace EffekseerRendererDX12
{
	class Shader
		: public DeviceObject
	{
	private:
		D3D12_SHADER_BYTECODE m_vertexShader;
		D3D12_SHADER_BYTECODE m_pixelShader;
		D3D12_INPUT_ELEMENT_DESC *m_InputElements;
		D3D12_INPUT_LAYOUT_DESC m_vertexDeclaration;
		

		std::vector<void*>		m_vertexConstantBuffers;
		std::vector<void*>		m_pixelConstantBuffers;

		int32_t					m_vertexRegisterCount;
		int32_t					m_pixelRegisterCount;

		int32_t					m_vertexBufferSize;
		int32_t					m_pixelBufferSize;

		int32_t					m_srvStartRootParamIdx{};

		int32_t					m_bufferIndex;
		int32_t					m_textureResourceIndex;
		int32_t					m_constantBufferViewEndIndex;

		UINT					m_heapIncrementSize;

		ComPtr<ID3D12DescriptorHeap> m_constantBufferDescriptorHeap;
		ComPtr<ID3D12DescriptorHeap> m_shaderResourceDescriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_handle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;

		std::vector<ComPtr<ID3D12Resource>> m_constantBuffersToVS;
		std::vector<ComPtr<ID3D12Resource>> m_constantBuffersToPS;

		ComPtr<ID3D12RootSignature> m_rootSignature;

		Shader(RendererImplemented* renderer,
			const D3D12_SHADER_BYTECODE& vertexShader,
			const D3D12_SHADER_BYTECODE& pixelShader,
			const D3D12_INPUT_LAYOUT_DESC& vertexDeclaration,
			D3D12_INPUT_ELEMENT_DESC* inputElements);

	public:
		virtual ~Shader();

		static Shader* Create(
			RendererImplemented* renderer,
			const uint8_t vertexShader[],
			int32_t vertexShaderSize,
			const uint8_t pixelShader[],
			int32_t pixelShaderSize,
			const char* name,
			const D3D12_INPUT_ELEMENT_DESC decl[],
			int32_t layoutCount);

		virtual void OnLostDevice();
		virtual void OnResetDevice();

		D3D12_SHADER_BYTECODE& GetVertexShader() { return m_vertexShader; }
		D3D12_SHADER_BYTECODE& GetPixelShader() { return m_pixelShader; }
		D3D12_INPUT_LAYOUT_DESC& GetLayoutInterface() { return m_vertexDeclaration; }

		void SetVertexConstantBufferSize(int32_t size);
		void SetPixelConstantBufferSize(int32_t size);

		void* GetVertexConstantBuffer() { return m_vertexConstantBuffers[m_bufferIndex]; }
		void* GetPixelConstantBuffer() { return m_pixelConstantBuffers[m_bufferIndex]; }

		void SetVertexRegisterCount(int32_t count) { m_vertexRegisterCount = count; }
		void SetPixelRegisterCount(int32_t count) { m_pixelRegisterCount = count; }

		void SetConstantBuffer();

		void ResetConstantBuffer();

		ID3D12DescriptorHeap* GetDescriptorHeap() { return m_constantBufferDescriptorHeap.Get(); }

		ID3D12RootSignature* GetRootSignature() { return m_rootSignature.Get(); }

		void SetTextureData(EffekseerRendererDX12::TextureData* textureData, int32_t rootParameterOffset = 0);

		void CreateRootSignature(D3D12_ROOT_PARAMETER *rootParam, int32_t size);

		void SetShaderResourceViewStartRootParamIdx(int32_t rootParamIndex);

	};

}

