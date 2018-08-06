
#include "EffekseerRendererDX12.Renderer.h"
#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererDX12.RenderState.h"

#include "EffekseerRendererDX12.Shader.h"
#include "EffekseerRendererDX12.VertexBuffer.h"
#include "EffekseerRendererDX12.IndexBuffer.h"
#include "EffekseerRendererDX12.DeviceObject.h"
#include "EffekseerRendererDX12.ModelRenderer.h"
#include "EffekseerRendererDX12.TextureLoader.h"
#include "EffekseerRendererDX12.ModelLoader.h"

#include "../EffekseerRendererCommon/EffekseerRenderer.SpriteRendererBase.h"
#include "../EffekseerRendererCommon/EffekseerRenderer.RibbonRendererBase.h"
#include "../EffekseerRendererCommon/EffekseerRenderer.RingRendererBase.h"
#include "../EffekseerRendererCommon/EffekseerRenderer.TrackRendererBase.h"

#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
#include "../EffekseerRendererCommon/EffekseerRenderer.PngTextureLoader.h"
#endif


//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
namespace EffekseerRendererDX12
{

	namespace Standard_VS
	{
#include "Shader/EffekseerRenderer.Standard_VS.h"
	}

	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace Standard_PS
	{
#include "Shader/EffekseerRenderer.Standard_PS.h"
	}

	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace StandardNoTexture_PS
	{
#include "Shader/EffekseerRenderer.StandardNoTexture_PS.h"
	}

	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace Standard_Distortion_VS
	{
#include "Shader/EffekseerRenderer.Standard_Distortion_VS.h"
	}

	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace Standard_Distortion_PS
	{
#include "Shader/EffekseerRenderer.Standard_Distortion_PS.h"
	}

	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace StandardNoTexture_Distortion_PS
	{
#include "Shader/EffekseerRenderer.StandardNoTexture_Distortion_PS.h"
	}


