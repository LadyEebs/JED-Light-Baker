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

uint GetDescriptorBits(SDescriptor desc)
{
	return ExtractBits(desc.format.x, 2, 6);
}

uint2 GetTextureCoordinate(SDescriptor desc, float2 uv)
{
	const uint2 resolution = max(desc.resolution, uint2(1, 1));
	const float2 wrappedUV = frac(uv);
	uint2 coord = (uint2)floor(wrappedUV * (float2)resolution + 0.5f);
	return coord % resolution;
}

uint4 SampleTextureRGBA(SDescriptor desc, float2 uv, bool bIsAlbedo)
{
	uint2 coord = GetTextureCoordinate(desc, uv);
	
	SColorFormat format = UnpackColorFormat(desc.format);
	
	// check the color mode and load either paletted or rgba
	uint colorMode = format.colorMode;
	if (colorMode == 0) // paletted
	{
		uint pixel = Load8(coord.xy, int2(desc.offset, desc.rowStride));
		// todo: read colormap
		return uint4(pixel, pixel, pixel, pixel);
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
	
	return result;
}
