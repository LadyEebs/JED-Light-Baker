#ifndef VRAM_H
#define VRAM_H

static const uint kVRAMSize = 128 * 1024 * 1024; // needs to match VRAM_SIZE in VRAM.h
static const uint kVRAMMask = (kVRAMSize - 1);

Buffer<uint> VRAM : register(VRAM_REGISTER);

uint CalculateAddress(int x, int y, int offset, int stride)
{
	return (x + y * stride + offset) & kVRAMMask;
}

uint Load8(int2 coord, int2 offset)
{
	return VRAM[CalculateAddress(coord.x, coord.y, offset.x, offset.y)] & 0xFF;
}

uint Load16(int2 coord, int2 offset)
{
	coord.x <<= 1;
	
	uint texel;
	texel = (VRAM[CalculateAddress(coord.x + 0, coord.y, offset.x, offset.y)] & 0xFF) << 0;
	texel |= (VRAM[CalculateAddress(coord.x + 1, coord.y, offset.x, offset.y)] & 0xFF) << 8;
	return texel;
}

uint Load32(int2 coord, int2 offset)
{
	coord.x <<= 2;
	
	uint texel;
	texel = (VRAM[CalculateAddress(coord.x + 0, coord.y, offset.x, offset.y)] & 0xFF) << 0;
	texel |= (VRAM[CalculateAddress(coord.x + 1, coord.y, offset.x, offset.y)] & 0xFF) << 8;
	texel |= (VRAM[CalculateAddress(coord.x + 2, coord.y, offset.x, offset.y)] & 0xFF) << 16;
	texel |= (VRAM[CalculateAddress(coord.x + 3, coord.y, offset.x, offset.y)] & 0xFF) << 24;
	return texel;
}

#endif
