#pragma once

#include "common/common.hpp"
#include "dxgiformat.h"

namespace rn::rhi
{
	enum class DepthFormat : uint32_t;
	enum class RenderTargetFormat : uint32_t;
	enum class TextureFormat : uint32_t;

	DXGI_FORMAT ToDXGIFormat(DepthFormat format);
	DXGI_FORMAT ToDXGIFormat(RenderTargetFormat format);
	DXGI_FORMAT ToDXGIFormat(TextureFormat format);
	DXGI_FORMAT ToTypelessDXGIFormat(TextureFormat format);

	TextureFormat DXGIToTextureFormat(DXGI_FORMAT dxgiFormat);
}
