#pragma once

// 4 component vector
struct float4
{
	constexpr float4() {}
	constexpr float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	float x;
	float y;
	float z;
	float w;

	float4& operator+=(float other)
	{
		x += other;
		y += other;
		z += other;
		w += other;
		return *this;
	}

	float4& operator+=(const float4& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	float4& operator-=(const float4& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		z -= other.w;
		return *this;
	}

	float4& operator*=(const float4& other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		w *= other.w;
		return *this;
	}


	float4& operator*=(float scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;
		w *= scale;
		return *this;
	}
};

inline float4 operator+(const float4& a, const float4& b)
{
	float4 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = a.w + b.w;
	return result;
}

inline float4 operator+(const float4& v, float scale)
{
	float4 result;
	result.x = v.x + scale;
	result.y = v.y + scale;
	result.z = v.z + scale;
	result.w = v.w + scale;
	return result;
}

inline float4 operator-(const float4& a, const float4& b)
{
	float4 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.w = a.w - b.w;
	return result;
}

inline float4 operator-(const float4& v, float scale)
{
	float4 result;
	result.x = v.x - scale;
	result.y = v.y - scale;
	result.z = v.z - scale;
	result.w = v.w - scale;
	return result;
}

inline float4 operator*(const float4& a, const float4& b)
{
	float4 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	result.w = a.w * b.w;
	return result;
}

inline float4 operator*(const float4& v, float scale)
{
	float4 result;
	result.x = v.x * scale;
	result.y = v.y * scale;
	result.z = v.z * scale;
	result.w = v.w * scale;
	return result;
}

inline float4 operator*(float scale, const float4& v)
{
	float4 result;
	result.x = v.x * scale;
	result.y = v.y * scale;
	result.z = v.z * scale;
	result.w = v.w * scale;
	return result;
}

inline float4 operator/(const float4& a, const float4& b)
{
	float4 result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	result.w = a.w / b.w;
	return result;
}

inline float4 operator/(const float4& v, float scale)
{
	float invScale = 1.0f / scale;
	float4 result;
	result.x = v.x * invScale;
	result.y = v.y * invScale;
	result.z = v.z * invScale;
	result.w = v.w * invScale;
	return result;
}

inline float dot(const float4& v1, const float4& v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z) + (v1.w * v2.w);
}

// Converts a linear rgb color to a srgb color
// Typically you'd leave alpha alone, but since we're using it for "intensity"
// we want to include it in the conversion
inline float4 ToSRGB(const float4& rgb)
{
	return float4(
		ToSRGB(rgb.x),
		ToSRGB(rgb.y),
		ToSRGB(rgb.z),
		ToSRGB(rgb.w)
	);
}

// Converts a srgb color to a linear rgb color
inline float4 ToLinear(const float4& srgb)
{
	return float4(
		ToLinear(srgb.x),
		ToLinear(srgb.y),
		ToLinear(srgb.z),
		ToLinear(srgb.w)
	);
}
