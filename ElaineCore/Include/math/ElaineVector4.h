#pragma once
#include "ElaineMath.h"
#include "ElaineVector3.h"

namespace Elaine
{
	class ElaineCoreExport Vector4
	{
	public:
		Vector4() = default;
		Vector4(float x_, float y_, float z_, float w_) : x{ x_ }, y{ y_ }, z{ z_ }, w{ w_ } {}
		Vector4(const Vector3& v3, float w_) : x{ v3.x }, y{ v3.y }, z{ v3.z }, w{ w_ } {}

		explicit Vector4(float coords[4]) : x{ coords[0] }, y{ coords[1] }, z{ coords[2] }, w{ coords[3] } {}

		float			operator[](size_t i) const
		{
			assert(i < 4);
			return *(&x + i);
		}

		float&			operator[](size_t i)
		{
			assert(i < 4);
			return *(&x + i);
		}

		float*			ptr() { return &x; }

		const float*	ptr() const { return &x; }

		Vector4&		operator=(float scalar)
		{
			x = scalar;
			y = scalar;
			z = scalar;
			w = scalar;
			return *this;
		}

		bool			operator==(const Vector4& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w); }

		bool			operator!=(const Vector4& rhs) const { return !(rhs == *this); }

		Vector4			operator+(const Vector4& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
		Vector4			operator-(const Vector4& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
		Vector4			operator*(float scalar) const { return Vector4(x * scalar, y * scalar, z * scalar, w * scalar); }
		Vector4			operator*(const Vector4& rhs) const { return Vector4(rhs.x * x, rhs.y * y, rhs.z * z, rhs.w * w); }
		Vector4			operator/(float scalar) const
		{
			assert(scalar != 0.0);
			return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
		}
		Vector4			operator/(const Vector4& rhs) const
		{
			assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
			return Vector4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
		}

		const Vector4&	operator+() const { return *this; }

		Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }

		friend Vector4	operator*(float scalar, const Vector4& rhs)
		{
			return Vector4(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z, scalar * rhs.w);
		}

		friend Vector4	operator/(float scalar, const Vector4& rhs)
		{
			assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
			return Vector4(scalar / rhs.x, scalar / rhs.y, scalar / rhs.z, scalar / rhs.w);
		}

		friend Vector4	operator+(const Vector4& lhs, float rhs)
		{
			return Vector4(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs);
		}

		friend Vector4	operator+(float lhs, const Vector4& rhs)
		{
			return Vector4(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w);
		}

		friend Vector4	operator-(const Vector4& lhs, float rhs)
		{
			return Vector4(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs);
		}

		friend Vector4	operator-(float lhs, const Vector4& rhs)
		{
			return Vector4(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w);
		}

		Vector4&		operator+=(const Vector4& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}

		Vector4&		operator-=(const Vector4& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}

		Vector4&		operator*=(float scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}

		Vector4&		operator+=(float scalar)
		{
			x += scalar;
			y += scalar;
			z += scalar;
			w += scalar;
			return *this;
		}

		Vector4&		operator-=(float scalar)
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			w -= scalar;
			return *this;
		}

		Vector4&		operator*=(const Vector4& rhs)
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			w *= rhs.w;
			return *this;
		}

		Vector4&		operator/=(float scalar)
		{
			assert(scalar != 0.0);

			x /= scalar;
			y /= scalar;
			z /= scalar;
			w /= scalar;
			return *this;
		}

		Vector4&		operator/=(const Vector4& rhs)
		{
			assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			w /= rhs.w;
			return *this;
		}

		float			dotProduct(const Vector4& vec) const { return x * vec.x + y * vec.y + z * vec.z + w * vec.w; }

		bool isNaN() const { return Math::isNan(x) || Math::isNan(y) || Math::isNan(z) || Math::isNan(w); }

		static const Vector4 ZERO;
		static const Vector4 UNIT_SCALE;
	public:
			 float x {0.f}, y{ 0.f }, z{ 0.f }, w{ 0.f };
	};
}