	::Effekseer::TextureLoader* CreateTextureLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface)
	{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
		return new TextureLoader(device, fileInterface);
#else
		return nullptr;
#endif
	}

	::Effekseer::ModelLoader* CreateModelLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface)
	{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
		return new ModelLoader(device, fileInterface);
#else
		return nullptr;
#endif
	}
	
	Renderer* Renderer::Create(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,//ID3D11DeviceContext context
		int32_t squareMaxCount,
		D3D12_COMPARISON_FUNC depthFunc)
	{
		RendererImplemented* renderer = new RendererImplemented(squareMaxCount);
		if (renderer->Initialize(device,commandList,depthFunc))
		{
			return renderer;
		}
		return nullptr;
	}

	RendererImplemented::RendererImplemented(int32_t squareMaxCount)
		: m_device(nullptr)
		//, m_context( nullptr)
		, m_commandList(nullptr)
		, m_vertexBuffer(nullptr)
		, m_indexBuffer(nullptr)
		, m_squareMaxCount(squareMaxCount)
		, m_coordinateSystem(::Effekseer::CoordinateSystem::RH)
		, m_renderState(nullptr)
		, m_restorationOfStates(true)
		
		, m_shader(nullptr)
		, m_shader_no_texture(nullptr)
		, m_shader_distortion(nullptr)
		, m_shader_no_texture_distortion(nullptr)
		, m_standardRenderer(nullptr)
		, m_distortingCallback(nullptr)
	{
		::Effekseer::Vector3D direction(1.0f, 1.0f, 1.0f);
		SetLightDirection(direction);
		::Effekseer::Color lightColor(255, 255, 255, 255);
		SetLightColor(lightColor);
		::Effekseer::Color lightAmbient(0, 0, 0, 0);
		SetLightAmbientColor(lightAmbient);

		m_background.UserPtr = nullptr;

#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
		EffekseerRenderer::PngTextureLoader::Initialize();
#endif
	}

	RendererImplemented::~RendererImplemented()
	{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
		EffekseerRenderer::PngTextureLoader::Finalize();
#endif

		assert(GetRef() == 0);

		ES_SAFE_DELETE(m_distortingCallback);

		auto p = (ID3D12Resource*)m_background.UserPtr;
		ES_SAFE_RELEASE(p);

		ES_SAFE_DELETE(m_standardRenderer);
		ES_SAFE_DELETE(m_shader);
		ES_SAFE_DELETE(m_shader_no_texture);
		
		ES_SAFE_DELETE(m_shader_distortion);
		ES_SAFE_DELETE(m_shader_no_texture_distortion);

		
		ES_SAFE_DELETE(m_renderState);
		ES_SAFE_DELETE(m_vertexBuffer);
		ES_SAFE_DELETE(m_indexBuffer);

		assert(GetRef() == -6);
	}

	void RendererImplemented::OnLostDevice()
	{
		for (auto& device : m_deviceObjects)
		{
			device->OnLostDevice();
		}
	}

	void RendererImplemented::OnResetDevice()
	{
		for (auto& device : m_deviceObjects)
		{
			device->OnResetDevice();
		}
	}

	bool RendererImplemented::Initialize(ID3D12Device* device,ID3D12GraphicsCommandList* commandList, D3D12_COMPARISON_FUNC depthFunc)
	{
		m_device = device;
		m_commandList = commandList;
		m_depthFunc = depthFunc;

		// 頂点の生成
		{
			// 最大でfloat*10と仮定
			m_vertexBuffer = VertexBuffer::Create(this, sizeof(float) * 10 * m_squareMaxCount * 4, true);
			if (m_vertexBuffer == nullptr)
			{
				return false;
			}
		}

		// 参照カウントの調整
		Release();

		// インデックスの生成
		{
			m_indexBuffer = IndexBuffer::Create(this, m_squareMaxCount * 6, false);
			if (m_indexBuffer == nullptr)
			{
				return false;
			}

			m_indexBuffer->Lock();

			// 標準設定で DirectX 時計回りが表, OpenGLは反時計回りが表
			for (int i = 0; i < m_squareMaxCount; ++i)
			{
				uint16_t* buf = (uint16_t*)m_indexBuffer->GetBufferDirect(6);
				buf[0] = 3 + 4 * i;
				buf[1] = 1 + 4 * i;
				buf[2] = 0 + 4 * i;
				buf[3] = 3 + 4 * i;
				buf[4] = 0 + 4 * i;
				buf[5] = 2 + 4 * i;
			}

			m_indexBuffer->Unlock();
		}
		// 参照カウントの調整
		Release();

		m_renderState = new RenderState(this, m_depthFunc);


		// シェーダ
		// 座標(3) 色(1) UV(2)
		D3D12_INPUT_ELEMENT_DESC decl[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float)*3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float)*4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		D3D12_INPUT_ELEMENT_DESC decl_distortion[]=
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 6, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 9, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		m_shader = Shader::Create(
			this,
			Standard_VS::g_VS,
			sizeof(Standard_VS::g_VS),
			Standard_PS::g_PS,
			sizeof(Standard_PS::g_PS),
			"StandardRenderer", decl, ARRAYSIZE(decl));

		if (m_shader == nullptr)
		{
			return false;
		}


		// 参照カウントの調整
		Release();

		m_shader_no_texture = Shader::Create(
			this,
			Standard_VS::g_VS,
			sizeof(Standard_VS::g_VS),
			StandardNoTexture_PS::g_PS,
			sizeof(StandardNoTexture_PS::g_PS),
			"StandardRenderer No Texture",
			decl, ARRAYSIZE(decl));

		if (m_shader_no_texture == nullptr)
		{
			return false;
		}

		// 参照カウントの調整
		Release();

		m_shader_distortion = Shader::Create(
			this,
			Standard_Distortion_VS::g_VS,
			sizeof(Standard_Distortion_VS::g_VS),
			Standard_Distortion_PS::g_PS,
			sizeof(Standard_Distortion_PS::g_PS),
			"StandardRenderer Distortion", decl_distortion, ARRAYSIZE(decl_distortion));
		if (m_shader_distortion == nullptr)
		{
			return false;
		}

		// 参照カウントの調整
		Release();

		m_shader_no_texture_distortion = Shader::Create(
			this,
			Standard_Distortion_VS::g_VS,
			sizeof(Standard_Distortion_VS::g_VS),
			StandardNoTexture_Distortion_PS::g_PS,
			sizeof(StandardNoTexture_Distortion_PS::g_PS),
			"StandardRenderer No Texture Distortion",
			decl_distortion, ARRAYSIZE(decl_distortion));
		if (m_shader_no_texture_distortion == nullptr)
		{
			return false;
		}

		// 参照カウントの調整
		Release();

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
			srvRootParam1
		};

		m_shader->SetVertexConstantBufferSize(sizeof(Effekseer::Matrix44) * 2);
		m_shader->SetVertexRegisterCount(8);
		m_shader->CreateRootSignature(shader_rootParams,6);
		m_shader->SetShaderResourceViewStartRootParamIdx(5);

		D3D12_ROOT_PARAMETER shader_no_texture_rootParams[] = {
			samplerRootParam1,
			samplerRootParam2,
			samplerRootParam3,
			samplerRootParam4,
			vertexcbRootParam,
		};
		m_shader_no_texture->SetVertexConstantBufferSize(sizeof(Effekseer::Matrix44) * 2);
		m_shader_no_texture->SetVertexRegisterCount(8);
		m_shader_no_texture->CreateRootSignature(shader_no_texture_rootParams,5);

		D3D12_ROOT_PARAMETER shader_distortion_rootParams[] = {
			samplerRootParam1,
			samplerRootParam2,
			samplerRootParam3,
			samplerRootParam4,
			vertexcbRootParam,
			pixelcbRootParam,
			srvRootParam1,
			srvRootParam2
		};
		m_shader_distortion->SetVertexConstantBufferSize(sizeof(Effekseer::Matrix44) * 2);
		m_shader_distortion->SetVertexRegisterCount(8);
		m_shader_distortion->SetPixelConstantBufferSize(sizeof(float) * 4);
		m_shader_distortion->SetPixelRegisterCount(1);
		m_shader_distortion->CreateRootSignature(shader_distortion_rootParams,8);
		m_shader_distortion->SetShaderResourceViewStartRootParamIdx(6);


		D3D12_ROOT_PARAMETER shader_no_texture_distortion_rootParams[] = {
			samplerRootParam1,
			samplerRootParam2,
			samplerRootParam3,
			samplerRootParam4,
			vertexcbRootParam,
			pixelcbRootParam,
			srvRootParam1,
		};
		m_shader_no_texture_distortion->SetVertexConstantBufferSize(sizeof(Effekseer::Matrix44) * 2);
		m_shader_no_texture_distortion->SetVertexRegisterCount(8);
		m_shader_no_texture_distortion->SetPixelConstantBufferSize(sizeof(float) * 4);
		m_shader_no_texture_distortion->SetPixelRegisterCount(1);
		m_shader_no_texture_distortion->CreateRootSignature(shader_no_texture_distortion_rootParams,7);
		m_shader_no_texture_distortion->SetShaderResourceViewStartRootParamIdx(6);

		m_standardRenderer = new EffekseerRenderer::StandardRenderer<RendererImplemented, Shader, Vertex, VertexDistortion>(
			this, m_shader, m_shader_no_texture, m_shader_distortion, m_shader_no_texture_distortion);

		return true;
	}

	void RendererImplemented::Destroy()
	{
		Release();
	}

	void RendererImplemented::SetRestorationOfStatesFlag(bool flag)
	{
		m_restorationOfStates = flag;
	}

	bool RendererImplemented::BeginRendering()
	{
		assert(m_device != nullptr);

		::Effekseer::Matrix44::Mul(m_cameraProj, m_camera, m_proj);


		// ステート初期設定
		m_renderState->GetActiveState().Reset();
		m_renderState->Update( true );

		//シェーダコンスタントバッファのリセット
		m_shader->ResetConstantBuffer();
		m_shader_no_texture->ResetConstantBuffer();
		m_shader_distortion->ResetConstantBuffer();
		m_shader_no_texture_distortion->ResetConstantBuffer();

		// レンダラーリセット
		m_standardRenderer->ResetAndRenderingIfRequired();

		return true;
	}

	bool RendererImplemented::EndRendering()
	{
		assert(m_device != nullptr);

		// レンダラーリセット
		m_standardRenderer->ResetAndRenderingIfRequired();

		return true;
	}

	ID3D12Device* RendererImplemented::GetDevice()
	{
		return m_device;
	}

	ID3D12GraphicsCommandList* RendererImplemented::GetCommandList()
	{
		return m_commandList;
	}

	VertexBuffer* RendererImplemented::GetVertexBuffer()
	{
		return m_vertexBuffer;
	}

	IndexBuffer* RendererImplemented::GetIndexBuffer()
	{
		return m_indexBuffer;
	}

	int32_t RendererImplemented::GetSquareMaxCount() const
	{
		return m_squareMaxCount;
	}

	::EffekseerRenderer::RenderStateBase* RendererImplemented::GetRenderState()
	{
		return m_renderState;
	}

	const ::Effekseer::Vector3D& RendererImplemented::GetLightDirection() const
	{
		return m_lightDirection;
	}

	void RendererImplemented::SetLightDirection(::Effekseer::Vector3D& direction)
	{
		m_lightDirection = direction;
	}

	const ::Effekseer::Color& RendererImplemented::GetLightColor() const
	{
		return m_lightColor;
	}

	void RendererImplemented::SetLightColor(::Effekseer::Color& color)
	{
		m_lightColor = color;
	}

	const ::Effekseer::Color& RendererImplemented::GetLightAmbientColor() const
	{
		return m_lightAmbient;
	}

	void RendererImplemented::SetLightAmbientColor(::Effekseer::Color& color)
	{
		m_lightAmbient = color;
	}

	const ::Effekseer::Matrix44& RendererImplemented::GetProjectionMatrix()const
	{
		return m_proj;
	}

	void RendererImplemented::SetProjectionMatrix(const ::Effekseer::Matrix44& mat)
	{
		m_proj = mat;
	}

	const ::Effekseer::Matrix44& RendererImplemented::GetCameraMatrix() const
	{
		return m_camera;
	}

	void RendererImplemented::SetCameraMatrix(const ::Effekseer::Matrix44& mat)
	{
		m_camera = mat;
	}

	::Effekseer::Matrix44& RendererImplemented::GetCameraProjectionMatrix()
	{
		return m_cameraProj;
	}


	::Effekseer::SpriteRenderer* RendererImplemented::CreateSpriteRenderer()
	{
		return new ::EffekseerRenderer::SpriteRendererBase<RendererImplemented, Vertex, VertexDistortion>(this);
	}

	::Effekseer::RibbonRenderer* RendererImplemented::CreateRibbonRenderer()
	{
		return new ::EffekseerRenderer::RibbonRendererBase<RendererImplemented, Vertex, VertexDistortion>(this);
	}


	::Effekseer::RingRenderer* RendererImplemented::CreateRingRenderer()
	{
		return new ::EffekseerRenderer::RingRendererBase<RendererImplemented, Vertex, VertexDistortion>(this);
	}

	::Effekseer::ModelRenderer* RendererImplemented::CreateModelRenderer()
	{
		return ModelRenderer::Create(this);
	}

	::Effekseer::TrackRenderer*RendererImplemented::CreateTrackRenderer()
	{
		return new ::EffekseerRenderer::TrackRendererBase<RendererImplemented, Vertex, VertexDistortion>(this);
	}

	::Effekseer::TextureLoader* RendererImplemented::CreateTextureLoader(::Effekseer::FileInterface* fileInterface)
	{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
		return new TextureLoader(this->GetDevice(), fileInterface);
#else
		return nullptr;
#endif
	}

	::Effekseer::ModelLoader* RendererImplemented::CreateModelLoader(::Effekseer::FileInterface * fileInterface)
	{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER_
		return new ModelLoader(this->GetDevice(), fileInterface);
#else
		return nullptr;
#endif
	}

	void RendererImplemented::SetBackground(ID3D12Resource* background)
	{
		/*
		ES_SAFE_ADDREF(background);
		auto p = (ID3D12Resource*)m_background.UserPtr;
		ES_SAFE_RELEASE(p);
		m_background.UserPtr = background;
		*/
		auto p = (TextureData*)m_background.UserPtr;
		ES_SAFE_DELETE(p);
		m_background.UserPtr = TextureData::Create(background);
	}

	EffekseerRenderer::DistortingCallback* RendererImplemented::GetDistortingCallback()
	{
		return m_distortingCallback;
	}

	void RendererImplemented::SetDistortingCallback(EffekseerRenderer::DistortingCallback * callback)
	{
		ES_SAFE_DELETE(m_distortingCallback);
		m_distortingCallback = callback;
	}

	void RendererImplemented::SetVertexBuffer(ID3D12Resource* vertexBuffer, int32_t size)
	{
		ID3D12Resource* vBuf = vertexBuffer;
		uint32_t vertexSize = size;
		uint32_t offset = 0;
	}

	void RendererImplemented::SetVertexBuffer(VertexBuffer* vertexBuffer, int32_t size)
	{
		auto vertexBufferView = vertexBuffer->GetInterface();
		vertexBufferView->StrideInBytes = size;
		GetCommandList()->IASetVertexBuffers(0, 1, vertexBufferView);
	}

	void RendererImplemented::SetIndexBuffer(IndexBuffer* indexBuffer)
	{
		GetCommandList()->IASetIndexBuffer(indexBuffer->GetInterface());
	}

	void RendererImplemented::SetIndexBuffer(ID3D12Resource* indexBuffer)
	{
	}

	void RendererImplemented::SetLayout(Shader* shader)
	{
		GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void RendererImplemented::DrawSprites(int32_t spriteCount, int32_t vertexOffset)
	{
		GetCommandList()->DrawIndexedInstanced(spriteCount * 2 * 3, 1, 0, vertexOffset, 0);
	}

	void RendererImplemented::DrawPolygon(int32_t vertexCount, int32_t indexCount)
	{
		GetCommandList()->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
	}

	void RendererImplemented::BeginShader(Shader* shader)
	{
		auto renderState = dynamic_cast<RenderState*>(m_renderState);
		if (renderState != nullptr)
		{
			renderState->ChangeShader(shader);
			auto pipelineState = renderState->GetPipelineState();
			GetCommandList()->SetPipelineState(pipelineState);
			GetCommandList()->SetGraphicsRootSignature(shader->GetRootSignature());
			renderState->SetDescriptorHeap();
		}
	}

	void RendererImplemented::EndShader(Shader* shader)
	{
		
	}

	void RendererImplemented::SetTextures(Shader* shader, Effekseer::TextureData** textures, int32_t count)
	{

		for (int32_t i = 0; i < count; ++i)
		{
			if (textures[i] != nullptr)
			{
				TextureData* texture = (TextureData*)textures[i]->UserPtr;
				shader->SetTextureData(texture,i);
			}
		}
	}

	void RendererImplemented::ResetRenderState()
	{
		m_renderState->GetActiveState().Reset();
		m_renderState->Update(true);
	}
}