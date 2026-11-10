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

uint4 SampleTextureRGBA(SDescriptor desc, int2 coord, bool bIsAlbedo)
{
	// check the color mode and load either paletted or rgba
	uint colorMode = ExtractBits(desc.format.x, 0, 2);
	if (colorMode == 0) // paletted
	{
		uint pixel = Load8(coord.xy, int2(desc.offset, desc.rowStride));
		// todo: read colormap
		return uint4(pixel, pixel, pixel, pixel);
	}

	// rgba mode
	uint bpp = ExtractBits(desc.format.x, 2, 6);
	
	uint pixel = 0;
	if (bpp == 16)
		pixel = Load16(coord.xy, int2(desc.offset, desc.rowStride));
	else
		pixel = Load32(coord.xy, int2(desc.offset, desc.rowStride));
		
	// decode the pixel into rgba
	SColorFormat format = UnpackColorFormat(desc.format);
	
	uint4 result = uint4(0, 0, 0, 0);
	DecodeRGBA(pixel, format, result.r, result.g, result.b, result.a);
	
	return result;
}
