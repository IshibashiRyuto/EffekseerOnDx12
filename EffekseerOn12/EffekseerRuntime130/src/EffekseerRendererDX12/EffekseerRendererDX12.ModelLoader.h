#pragma once

#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererDX12.DeviceObject.h"

namespace EffekseerRendererDX12
{

	class ModelLoader
		: public ::Effekseer::ModelLoader
	{
	private:
		ID3D12Device*		device;
		::Effekseer::FileInterface* m_fileInterface;
		::Effekseer::DefaultFileInterface m_defaultFileInterface;

	public:
		ModelLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface);
		virtual ~ModelLoader();

		void* Load(const EFK_CHAR* path);
		void Unload(void* data);

	};

}