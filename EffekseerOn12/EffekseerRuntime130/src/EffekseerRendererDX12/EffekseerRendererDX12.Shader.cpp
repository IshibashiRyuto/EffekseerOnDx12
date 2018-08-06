#include "EffekseerRendererDX12.Shader.h"
#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererDX12.TextureData.h"

namespace EffekseerRendererDX12
{
	Shader::Shader(
		RendererImplemented* renderer,
		const D3D12_SHADER_BYTECODE& vertexShader,
		const D3D12_SHADER_BYTECODE& pixelShader,
		const D3D12_INPUT_LAYOUT_DESC& vertexDeclaration,
		D3D12_INPUT_ELEMENT_DESC* inputElements)
		: DeviceObject(renderer)
		, m_vertexShader(vertexShader)
		, m_pixelShader(pixelShader)
		, m_vertexDeclaration(vertexDeclaration)
		, m_InputElements(inputElements)
		, m_constantBufferToVS(nullptr)
		, m_constantBufferToPS(nullptr)
		, m_vertexConstantBuffer(nullptr)
		, m_pixelConstantBuffer(nullptr)
		, m_vertexRegisterCount(0)
		, m_pixelRegisterCount(0)
		, m_vertexBufferSize(0)
		, m_pixelBufferSize(0)
		, m_rootSignature(nullptr)
		, m_constantBufferDescriptorHeap(nullptr)
		, m_handle(D3D12_CPU_DESCRIPTOR_HANDLE{})
	{
		// ヒープの作成
		// squareCount * 2で事足りるとは思うが……
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NumDescriptors = (UINT)renderer->GetSquareMaxCount()*2;
		renderer->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_constantBufferDescriptorHeap));

		m_handle = m_constantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_GPUHandle = m_constantBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		// コンスタントバッファの作成
		// とりあえずこっちもヒープと同じ数だけ分作成する
		D3D12_HEAP_PROPERTIES heapPropertie{};
		heapPropertie.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapPropertie.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapPropertie.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapPropertie.VisibleNodeMask = 1;
		heapPropertie.CreationNodeMask = 1;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = 0x100 * ( (UINT)renderer->GetSquareMaxCount() * 2 );
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.MipLevels = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		if (FAILED(GetRenderer()->GetDevice()->CreateCommittedResource(&heapPropertie,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBufferToVS))))
		{
#ifdef _DEBUG
			MessageBox(nullptr, TEXT("Failed Create by VertexConstantBuffer."), TEXT("Failed"), MB_OK);
#endif
			return;
		}

		if (FAILED(GetRenderer()->GetDevice()->CreateCommittedResource(&heapPropertie,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBufferToPS))))
		{

#ifdef _DEBUG
			MessageBox(nullptr, TEXT("Failed Create by PixelConstantBuffer."), TEXT("Failed"), MB_OK);
#endif
			return;
		}

		m_cbvToVS.BufferLocation = m_constantBufferToVS->GetGPUVirtualAddress();
		m_cbvToPS.BufferLocation = m_constantBufferToPS->GetGPUVirtualAddress();

		m_constantBufferToVS->Map(0, nullptr, (void**)(&m_vertexConstantBufferStart));
		m_constantBufferToPS->Map(0, nullptr, (void**)(&m_pixelConstantBufferStart));


		//ルートシグネチャの作成
		{
			
			D3D12_DESCRIPTOR_RANGE samplerRange1;
			samplerRange1.NumDescriptors = 1;
			samplerRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			samplerRange1.BaseShaderRegister = 0;
			samplerRange1.RegisterSpace = 0;
			samplerRange1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE samplerRange2;
			samplerRange2.NumDescriptors = 1;
			samplerRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			samplerRange2.BaseShaderRegister = 1;
			samplerRange2.RegisterSpace = 0;
			samplerRange2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE samplerRange3;
			samplerRange3.NumDescriptors = 1;
			samplerRange3.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			samplerRange3.BaseShaderRegister = 2;
			samplerRange3.RegisterSpace = 0;
			samplerRange3.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE samplerRange4;
			samplerRange4.NumDescriptors = 1;
			samplerRange4.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			samplerRange4.BaseShaderRegister = 3;
			samplerRange4.RegisterSpace = 0;
			samplerRange4.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE cbRange;
			cbRange.NumDescriptors = 1;
			cbRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			cbRange.BaseShaderRegister = 0;
			cbRange.RegisterSpace = 0;
			cbRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE srvRange1;
			srvRange1.NumDescriptors = 1;
			srvRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			srvRange1.BaseShaderRegister = 0;
			srvRange1.RegisterSpace = 0;
			srvRange1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE srvRange2;
			srvRange2.NumDescriptors = 1;
			srvRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			srvRange2.BaseShaderRegister = 1;
			srvRange2.RegisterSpace = 0;
			srvRange2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER samplerRootParam1;
			samplerRootParam1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			samplerRootParam1.DescriptorTable = { 1, &samplerRange1 };
			samplerRootParam1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_PARAMETER samplerRootParam2;
			samplerRootParam2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			samplerRootParam2.DescriptorTable = { 1, &samplerRange2 };
			samplerRootParam2.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_PARAMETER samplerRootParam3;
			samplerRootParam3.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			samplerRootParam3.DescriptorTable = { 1, &samplerRange3 };
			samplerRootParam3.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_PARAMETER samplerRootParam4;
			samplerRootParam4.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			samplerRootParam4.DescriptorTable = { 1, &samplerRange4 };
			samplerRootParam4.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_PARAMETER vertexcbRootParam;
			vertexcbRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			vertexcbRootParam.DescriptorTable = { 1,&cbRange };
			vertexcbRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			D3D12_ROOT_PARAMETER pixelcbRootParam;
			pixelcbRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			pixelcbRootParam.DescriptorTable = { 1,&cbRange };
			pixelcbRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_PARAMETER srvRootParam1;
			srvRootParam1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			srvRootParam1.DescriptorTable = { 1, &srvRange1 };
			srvRootParam1.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_PARAMETER srvRootParam2;
			srvRootParam2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			srvRootParam2.DescriptorTable = { 1, &srvRange2 };
			srvRootParam2.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_PARAMETER shader_rootParams[] = {
				samplerRootParam1,
				samplerRootParam2,
				samplerRootParam3,
				samplerRootParam4,
				vertexcbRootParam,
				pixelcbRootParam,
				srvRootParam1,
				srvRootParam2,
			};

			CreateRootSignature(shader_rootParams, 8);
			SetShaderResourceViewStartRootParamIdx(6);

		}

	}

	Shader::~Shader()
	{
		ES_SAFE_DELETE_ARRAY(m_InputElements);
	}

	Shader* Shader::Create(
		RendererImplemented* renderer,
		const uint8_t vertexShader[],
		int32_t vertexShaderSize,
		const uint8_t pixelShader[],
		int32_t pixelShaderSize,
		const char* name,
		const D3D12_INPUT_ELEMENT_DESC decl[],
		int32_t layoutCount)
	{
		assert(renderer != nullptr);
		assert(renderer->GetDevice() != nullptr);

		D3D12_SHADER_BYTECODE vs;
		D3D12_SHADER_BYTECODE ps;

		vs.pShaderBytecode = vertexShader;
		vs.BytecodeLength = vertexShaderSize;

		ps.pShaderBytecode = pixelShader;
		ps.BytecodeLength = pixelShaderSize;

		D3D12_INPUT_LAYOUT_DESC vertexDeclaration{};
		D3D12_INPUT_ELEMENT_DESC* inputElements = new D3D12_INPUT_ELEMENT_DESC[layoutCount];

		for (int i = 0; i < layoutCount; ++i)
		{
			inputElements[i] = decl[i];
		}

		vertexDeclaration.NumElements = layoutCount;
		vertexDeclaration.pInputElementDescs = inputElements;

		return new Shader(renderer, vs, ps, vertexDeclaration, inputElements);
	}

	void Shader::OnLostDevice()
	{
	}
	
	void Shader::OnResetDevice()
	{
	}

	void Shader::SetVertexConstantBufferSize(int32_t size)
	{
		m_vertexBufferSize = (size + 0xff) & ~0xff;
		m_cbvToVS.SizeInBytes = m_vertexBufferSize;
	}

	void Shader::SetPixelConstantBufferSize(int32_t size)
	{
		m_pixelBufferSize = (size + 0xff) & ~0xff;
		m_cbvToPS.SizeInBytes = m_vertexBufferSize;
	}

	void Shader::SetConstantBuffer()
	{
		if (m_vertexRegisterCount > 0)
		{

			GetRenderer()->GetDevice()->CreateConstantBufferView(&m_cbvToVS, m_handle);

			// デスクリプタをコマンドに積む処理
			GetRenderer()->GetCommandList()->SetGraphicsRootDescriptorTable(4, m_GPUHandle);

			m_vertexConstantBuffer = (uint8_t*)m_vertexConstantBuffer + m_vertexBufferSize;
			m_cbvToVS.BufferLocation += m_vertexBufferSize;
			m_handle.ptr += GetRenderer()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_GPUHandle.ptr += GetRenderer()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		if (m_pixelRegisterCount > 0)
		{
			GetRenderer()->GetDevice()->CreateConstantBufferView(&m_cbvToPS, m_handle);

			// デスクリプタをコマンドに積む処理
			GetRenderer()->GetCommandList()->SetGraphicsRootDescriptorTable(5, m_GPUHandle);
			

			m_pixelConstantBuffer = (uint8_t*)m_pixelConstantBuffer + m_pixelBufferSize;
			m_cbvToPS.BufferLocation += m_pixelBufferSize;
			m_handle.ptr += GetRenderer()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_GPUHandle.ptr += GetRenderer()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	void Shader::ResetConstantBuffer()
	{
		m_vertexConstantBuffer = m_vertexConstantBufferStart;
		m_pixelConstantBuffer = m_pixelConstantBufferStart;

		m_cbvToVS.BufferLocation = m_constantBufferToVS->GetGPUVirtualAddress();
		m_cbvToPS.BufferLocation = m_constantBufferToPS->GetGPUVirtualAddress();

		m_handle = m_constantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_GPUHandle = m_constantBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	}

	void Shader::SetTextureData(EffekseerRendererDX12::TextureData * textureData, int32_t rootParameterOffset)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = textureData->GetShaderResourceView();
		GetRenderer()->GetDevice()->CreateShaderResourceView(textureData->GetResource(), &srvDesc, m_handle);
		// デスクリプタをコマンドに積む処理

		GetRenderer()->GetCommandList()->SetGraphicsRootDescriptorTable(m_srvStartRootParamIdx + rootParameterOffset, m_GPUHandle);

		m_handle.ptr += GetRenderer()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_GPUHandle.ptr += GetRenderer()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	void Shader::CreateRootSignature(D3D12_ROOT_PARAMETER *rootParam, int32_t size)
	{
		//ルートシグネチャの作成
		{
			ComPtr<ID3DBlob> signature = nullptr;
			ComPtr<ID3DBlob> error = nullptr;

			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			rootSignatureDesc.NumParameters = size;
			rootSignatureDesc.pParameters = rootParam;
			rootSignatureDesc.NumStaticSamplers = 0;

			auto hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

			if (FAILED(hr))
			{
#ifdef _DEBUG
				MessageBox(nullptr, TEXT("Failed Create Signature."), TEXT("Failed"), MB_OK);
#endif
				return;
			}

			if (FAILED(
				GetRenderer()->GetDevice()->CreateRootSignature(
					0,
					signature->GetBufferPointer(),
					signature->GetBufferSize(),
					IID_PPV_ARGS(&m_rootSignature))))
			{
#ifdef _DEBUG
				MessageBox(nullptr, TEXT("Failed Create RootSignature."), TEXT("Failed"), MB_OK);
#endif
				return;
			}

		}
	}
	void Shader::SetShaderResourceViewStartRootParamIdx(int32_t rootParamIndex)
	{
		m_srvStartRootParamIdx = rootParamIndex;
	}
}