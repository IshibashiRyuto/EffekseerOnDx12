
#include "EffekseerRendererDX12.RendererImplemented.h"
#include "EffekseerRendererDX12.TextureLoader.h"
#include "EffekseerRendererDX12.TextureData.h"
#include "../EffekseerRendererCommon/EffekseerRenderer.DXTK.DDSTextureLoader.h"
#include "../EffekseerRendererCommon/EffekseerRenderer.PngTextureLoader.h"
#include "../EffekseerRendererCommon/EffekseerRenderer.DDSTextureLoader.h"

namespace EffekseerRendererDX12
{
	TextureLoader::TextureLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface)
		: m_fileInterface(fileInterface)
		, device(device)
	{
		ES_SAFE_ADDREF(device);
		if (fileInterface == nullptr)
		{
			m_fileInterface = &m_defaultFileInterface;
		}
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
		EffekseerRenderer::PngTextureLoader::Initialize();
#endif
	}

	TextureLoader::~TextureLoader()
	{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
		EffekseerRenderer::PngTextureLoader::Finalize();
#endif

		ES_SAFE_RELEASE(device);
	}

	Effekseer::TextureData* TextureLoader::Load(const EFK_CHAR* path, ::Effekseer::TextureType textureType)
	{
		std::shared_ptr<::Effekseer::FileReader>
			reader(m_fileInterface->OpenRead(path));
		if (reader)
		{
			// テクスチャ読み込み処理
			ComPtr<ID3D12Resource> texture{ nullptr };
			Effekseer::TextureData* textureData{ nullptr };

			D3D12_HEAP_PROPERTIES heapProp;
			heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
			heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
			heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
			heapProp.CreationNodeMask = 1;
			heapProp.VisibleNodeMask = 1;


			size_t size_texture = reader->GetLength();
			char* data_texture = new char[size_texture];
			reader->Read(data_texture, size_texture);

			if (size_texture < 4)
			{

			}
			else if (data_texture[1] == 'P' &&
				data_texture[2] == 'N' &&
				data_texture[3] == 'G')
			{
				if (::EffekseerRenderer::PngTextureLoader::Load(data_texture, size_texture, false))
				{
					D3D12_RESOURCE_DESC resourceDesc{};
					resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					resourceDesc.Width = ::EffekseerRenderer::PngTextureLoader::GetWidth();
					resourceDesc.Height = ::EffekseerRenderer::PngTextureLoader::GetHeight();
					resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					resourceDesc.DepthOrArraySize = 1;
					resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
					resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
					resourceDesc.SampleDesc.Count = 1;
					resourceDesc.SampleDesc.Quality = 0;

					
					auto hr = device->CreateCommittedResource(
						&heapProp,
						D3D12_HEAP_FLAG_NONE,
						&resourceDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&texture));
					if (FAILED(hr))
					{
						goto Exit;
					}

					D3D12_BOX textureBox = { 0,0,0,resourceDesc.Width, resourceDesc.Height, 1 };
					texture->WriteToSubresource(0, &textureBox, ::EffekseerRenderer::PngTextureLoader::GetData().data(),
						resourceDesc.Width * 4, resourceDesc.Width * resourceDesc.Height * 4);

					auto tex = TextureData::Create(texture.Get());
					textureData = new Effekseer::TextureData();
					textureData->UserPtr = tex;
					textureData->UserID = 0;
					textureData->TextureFormat = Effekseer::TextureFormatType::ABGR8;
					textureData->Width = resourceDesc.Width;
					textureData->Height = resourceDesc.Height;
				}
			}
		Exit:;
			delete[] data_texture;
			return textureData;

		}

		return nullptr;
	}

	void TextureLoader::Unload(Effekseer::TextureData* data)
	{
		if (data != nullptr && data->UserPtr != nullptr)
		{
			//テクスチャデータの削除処理
			auto texture = (TextureData*)data->UserPtr;
			ES_SAFE_DELETE(texture);
		}

		if (data != nullptr)
		{
			delete data;
		}
	}
}