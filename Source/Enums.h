#pragma once

// Light bake configuration
enum ELightBakeFlags
{
	ELightBake_Lights = 0x1,
	ELightBake_Sun = 0x2,
	ELightBake_Sky = 0x4,
	ELightBake_Emissive = 0x8,
	ELightBake_Indirect = 0x10,
	ELightBake_Selected = 0x20,
	ELightBake_VisibleLayers = 0x40,
	ELightBake_GammaCorrect = 0x80,
	ELightBake_ExtraLightEmissive = 0x100,
	ELightBake_ToneMap = 0x200,
	ELightBake_PhysicalFalloff = 0x400,
	ELightBake_SkyColorFromMaterial = 0x800,
	ELightBake_AllowLightCleave = 0x1000,
	ELightBake_AllowSunCleave = 0x2000,

	ELightBake_Direct = ELightBake_Lights | ELightBake_Sun | ELightBake_Sky | ELightBake_Emissive
};

// Gpu surface flags
enum EGpuSurfaceFlags
{
	EGpuSurface_IsHorizonSky = 0x1,
	EGpuSurface_IsCeilingSky = 0x2,
	EGpuSurface_IsVisible = 0x4,
	EGpuSurface_IsTranslucent = 0x8,

	EGpuSurface_IsSky = EGpuSurface_IsHorizonSky | EGpuSurface_IsCeilingSky
};

// Engine/editor sector flags
enum ESectorFlags
{
	// SED specific
	ESector_NoAmbientLightRGB = 0x20000000,
	ESector_NoAmbientLight = 0x40000000
};

// Engine/editor adjoin flags
enum EAdjoinFlags
{
	EAdjoin_Visible = 0x1,
	EAdjoin_BlocksLight = 0x80000000,
};

// Engine/editor face flags
enum EFaceFlags
{
	EFace_Translucent = 0x2,
};

// Engine/editor surface flags
enum ESurfaceFlags
{
	ESurface_CogLinked = 0x2,

	ESurface_Horizon = 0x200,
	ESurface_Ceiling = 0x400,

	ESurface_IsSky = ESurface_Horizon | ESurface_Ceiling
};

// Engine/editor geometry modes
enum EGeoMode
{
	EGeoMode_NotRendered = 0,
	EGeoMode_Points = 1,
	EGeoMode_Lines = 2,
	EGeoMode_Solid = 3,
	EGeoMode_Textured = 4
};

// Engine/editor light modes
enum ELightMode
{
	ELightMode_FullyLit = 0,
	ELightMode_Unlit = 1,
	ELightMode_Diffuse = 2,
	ELightMode_Gouraud = 3,
};

// Baker specific light flags
enum ELightFlags
{
	ELight_Sun = 0x2,
	ELight_Sky = 0x4,
	ELight_Anchor = 0x8,
	ELight_Spotlight = 0x10, // TODO
	ELight_Ambient = 0x20,
	ELight_Cleave = 0x40,
};

// Material record types (for .mat reading)
enum EMaterialRecordType
{
	EMaterialRecordType_Color = 0,
	EMaterialRecordType_Texture = 0x8,
};

// Must match order of variables in defaultCogContent
enum ESettings
{
	ESetting_BakePointLights,
	ESetting_AllowSunLightCleave,
	ESetting_AllowLightCleave,
	ESetting_BakeSunLight,
	ESetting_BakeSkyLight,
	ESetting_BakeEmissiveLight,
	ESetting_BakeIndirectLight,
	ESetting_GammaCorrect,
	ESetting_ExtraLightEmissive,
	ESetting_PhysicalFalloff,
	ESetting_ToneMap,
	ESetting_SkyColorFromMaterials,
	ESetting_SkyEmissiveRayCount,
	ESetting_IndirectRayCount,
	ESetting_IndirectBounces,
	ESetting_NormalSmoothingAngle,

	ESettings_Count
};