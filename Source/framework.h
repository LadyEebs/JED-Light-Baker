#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxdialogex.h>

#include <chrono>
#include <unordered_map>
#include <vector>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "dxguid" )

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <d3d11_3.h>
#include <dxgi.h>

#include "IJed.h"

#include <cmath>
#include <math.h>

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

#undef min
#undef max