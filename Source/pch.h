#ifndef PCH_H
#define PCH_H

#include "framework.h"

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "dxguid" )

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <d3d11_3.h>
#include <dxgi.h>

#undef min
#undef max

#include "IJed.h"

#include <algorithm>
#include <cmath>
#include <math.h>
#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

static constexpr float SRGB_GAMMA = 1.0f / 2.2f;
static constexpr float SRGB_INVERSE_GAMMA = 2.2f;
static constexpr float SRGB_ALPHA = 0.055f;

// Converts a single linear channel to srgb
inline float ToSRGB(float channel)
{
	if (channel <= 0.0031308)
		return 12.92f * channel;
	else
		return (1.0f + SRGB_ALPHA) * powf(channel, 1.0f / 2.4f) - SRGB_ALPHA;
}

// Converts a single srgb channel to rgb
inline float ToLinear(float channel)
{
	if (channel <= 0.04045f)
		return channel / 12.92f;
	else
		return powf((channel + SRGB_ALPHA) / (1.0f + SRGB_ALPHA), 2.4f);
}

#include "float3.h"
#include "float4.h"

#endif //PCH_H
