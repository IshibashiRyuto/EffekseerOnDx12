#include "EffekseerRendererDX12.RenderState.h"
#include "EffekseerRendererDX12.Shader.h"



namespace EffekseerRendererDX12
{
	namespace Standard_VS
	{
#include "Shader/EffekseerRenderer.Standard_VS.h"
	}

	namespace Standard_PS
	{
#include "Shader/EffekseerRenderer.Standard_PS.h"
	}


	bool RenderState::EqualPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC & ps1, const D3D12_GRAPHICS_PIPELINE_STATE_DESC & ps2)
	{
		if (memcmp(&ps1, &ps2, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)) == 0)
		{
			return true;
		}
		return false;
	}

	RenderState::RenderState(RendererImplemented* renderer, D3D12_COMPARISON_FUNC depthFunc)
		: m_renderer(renderer)
		, m_pipelineStateNum(0)
	{
		// カリングモードのテーブル作成
		D3D12_CULL_MODE cullTbl[] =
		{
			D3D12_CULL_MODE_BACK,
			D3D12_CULL_MODE_FRONT,
			D3D12_CULL_MODE_NONE,
		};

		// ラスタライザデスクの作成
		for (int32_t ct = 0; ct < CulTypeCount; ++ct)
		{
			ZeroMemory(&m_rStates[ct], sizeof(D3D12_RASTERIZER_DESC));
			m_rStates[ct].CullMode = cullTbl[ct];
			m_rStates[ct].FillMode = D3D12_FILL_MODE_SOLID;
			m_rStates[ct].DepthClipEnable = TRUE;
		}

		// デプスステンシルデスクの作成
		for (int32_t dt = 0; dt < DepthTestCount; ++dt)
		{
			for (int32_t dw = 0; dw < DepthWriteCount; ++dw)
			{
				ZeroMemory(&m_dStates[dt][dw], sizeof(D3D12_DEPTH_STENCIL_DESC));
				m_dStates[dt][dw].DepthEnable = dt;
				m_dStates[dt][dw].DepthWriteMask = (D3D12_DEPTH_WRITE_MASK)dw;
				m_dStates[dt][dw].DepthFunc = depthFunc;
				m_dStates[dt][dw].StencilEnable = FALSE;
			}
		}

		// ブレンドデスクの生成
		for (int32_t i = 0; i < AlphaTypeCount; ++i)
		{
			auto type = (::Effekseer::AlphaBlendType)i;
			ZeroMemory(&m_bStates[i], sizeof(D3D12_BLEND_DESC));
			m_bStates[i].AlphaToCoverageEnable = false;

			for (int32_t k = 0; k < 8; ++k)
			{
				m_bStates[i].RenderTarget[k].BlendEnable = type != ::Effekseer::AlphaBlendType::Opacity;
				m_bStates[i].RenderTarget[k].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
				m_bStates[i].RenderTarget[k].SrcBlendAlpha = D3D12_BLEND_ONE;
				m_bStates[i].RenderTarget[k].DestBlendAlpha = D3D12_BLEND_ONE;
				m_bStates[i].RenderTarget[k].BlendOpAlpha = D3D12_BLEND_OP_MAX;
				switch (type)
				{
				case ::Effekseer::AlphaBlendType::Opacity:
					m_bStates[i].RenderTarget[k].DestBlend = D3D12_BLEND_ZERO;
					m_bStates[i].RenderTarget[k].SrcBlend = D3D12_BLEND_ONE;
					m_bStates[i].RenderTarget[k].BlendOp = D3D12_BLEND_OP_ADD;
					break;
				case ::Effekseer::AlphaBlendType::Blend:
					m_bStates[i].RenderTarget[k].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
					m_bStates[i].RenderTarget[k].SrcBlend = D3D12_BLEND_SRC_ALPHA;
					m_bStates[i].RenderTarget[k].BlendOp = D3D12_BLEND_OP_ADD;
					break;
				case ::Effekseer::AlphaBlendType::Add:
					m_bStates[i].RenderTarget[k].DestBlend = D3D12_BLEND_ONE;
					m_bStates[i].RenderTarget[k].SrcBlend = D3D12_BLEND_SRC_ALPHA;
					m_bStates[i].RenderTarget[k].BlendOp = D3D12_BLEND_OP_ADD;
					break;
				case ::Effekseer::AlphaBlendType::Sub:
					m_bStates[i].RenderTarget[k].DestBlend = D3D12_BLEND_ONE;
					m_bStates[i].RenderTarget[k].SrcBlend = D3D12_BLEND_SRC_ALPHA;
					m_bStates[i].RenderTarget[k].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
					break;
				case ::Effekseer::AlphaBlendType::Mul:
					m_bStates[i].RenderTarget[k].DestBlend = D3D12_BLEND_SRC_COLOR;
					m_bStates[i].RenderTarget[k].SrcBlend = D3D12_BLEND_ZERO;
					m_bStates[i].RenderTarget[k].BlendOp = D3D12_BLEND_OP_ADD;
					break;
				}
			}
		}

		// サンプラ用デスクリプタヒープの作成
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc{};
		samplerHeapDesc.NumDescriptors = TextureWrapCount* TextureFilterCount;
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		renderer->GetDevice()->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap));

		D3D12_SAMPLER_DESC samplerDesc[TextureFilterCount][TextureWrapCount];
		D3D12_CPU_DESCRIPTOR_HANDLE descCpuHandle = m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE descGpuHandle = m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		// サンプラの作成
		for (int32_t f = 0; f < TextureFilterCount; ++f)
		{
			for (int32_t w = 0; w < TextureWrapCount; ++w)
			{
				D3D12_TEXTURE_ADDRESS_MODE Addres[] =
				{
					D3D12_TEXTURE_ADDRESS_MODE_WRAP,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				};

				D3D12_FILTER Filter[] = 
				{
					D3D12_FILTER_MIN_MAG_MIP_POINT,
					D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				};

				uint32_t Anisotropic[] =
				{
					0,0,
				};
				float borderColor[] = { 0.0f,0.0f,0.0f,0.0f };
				samplerDesc[f][w].Filter = Filter[f];
				samplerDesc[f][w].AddressU = Addres[w];
				samplerDesc[f][w].AddressV = Addres[w];
				samplerDesc[f][w].AddressW = Addres[w];
				samplerDesc[f][w].MipLODBias = 0.0f;
				samplerDesc[f][w].MaxAnisotropy = Anisotropic[f];
				samplerDesc[f][w].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
				samplerDesc[f][w].BorderColor[0] = 0.0f;
				samplerDesc[f][w].BorderColor[1] = 0.0f;
				samplerDesc[f][w].BorderColor[2] = 0.0f;
				samplerDesc[f][w].BorderColor[3] = 0.0f;
				samplerDesc[f][w].MinLOD = 0.0f;
				samplerDesc[f][w].MaxLOD = D3D12_FLOAT32_MAX;

				m_sState[f][w] = descGpuHandle;
				renderer->GetDevice()->CreateSampler(&samplerDesc[f][w], descCpuHandle);
				descCpuHandle.ptr += renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
				descGpuHandle.ptr += renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			}
		}



		// パイプラインステートデスクの初期化
		m_pipelineState.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		m_pipelineState.pRootSignature = nullptr;
		m_pipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_pipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_pipelineState.NumRenderTargets = 1;
		m_pipelineState.SampleDesc.Count = 1;
		m_pipelineState.SampleDesc.Quality = 0;
		m_pipelineState.SampleMask = UINT_MAX;

		// 仮のシェーダーをセット
		m_pipelineState.VS.BytecodeLength = sizeof(Standard_VS::g_VS);
		m_pipelineState.VS.pShaderBytecode = Standard_VS::g_VS;
		m_pipelineState.PS.BytecodeLength = sizeof(Standard_PS::g_PS);
		m_pipelineState.PS.pShaderBytecode = Standard_PS::g_PS;

		
	}


	RenderState::~RenderState()
	{
	}

	void RenderState::Update(bool forced)
	{
		bool changeDepth = forced;
		bool changeRasterizer = forced;
		bool changeBlend = forced;
		bool changeSamplerEither = forced;

		// デプスステンシルステートの変更
		if (m_active.DepthTest != m_next.DepthTest || forced)
		{
			changeDepth = true;
		}

		if (m_active.DepthWrite != m_next.DepthWrite || forced)
		{
			changeDepth = true;
		}

		if (changeDepth)
		{
			m_pipelineState.DepthStencilState = m_dStates[m_next.DepthTest][m_next.DepthWrite];
		}

		// ラスタライザステートの変更
		if (m_active.CullingType != m_next.CullingType || forced)
		{
			changeRasterizer = true;
		}

		if (changeRasterizer)
		{
			auto cullingType = (int32_t)m_next.CullingType;
			m_pipelineState.RasterizerState = m_rStates[cullingType];
		}

		// ブレンドステートの変更
		if (m_active.AlphaBlend != m_next.AlphaBlend || forced)
		{
			changeBlend = true;
		}

		if (changeBlend)
		{
			auto alphaBlend = (int32_t)m_next.AlphaBlend;
			m_pipelineState.BlendState = m_bStates[alphaBlend];
			float blendFactor[] = { 0.0f,0.0f,0.0f,0.0f };
			m_renderer->GetCommandList()->OMSetBlendFactor(blendFactor);
		}

		for (int32_t i = 0; i < 4; ++i)
		{
			bool changeSampler = forced;
			if (m_active.TextureFilterTypes[i] != m_next.TextureFilterTypes[i] || forced)
			{
				changeSampler = true;
			}

			if (m_active.TextureWrapTypes[i] != m_next.TextureWrapTypes[i] || forced)
			{
				changeSampler = true;
			}

			if (changeSampler)
			{
				changeSamplerEither = true;
				auto filter = (int32_t)m_next.TextureFilterTypes[i];
				auto wrap = (int32_t)m_next.TextureWrapTypes[i];

				// サンプラ変更処理
				m_samplerDescriptorHandle[i] = m_sState[filter][wrap];
			}
		}

		// パイプラインステートの変更をコマンドに書き出し
		if (changeDepth || changeRasterizer || changeBlend || changeSamplerEither)
		{
			auto state = GetPipelineState();
			if (state != nullptr)
			{
				//m_renderer->GetCommandList()->SetGraphicsRootSignature(m_rootSignature.Get());
				m_renderer->GetCommandList()->SetGraphicsRootSignature(m_pipelineState.pRootSignature);
				m_renderer->GetCommandList()->SetPipelineState(state);
			}
		}
	}

	ID3D12PipelineState * RenderState::GetPipelineState()
	{
		int key = -1;
		for (int i = 0; i < m_pipelineStateNum; ++i)
		{
			if (EqualPipelineState(m_pipelineState, m_pipelineStateDescVector[i]))
			{
				key = i;
				break;
			}
		}
		if (key == -1)
		{
			if (m_pipelineState.pRootSignature == nullptr)
			{
				return nullptr;
			}
			ComPtr<ID3D12PipelineState> pso;
			auto hr = m_renderer->GetDevice()->CreateGraphicsPipelineState(&m_pipelineState, IID_PPV_ARGS(&pso));
			if (FAILED(hr))
			{
#ifdef _DEBUG
				MessageBox(nullptr, TEXT("Failed Create PipelineStateObject."), TEXT("Failed"), MB_OK);
				return nullptr;
#endif
			}
			key = m_pipelineStateNum;
			++m_pipelineStateNum;

			m_pipelineStateDescVector.resize(m_pipelineStateNum);
			m_pipelineStateDescVector[key] = m_pipelineState;
			m_pipelineStateMap[key] = pso;
			return pso.Get();
		}
		return m_pipelineStateMap[key].Get();
	}

	void RenderState::ChangeShader(Shader * shader)
	{
		m_pipelineState.VS = shader->GetVertexShader();
		m_pipelineState.PS = shader->GetPixelShader();
		m_pipelineState.InputLayout = shader->GetLayoutInterface();
		m_constantBufferDescriptorHeap = shader->GetDescriptorHeap();
		m_pipelineState.pRootSignature = shader->GetRootSignature();
	}

	void RenderState::SetDescriptorHeap()
	{
		ID3D12DescriptorHeap* ppHeaps[] = { m_samplerDescriptorHeap.Get(),m_constantBufferDescriptorHeap.Get() };
		if (m_constantBufferDescriptorHeap != nullptr && m_samplerDescriptorHeap != nullptr)
		{
			m_renderer->GetCommandList()->SetDescriptorHeaps(2, ppHeaps);
			for (int i = 0; i < 4; ++i)
			{
				m_renderer->GetCommandList()->SetGraphicsRootDescriptorTable(i, m_samplerDescriptorHandle[i]);
			}
		}
	}

	

}