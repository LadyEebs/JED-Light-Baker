#pragma once

struct float3
{
	constexpr float3() {}
	constexpr float3(float x, float y, float z) : x(x), y(y), z(z) {}

	float x;
	float y;
	float z;

	float3& operator+=(float other)
	{
		x += other;
		y += other;
		z += other;
		return *this;
	}

	float3& operator+=(const float3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	float3& operator-=(const float3& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	float3& operator*=(const float3& other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}


	float3& operator*=(float scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;
		return *this;
	}
};

inline float3 operator*(const float3& a, const float3& b)
{
	float3 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	return result;
}

inline float3 operator/(const float3& a, const float3& b)
{
	float3 result;
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	return result;
}

inline float3 operator+(const float3& a, const float3& b)
{
	float3 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

inline float3 operator+(const float3& v, float scale)
{
	float3 result;
	result.x = v.x + scale;
	result.y = v.y + scale;
	result.z = v.z + scale;
	return result;
}

inline float3 operator-(const float3& a, const float3& b)
{
	float3 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

inline float3 operator-(const float3& v, float scale)
{
	float3 result;
	result.x = v.x - scale;
	result.y = v.y - scale;
	result.z = v.z - scale;
	return result;
}

inline float3 operator*(const float3& v, float scale)
{
	float3 result;
	result.x = v.x * scale;
	result.y = v.y * scale;
	result.z = v.z * scale;
	return result;
}

inline float3 operator*(float scale, const float3& v)
{
	float3 result;
	result.x = v.x * scale;
	result.y = v.y * scale;
	result.z = v.z * scale;
	return result;
}

inline float3 operator/(const float3& v, float scale)
{
	float invScale = 1.0f / scale;
	float3 result;
	result.x = v.x * invScale;
	result.y = v.y * invScale;
	result.z = v.z * invScale;
	return result;
}

inline float3 cross(const float3& v2, const float3& v3)
{
	float3 result;
	result.x = (v3.z * v2.y) - (v2.z * v3.y);
	result.y = (v2.z * v3.x) - (v3.z * v2.x);
	result.z = (v3.y * v2.x) - (v2.y * v3.x);
	return result;
}

inline float dot(const float3& v1, const float3& v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

inline float length(const float3& v)
{
	return sqrtf(dot(v, v));
}

inline float3 normalize(const float3& v)
{
	float len = length(v);
	if (len < 1e-6)
		return v;
		
	return v / len;
}

inline float3 reflect(const float3& v, const float3& n)
{
	return v - n * 2.0f * dot(n, v);
}

// Converts a linear rgb color to a srgb color
inline float3 ToSRGB(const float3& rgb)
{
	return float3(
		ToSRGB(rgb.x),
		ToSRGB(rgb.y),
		ToSRGB(rgb.z)
	);
}

// Converts a srgb color to a linear rgb color
inline float3 ToLinear(const float3& srgb)
{
	return float3(
		ToLinear(srgb.x),
		ToLinear(srgb.y),
		ToLinear(srgb.z)
	);
}
