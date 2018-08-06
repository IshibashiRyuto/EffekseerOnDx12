#pragma once

#include <Effekseer.h>
#include <assert.h>
#include <string.h>
#include <vector>

namespace EffekseerRenderer
{
	class DDSTextureLoader
	{
	private:
		static std::vector<uint8_t> textureData;
		static int32_t textureWidth;
		static int32_t textureHeight;
		static Effekseer::TextureFormatType textureFormatType;

	public :
		static bool Load(void* data, int32_t size);
		static void Unload();

		static std::vector<uint8_t>& GetData() { return textureData; }
		static int32_t GetWidth() { return textureWidth; }
		static int32_t GetHeight() { return textureHeight; }
		static Effekseer::TextureFormatType GetTextureFormat() { return textureFormatType; }
	};
}
