
#ifndef	__EFFEKSEERRENDERER_DX11_DEVICEOBJECT_H__
#define	__EFFEKSEERRENDERER_DX11_DEVICEOBJECT_H__

#include "EffekseerRendererDX12.RendererImplemented.h"

namespace EffekseerRendererDX12
{
	/*	@class DeviceObject
		デバイスによって生成されるオブジェクト
	*/
	class DeviceObject
	{
	private:
		RendererImplemented*	m_renderer;
	public:
		DeviceObject( RendererImplemented* renderer );
		virtual ~DeviceObject();

		virtual void OnLostDevice() = 0;
		virtual void OnResetDevice() = 0;
		RendererImplemented* GetRenderer() const;
	};

}

#endif