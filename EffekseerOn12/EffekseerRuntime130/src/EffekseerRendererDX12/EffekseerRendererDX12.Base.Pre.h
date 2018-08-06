
#ifndef	__EFFEKSEERRENDERER_DX11_BASE_PRE_H__
#define	__EFFEKSEERRENDERER_DX11_BASE_PRE_H__

#include <Effekseer.h>

#include <windows.h>
#include <d3d12.h>
#include <wrl.h>


#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib,"d3d12.lib")

using Microsoft::WRL::ComPtr;

namespace EffekseerRendererDX12
{

	class Renderer;

}

#endif