#pragma once
#include "math/ElaineMath.h"

namespace Elaine
{
	class Vector2
	{
	public:
		Vector2() = default;
		Vector2(float _x, float _y) :x(_x), y(_y) {}
		explicit Vector2(float val) :x(val), y(val) {}
		explicit Vector2(const float v[2]) :x(v[0]), y(v[1]) {}
		explicit Vector2(float* const r) :x(r[0]), y(r[1]) {}

		float*				ptr() { return &x; }

		const float*		ptr()const { return &x; }

		float				operator[](size_t i) const
		{
			assert(i < 2);
			return (i == 0 ? x : y);
		}

		float&				operator[](size_t i)
		{
			assert(i < 2);
			return (i == 0 ? x : y);
		}

        bool                operator==(const Vector2& rhs) const { return (x == rhs.x && y == rhs.y); }

        bool                operator!=(const Vector2& rhs) const { return (x != rhs.x || y != rhs.y); }

        Vector2             operator+(const Vector2& rhs) const { return Vector2(x + rhs.x, y + rhs.y); }

        Vector2             operator-(const Vector2& rhs) const { return Vector2(x - rhs.x, y - rhs.y); }

        Vector2             operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }

        Vector2             operator*(const Vector2& rhs) const { return Vector2(x * rhs.x, y * rhs.y); }

        Vector2             operator/(float scale) const
        {
            assert(scale != 0.0);

            float inv = 1.0f / scale;
            return Vector2(x * inv, y * inv);
        }

        Vector2             operator/(const Vector2& rhs) const { return Vector2(x / rhs.x, y / rhs.y); }

        const Vector2&      operator+() const { return *this; }

        Vector2             operator-() const { return Vector2(-x, -y); }

        // overloaded operators to help Vector2
        friend Vector2      operator*(float scalar, const Vector2& rhs) { return Vector2(scalar * rhs.x, scalar * rhs.y); }

        friend Vector2      operator/(float fScalar, const Vector2& rhs)
        {
            return Vector2(fScalar / rhs.x, fScalar / rhs.y);
        }

        friend Vector2      operator+(const Vector2& lhs, float rhs) { return Vector2(lhs.x + rhs, lhs.y + rhs); }

        friend Vector2      operator+(float lhs, const Vector2& rhs) { return Vector2(lhs + rhs.x, lhs + rhs.y); }

        friend Vector2      operator-(const Vector2& lhs, float rhs) { return Vector2(lhs.x - rhs, lhs.y - rhs); }

        friend Vector2      operator-(float lhs, const Vector2& rhs) { return Vector2(lhs - rhs.x, lhs - rhs.y); }
        bool                operator<(const Vector2& rhs) const { return x < rhs.x && y < rhs.y; }
        bool                operator>(const Vector2& rhs) const { return x > rhs.x && y > rhs.y; }
        Vector2&            operator+=(const Vector2& rhs)
        {
            x += rhs.x;
            y += rhs.y;

            return *this;
        }

        Vector2&            operator+=(float scalar)
        {
            x += scalar;
            y += scalar;

            return *this;
        }

        Vector2&            operator-=(const Vector2& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;

            return *this;
        }

        Vector2&            operator-=(float scalar)
        {
            x -= scalar;
            y -= scalar;

            return *this;
        }

        Vector2&            operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;

            return *this;
        }

        Vector2&            operator*=(const Vector2& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;

            return *this;
        }

        Vector2&            operator/=(float scalar)
        {
            assert(scalar != 0.0);

            float inv = 1.0f / scalar;

            x *= inv;
            y *= inv;

            return *this;
        }

        Vector2&            operator/=(const Vector2& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;

            return *this;
        }

        float               length() const { return std::hypot(x, y); }
        float               squaredLength() const { return x * x + y * y; }
        float               distance(const Vector2& rhs) const { return (*this - rhs).length(); }
        float               squaredDistance(const Vector2& rhs) const { return (*this - rhs).squaredLength(); }
        float               dotProduct(const Vector2& vec) const { return x * vec.x + y * vec.y; }
        float               normalise()
        {
            float lengh = std::hypot(x, y);

            if (lengh > 0.0f)
            {
                float inv_length = 1.0f / lengh;
                x *= inv_length;
                y *= inv_length;
            }

            return lengh;
        }

        float               getX() const { return x; }
        float               getY() const { return y; }

        void                setX(float value) { x = value; }
        void                setY(float value) { y = value; }
        Vector2             midPoint(const Vector2& vec) const { return Vector2((x + vec.x) * 0.5f, (y + vec.y) * 0.5f); }
        void                makeFloor(const Vector2& cmp)
        {
            if (cmp.x < x)
                x = cmp.x;
            if (cmp.y < y)
                y = cmp.y;
        }
        void                makeCeil(const Vector2& cmp)
        {
            if (cmp.x > x)
                x = cmp.x;
            if (cmp.y > y)
                y = cmp.y;
        }
        Vector2             perpendicular(void) const { return Vector2(-y, x); }
        float               crossProduct(const Vector2& rhs) const { return x * rhs.y - y * rhs.x; }
        bool                isZeroLength(void) const
        {
            float sqlen = (x * x) + (y * y);
            return (sqlen < (FLT_EPSILON * FLT_EPSILON));
        }
        Vector2             normalisedCopy(void) const
        {
            Vector2 ret = *this;
            ret.normalise();
            return ret;
        }
        Vector2             reflect(const Vector2& normal) const
        {
            return Vector2(*this - (2 * this->dotProduct(normal) * normal));
        }
        bool                isNaN() const { return Math::isNan(x) || Math::isNan(y); }
        static Vector2      lerp(const Vector2& lhs, const Vector2& rhs, float alpha) { return lhs + alpha * (rhs - lhs); }
	public:
		float				        x = .0f;
		float				        y = .0f;
        static const Vector2        ZERO;
        static const Vector2        UNIT_X;
        static const Vector2        UNIT_Y;
        static const Vector2        NEGATIVE_UNIT_X;
        static const Vector2        NEGATIVE_UNIT_Y;
        static const Vector2        UNIT_SCALE;
	};
}