#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererDX12.RenderState.h"

#include "EffekseerRendererDX12.VertexBuffer.h"
#include "EffekseerRendererDX12.IndexBuffer.h"
#include "EffekseerRendererDX12.ModelRenderer.h"
#include "EffekseerRendererDX12.Shader.h"

namespace EffekseerRendererDX12
{
	namespace ShaderLightingTextureNormal_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLightingTextureNormal_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLightingTextureNormal_PS.h"

	}
	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace ShaderLightingNormal_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLightingNormal_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLightingNormal_PS.h"
	}
	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace ShaderLightingTexture_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLightingTexture_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLightingTexture_PS.h"
	}
	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace ShaderLighting_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLighting_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.ShaderLighting_PS.h"
	}
	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace ShaderTexture_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.ShaderTexture_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.ShaderTexture_PS.h"
	}
	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace Shader_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.Shader_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.Shader_PS.h"
	}

	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace ShaderDistortionTexture_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.ShaderDistortion_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.ShaderDistortionTexture_PS.h"
	}

	//-----------------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------------
	namespace ShaderDistortion_
	{
#include "Shader/EffekseerRenderer.ModelRenderer.ShaderDistortion_VS.h"

#include "Shader/EffekseerRenderer.ModelRenderer.ShaderDistortion_PS.h"
	}

	ModelRenderer::ModelRenderer(RendererImplemented* renderer,
		Shader* shader_lighting_texture_normal,
		Shader* shader_lighting_normal,
		Shader* shader_lighting_texture,
		Shader* shader_lighting,
		Shader* shader_texture,
		Shader* shader,
		Shader* shader_distortion_texture,
		Shader* shader_distortion)
		: m_renderer(renderer)
		, m_shader_lighting_texture_normal(shader_lighting_texture_normal)
		, m_shader_lighting_normal(shader_lighting_normal)
		, m_shader_lighting_texture(shader_lighting_texture)
		, m_shader_lighting(shader_lighting)
		, m_shader_texture(shader_texture)
		, m_shader(shader)
		, m_shader_distortion_texture(shader_distortion_texture)
		, m_shader_distortion(shader_distortion)
	{

		Shader* shaders[6];
		shaders[0] = m_shader_lighting_texture_normal;
		shaders[1] = m_shader_lighting_normal;
		shaders[2] = m_shader_lighting_texture;
		shaders[3] = m_shader_lighting;
		shaders[4] = m_shader_texture;
		shaders[5] = m_shader;

		for (int32_t i = 0; i < 6; i++)
		{
			shaders[i]->SetVertexConstantBufferSize(sizeof(::EffekseerRenderer::ModelRendererVertexConstantBuffer<40>));
			shaders[i]->SetVertexRegisterCount(sizeof(::EffekseerRenderer::ModelRendererVertexConstantBuffer<40>) / (sizeof(float) * 4));
			shaders[i]->SetPixelConstantBufferSize(sizeof(::EffekseerRenderer::ModelRendererPixelConstantBuffer));
			shaders[i]->SetPixelRegisterCount(sizeof(::EffekseerRenderer::ModelRendererPixelConstantBuffer) / (sizeof(float) * 4));
		}

		m_shader_distortion_texture->SetVertexConstantBufferSize(sizeof(::EffekseerRenderer::ModelRendererVertexConstantBuffer<40>));
		m_shader_distortion_texture->SetVertexRegisterCount(sizeof(::EffekseerRenderer::ModelRendererVertexConstantBuffer<40>) / (sizeof(float) * 4));
		m_shader_distortion_texture->SetPixelConstantBufferSize(sizeof(float) * 4);
		m_shader_distortion_texture->SetPixelRegisterCount(1);

		m_shader_distortion->SetVertexConstantBufferSize(sizeof(::EffekseerRenderer::ModelRendererVertexConstantBuffer<40>));
		m_shader_distortion->SetVertexRegisterCount(sizeof(::EffekseerRenderer::ModelRendererVertexConstantBuffer<40>) / (sizeof(float) * 4));
		m_shader_distortion->SetPixelConstantBufferSize(sizeof(float) * 4);
		m_shader_distortion->SetPixelRegisterCount(1);
	}

	ModelRenderer::~ModelRenderer()
	{
		ES_SAFE_DELETE(m_shader_lighting_texture_normal);
		ES_SAFE_DELETE(m_shader_lighting_normal);
		ES_SAFE_DELETE(m_shader_lighting_texture);
		ES_SAFE_DELETE(m_shader_lighting);
		ES_SAFE_DELETE(m_shader_texture);
		ES_SAFE_DELETE(m_shader);

		ES_SAFE_DELETE(m_shader_distortion_texture);
		ES_SAFE_DELETE(m_shader_distortion);
	}

	ModelRenderer* ModelRenderer::Create(RendererImplemented* renderer)
	{
		assert(renderer != nullptr);
		assert(renderer->GetDevice() != nullptr);

		D3D12_INPUT_ELEMENT_DESC decl[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 6, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "NORMAL", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 9, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "NORMAL", 3, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 14, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{ "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, sizeof(float) * 15, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		Shader* shader_lighting_texture_normal = Shader::Create(
			renderer,
			ShaderLightingTextureNormal_::g_VS,
			sizeof(ShaderLightingTextureNormal_::g_VS),
			ShaderLightingTextureNormal_::g_PS,
			sizeof(ShaderLightingTextureNormal_::g_PS),
			"ModelRendererLightingTextureNormal",
			decl,
			ARRAYSIZE(decl));


		Shader* shader_lighting_normal = Shader::Create(
			renderer,
			ShaderLightingNormal_::g_VS,
			sizeof(ShaderLightingNormal_::g_VS),
			ShaderLightingNormal_::g_PS,
			sizeof(ShaderLightingNormal_::g_PS),
			"ModelRendererLightingNormal",
			decl,
			ARRAYSIZE(decl));

		Shader* shader_lighting_texture = Shader::Create(
			renderer,
			ShaderLightingTexture_::g_VS,
			sizeof(ShaderLightingTexture_::g_VS),
			ShaderLightingTexture_::g_PS,
			sizeof(ShaderLightingTexture_::g_PS),
			"ModelRendererLightingTexture",
			decl,
			ARRAYSIZE(decl));

		Shader* shader_lighting = Shader::Create(
			renderer,
			ShaderLighting_::g_VS,
			sizeof(ShaderLighting_::g_VS),
			ShaderLighting_::g_PS,
			sizeof(ShaderLighting_::g_PS),
			"ModelRendererLighting",
			decl,
			ARRAYSIZE(decl));

		Shader* shader_texture = Shader::Create(
			renderer,
			ShaderTexture_::g_VS,
			sizeof(ShaderTexture_::g_VS),
			ShaderTexture_::g_PS,
			sizeof(ShaderTexture_::g_PS),
			"ModelRendererTexture",
			decl,
			ARRAYSIZE(decl));

		Shader* shader = Shader::Create(
			renderer,
			Shader_::g_VS,
			sizeof(Shader_::g_VS),
			Shader_::g_PS,
			sizeof(Shader_::g_PS),
			"ModelRenderer",
			decl,
			ARRAYSIZE(decl));

		Shader* shader_distortion_texture = Shader::Create(
			renderer,
			ShaderDistortionTexture_::g_VS,
			sizeof(ShaderDistortionTexture_::g_VS),
			ShaderDistortionTexture_::g_PS,
			sizeof(ShaderDistortionTexture_::g_PS),
			"ModelRendererDistortionTexture",
			decl,
			ARRAYSIZE(decl));

		Shader* shader_distortion = Shader::Create(
			renderer,
			ShaderDistortion_::g_VS,
			sizeof(ShaderDistortion_::g_VS),
			ShaderDistortion_::g_PS,
			sizeof(ShaderDistortion_::g_PS),
			"ModelRendererDistortion",
			decl,
			ARRAYSIZE(decl));

		if (shader_lighting_texture_normal == nullptr ||
			shader_lighting_normal == nullptr ||
			shader_lighting_texture == nullptr ||
			shader_lighting == nullptr ||
			shader_texture == nullptr ||
			shader == nullptr ||
			shader_distortion_texture == nullptr ||
			shader_distortion == nullptr)
		{
			ES_SAFE_DELETE(shader_lighting_texture_normal);
			ES_SAFE_DELETE(shader_lighting_normal);
			ES_SAFE_DELETE(shader_lighting_texture);
			ES_SAFE_DELETE(shader_lighting);
			ES_SAFE_DELETE(shader_texture);
			ES_SAFE_DELETE(shader);
			ES_SAFE_DELETE(shader_distortion_texture);
			ES_SAFE_DELETE(shader_distortion);
		}

		return new ModelRenderer(renderer,
			shader_lighting_texture_normal,
			shader_lighting_normal,
			shader_lighting_texture,
			shader_lighting,
			shader_texture,
			shader,
			shader_distortion_texture,
			shader_distortion);
	}

	void ModelRenderer::BeginRendering(const efkModelNodeParam& parameter, int32_t count, void* userData)
	{
		// シェーダのリセット
		Shader* shaders[6];
		shaders[0] = m_shader_lighting_texture_normal;
		shaders[1] = m_shader_lighting_normal;
		shaders[2] = m_shader_lighting_texture;
		shaders[3] = m_shader_lighting;
		shaders[4] = m_shader_texture;
		shaders[5] = m_shader;
		for (int i = 0; i < 6; ++i)
		{
			shaders[i]->ResetConstantBuffer();
		}
		
		BeginRendering_(m_renderer, parameter, count, userData);
	}

	void ModelRenderer::EndRendering(const efkModelNodeParam& parameter, void* userData)
	{
		EndRendering_<
			RendererImplemented,
			Shader,
			Model,
			true,
			40>(
				m_renderer,
				m_shader_lighting_texture_normal,
				m_shader_lighting_normal,
				m_shader_lighting_texture,
				m_shader_lighting,
				m_shader_texture,
				m_shader,
				m_shader_distortion_texture,
				m_shader_distortion,
				parameter
				);
	}
}