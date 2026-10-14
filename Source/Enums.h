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

	ELightBake_Direct = ELightBake_Lights | ELightBake_Sun | ELightBake_Sky | ELightBake_Emissive
};

// Gpu surface flags
enum ESurfaceFlags
{
	ESurface_IsHorizonSky = 0x1,
	ESurface_IsCeilingSky = 0x2,
	ESurface_IsVisible = 0x4,
	ESurface_IsTranslucent = 0x8,

	ESurface_IsSky = ESurface_IsHorizonSky | ESurface_IsCeilingSky
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

// Engine/editor sky surface flags
enum ESkyFlags
{
	ESky_Horizon = 0x200,
	ESky_Ceiling = 0x400,
};

// Baker specific light flags
enum ELightFlags
{
	ELight_Sun = 0x2,
	ELight_Sky = 0x4,
	ELight_Anchor = 0x8,
	ELight_Ambient = 0x10,
};
