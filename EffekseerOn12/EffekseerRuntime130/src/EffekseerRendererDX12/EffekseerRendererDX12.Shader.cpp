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
		, m_vertexRegisterCount(0)
		, m_pixelRegisterCount(0)
		, m_vertexBufferSize(0)
		, m_pixelBufferSize(0)
		, m_rootSignature(nullptr)
		, m_constantBufferDescriptorHeap(nullptr)
		, m_handle(D3D12_CPU_DESCRIPTOR_HANDLE{})
	{
		// ヒープの作成
		// 定数バッファヒープはsquareCount * 2 (三角形の数) *2 (VSとPS)分作成
		// シェーダリソースヒープはsquareCount * 2 (三角形の数) *2 (テクスチャ2枚分)
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NumDescriptors = (UINT)renderer->GetSquareMaxCount() * 2 * 2;

		static int shaderNum = 0;
		++shaderNum;

		auto result = renderer->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_constantBufferDescriptorHeap));
		if(FAILED(result))
		{
#ifdef _DEBUG
			MessageBox(nullptr, TEXT("Failed Create by ConstantBufferDescHeap."), TEXT("Failed"), MB_OK);
#endif
			return;
		}
		heapDesc.NumDescriptors = (UINT)renderer->GetSquareMaxCount() * 2 * 2;
		result = renderer->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_shaderResourceDescriptorHeap));

		if (FAILED(result))
		{
#ifdef _DEBUG
			MessageBox(nullptr, TEXT("Failed Create by ConstantBufferDescHeap."), TEXT("Failed"), MB_OK);
#endif
			return;
		}

		m_handle = m_constantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_GPUHandle = m_constantBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart();


		// コンスタントバッファの作成
		// 三角形の数分(squareCount * 2)作成する
		// サイズは(現状)256byte固定
		// (それ以上のデータを送ることがないため)
		int bufferNum = (int)renderer->GetSquareMaxCount() * 2;
		m_constantBuffersToVS.resize(bufferNum);
		m_constantBuffersToPS.resize(bufferNum);
		m_vertexConstantBuffers.resize(bufferNum);
		m_pixelConstantBuffers.resize(bufferNum);

		D3D12_HEAP_PROPERTIES heapPropertie{};
		heapPropertie.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapPropertie.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapPropertie.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapPropertie.VisibleNodeMask = 1;
		heapPropertie.CreationNodeMask = 1;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = 0x100;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.MipLevels = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
		cbv.SizeInBytes = 0x100;
		auto heapCPUHandle = m_constantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		auto device = renderer->GetDevice();
		m_heapIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (int i = 0; i < bufferNum; ++i)
		{
			static int allBufferNum = 0;
			++allBufferNum;

			auto result = GetRenderer()->GetDevice()->CreateCommittedResource(&heapPropertie,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_constantBuffersToVS[i]));

			if (FAILED(result))
			{
#ifdef _DEBUG
				MessageBox(nullptr, TEXT("Failed Create by VertexConstantBuffer."), TEXT("Failed"), MB_OK);
#endif
				return;
			}

			m_constantBuffersToVS[i]->Map(0, nullptr, (void**)(&m_vertexConstantBuffers[i]));
			cbv.BufferLocation = m_constantBuffersToVS[i]->GetGPUVirtualAddress();
			device->CreateConstantBufferView(&cbv, heapCPUHandle);
			heapCPUHandle.ptr += m_heapIncrementSize;

			result = GetRenderer()->GetDevice()->CreateCommittedResource(&heapPropertie,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_constantBuffersToPS[i]));
			if (FAILED(result))
			{

#ifdef _DEBUG
				MessageBox(nullptr, TEXT("Failed Create by PixelConstantBuffer."), TEXT("Failed"), MB_OK);
#endif
				return;
			}

			m_constantBuffersToPS[i]->Map(0, nullptr, (void**)(&m_pixelConstantBuffers[i]));
			cbv.BufferLocation = m_constantBuffersToPS[i]->GetGPUVirtualAddress();
			device->CreateConstantBufferView(&cbv, heapCPUHandle);
			heapCPUHandle.ptr += m_heapIncrementSize;

		}
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
	}

	void Shader::SetPixelConstantBufferSize(int32_t size)
	{
		m_pixelBufferSize = (size + 0xff) & ~0xff;
	}

	void Shader::SetConstantBuffer()
	{
		auto gpuHandle = m_constantBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		gpuHandle.ptr += m_heapIncrementSize * m_bufferIndex * 2;

		if (m_vertexRegisterCount > 0)
		{
			// デスクリプタをコマンドに積む処理
			GetRenderer()->GetCommandList()->SetGraphicsRootDescriptorTable(4, gpuHandle);
		}

		gpuHandle.ptr += m_heapIncrementSize;

		if (m_pixelRegisterCount > 0)
		{
			// デスクリプタをコマンドに積む処理
			GetRenderer()->GetCommandList()->SetGraphicsRootDescriptorTable(5, gpuHandle);
		}
		++m_bufferIndex;
	}

	void Shader::ResetConstantBuffer()
	{
		m_bufferIndex = 0;
		m_textureResourceIndex = 0;
	}

	void Shader::SetTextureData(EffekseerRendererDX12::TextureData * textureData, int32_t rootParameterOffset)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = textureData->GetShaderResourceView();

		auto cpuHandle = m_shaderResourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		auto gpuHandle = m_shaderResourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += m_heapIncrementSize * m_textureResourceIndex;
		gpuHandle.ptr += m_heapIncrementSize * m_textureResourceIndex;

		GetRenderer()->GetDevice()->CreateShaderResourceView(textureData->GetResource(), &srvDesc, cpuHandle);
		GetRenderer()->GetCommandList()->SetGraphicsRootDescriptorTable(m_srvStartRootParamIdx + rootParameterOffset, gpuHandle);
		++m_textureResourceIndex;
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