
#include "EffekseerRendererDX12.TextureData.h"

namespace EffekseerRendererDX12
{
	TextureData::TextureData(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
		: m_resource(resource)
		, m_srvDesc(srvDesc)
	{

	}
	TextureData * TextureData::Create(ID3D12Resource * resource)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		return new TextureData(resource, srvDesc);
	}
}