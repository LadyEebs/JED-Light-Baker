#include "VRAM.hlsli"
#include "Color.hlsli"

struct SDescriptor
{
	uint2 resolution;
	uint padding;
	uint totalSize;
	uint offset; // offset into vram
	uint rowStride;
	uint2 format;
};

StructuredBuffer<SDescriptor> aDescriptors : register(DESCRIPTOR_REGISTER);

Buffer<uint> aColormap : register(COLORMAP_REGISTER);
Buffer<uint> aLightLevels : register(LIGHTLEVELS_REGISTER);

uint4 DecodeColormapPixel(uint pixel)
{
	uint4 result = uint4(0, 0, 0, 0);
	result.r = ExtractBits(pixel, 0, 8);
	result.g = ExtractBits(pixel, 8, 8);
	result.b = ExtractBits(pixel, 16, 8);
	result.a = 255;
	return result;
}

uint GetDescriptorBits(SDescriptor desc)
{
	return ExtractBits(desc.format.x, 2, 6);
}

uint GetDescriptorColorMode(SDescriptor desc)
{
	return ExtractBits(desc.format.x, 0, 2);
}

uint2 GetTextureCoordinate(SDescriptor desc, float2 uv)
{
	const int2 resolution = max((int2) desc.resolution, int2(1, 1));
	int2 coord = (int2)floor(uv * (float2)resolution + 0.5f);
	coord = ((coord % resolution) + resolution) % resolution;
	return (uint2) coord;
}

uint4 SampleTextureRGBA(int hDescriptorHandle, float2 uv, bool bIsAlbedo, float emissiveLightLevel)
{
	// if the handle is 0 it's invalid, return black
	if (hDescriptorHandle == 0)
	{
		return uint4(0, 0, 0, 0);
	}
	
	// if the handle is negative it's a solid fill color
	if (hDescriptorHandle < 0)
	{
		uint pixel = abs(hDescriptorHandle);
		if (!bIsAlbedo)
		{
			uint lightLevel = saturate(emissiveLightLevel) * 63.0;
			pixel = aLightLevels[lightLevel * 256 + pixel];
		}
		return DecodeColormapPixel(aColormap[pixel]);
	}
	
	// otherwise it's a texture
	SDescriptor desc = aDescriptors[hDescriptorHandle];	
	uint2 coord = GetTextureCoordinate(desc, uv);	
	SColorFormat format = UnpackColorFormat(desc.format);
	
	// check the color mode and load either paletted or rgba
	uint colorMode = format.colorMode;
	if (colorMode == 0) // paletted
	{
		uint pixel = Load8(coord.xy, int2(desc.offset, desc.rowStride));
		if (!bIsAlbedo)
		{
			uint lightLevel = saturate(emissiveLightLevel) * 63.0;
			pixel = aLightLevels[lightLevel * 256 + pixel];
		}
		
		uint4 result = DecodeColormapPixel(aColormap[pixel]);
		return result;
	}

	// rgba mode
	uint pixel = 0;
	if (format.bpp == 16)
		pixel = Load16(coord.xy, int2(desc.offset, desc.rowStride));
	else
		pixel = Load32(coord.xy, int2(desc.offset, desc.rowStride));
		
	// decode the pixel into rgba
	uint4 result = uint4(0, 0, 0, 0);
	DecodeRGBA(pixel, format, result.r, result.g, result.b, result.a);
	
	if (!bIsAlbedo)
		result *= emissiveLightLevel;
	
	return result;
}
