
#ifndef	__EFFEKSEERRENDERER_DX12_BASE_PRE_H__
#define	__EFFEKSEERRENDERER_DX12_BASE_PRE_H__

#include <Effekseer.h>

#include <windows.h>
#include <d3d12.h>

#if _WIN32
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "d3d12.lib")
#endif

namespace EffekseerRendererDX12
{
	class Renderer;
}

#endif



#ifndef __EFFEKSEERRENDERER_RENDERER_H__
#define __EFFEKSEERRENDERER_RENDERER_H__

#include <Effekseer.h>

namespace EffekseerRenderer
{
	class DistortingCallback
	{
	public:
		DistortingCallback() {}
		virtual ~DistortingCallback() {}
		virtual bool OnDistorting() { return false; }
	};

	class Renderer
		: public ::Effekseer::IReference
	{
	protected:
		Renderer() {}
		virtual ~Renderer() {}

	public:

		virtual void OnLostDevice() = 0;
		virtual void OnResetDevice() = 0;
		virtual void Destroy() = 0;
		virtual void SetRestorationOfStatesFlag(bool flag) = 0;
		virtual bool BeginRendering() = 0;
		virtual bool EndRendering() = 0;

		virtual const ::Effekseer::Vector3D& GetLightDirection() const = 0;
		virtual void SetLightDirection(::Effekseer::Vector3D& direction) = 0;

		virtual const ::Effekseer::Color& GetLigthColor() const = 0;
		virtual void SetLightColor(::Effekseer::Color& color) = 0;

		virtual const ::Effekseer::Color& GetLightAmbientColor() const = 0;
		virtual void SetLightAmbientColor(::Effekseer::Color& color) = 0;

		virtual int32_t GetSquareMaxCount() const = 0;

		virtual const ::Effekseer::Matrix44& GetProjectionMatrix() const = 0;
		virtual void SetProjectionMatrix(const ::Effekseer::Matrix44& mat) = 0;

		virtual const ::Effekseer::Matrix44& GetCameraMatrix() const = 0;
		virtual void SetCameraMatrix(const ::Effekseer::Matrix44& mat) = 0;

		virtual ::Effekseer::Matrix44& GetCameraProjectionMatrix() = 0;
		virtual ::Effekseer::SpriteRenderer* CreateSpriteRenderer() = 0;
		virtual ::Effekseer::RibbonRenderer* CreateRibbonRenderer() = 0;
		virtual ::Effekseer::RingRenderer* CreateRingRenderer() = 0;
		virtual ::Effekseer::ModelRenderer* CreateModelRenderer() = 0;
		virtual ::Effekseer::TrackRenderer* CreateTrackRenderer() = 0;

		virtual ::Effekseer::TextureLoader* CreateTextureLoader(::Effekseer::FileInterface* fileInterface = nullptr) = 0;
		virtual ::Effekseer::ModelLoader* CreateModelLoader(::Effekseer::FileInterface* fileInterface = nullptr) = 0;

		// レンダーステートを強制的にリセットする
		virtual void ResetRenderState() = 0;

		virtual DistortingCallback* GetDistortingCallback() = 0;

		virtual void SetDistortingCallback(DistortingCallback* callback) = 0;


	};
}

#endif

#ifndef __EFFEKSEERRENDERER_DX12_RENDERER_H__

#define __EFFEKSEERRENDERER_DX12_RENDERER_H__

namespace EffekseerRendererDX12
{
	::Effekseer::TextureLoader* CreateTextureLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface = nullptr);

	::Effekseer::ModelLoader* CreateModelLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface = nullptr);

	class Renderer
		: public ::EffekseerRenderer::Renderer
	{
	protected:
		Renderer() {}
		virtual ~Renderer() {}

	public:

		static Renderer* Create(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			int32_t squareMaxCount,
			D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS);

		virtual ID3D12Device* GetDevice() = 0;
		virtual Effekseer::TextureData* GetBackground() = 0;

		//virtual void SetBackground(ID3D11ShaderResourceView* background) = 0;
	};


	class Model : public Effekseer::Model
	{
	private:

	public:
		ID3D12Resource*			VertexBuffer;
		ID3D12Resource*			IndexBuffer;
		int32_t					VertexCount;
		int32_t					IndexCount;
		int32_t					FaceCount;
		int32_t					ModelCount;

		Model(uint8_t* data, int32_t size)
			: Effekseer::Model(data, size)
			, VertexBuffer(nullptr)
			, IndexBuffer(nullptr)
			, VertexCount(0)
			, IndexCount(0)
			, FaceCount(0)
			, ModelCount(0)
		{
		}

		virtual ~Model()
		{
		}
	};
}

#endif