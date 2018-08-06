#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__

#pragma once

#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererDX12.DeviceObject.h"

namespace EffekseerRendererDX12
{
	/// テクスチャ読込クラス
	class TextureLoader
		: public ::Effekseer::TextureLoader {
	private:
		ID3D12Device*						device;
		::Effekseer::FileInterface*			m_fileInterface;
		::Effekseer::DefaultFileInterface	m_defaultFileInterface;

	public:
		TextureLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface = nullptr);
		virtual ~TextureLoader();

		Effekseer::TextureData* Load(const EFK_CHAR* path, ::Effekseer::TextureType textureType) override;

		void Unload(Effekseer::TextureData* data) override;
	};

}

#endif