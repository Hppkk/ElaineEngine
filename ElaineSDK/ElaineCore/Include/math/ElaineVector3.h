#pragma once
#include "math/ElaineMath.h"
#include "math/ElaineQuaternion.h"

namespace Elaine
{
	class ElaineCoreExport Vector3
	{
	public:
		Vector3() = default;
		Vector3(float a, float b, float c) :x{ a }, y{ b }, z{ c } {}

		explicit Vector3(const float coords[3]) : x{ coords[0] }, y{ coords[1] }, z{ coords[2] } {}

		float				operator[](size_t i) const
		{
			assert(i < 3);
			return *(&x + i);
		}

		float&				operator[](size_t i)
		{
			assert(i < 3);
			return *(&x + i);
		}

        float*              ptr() { return &x; }

        const float*        ptr() const { return &x; }

        bool                operator==(const Vector3& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z); }

        bool                operator!=(const Vector3& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }

        Vector3             operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }

        Vector3             operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }

        Vector3             operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }

        Vector3             operator*(const Vector3& rhs) const { return Vector3(x * rhs.x, y * rhs.y, z * rhs.z); }

        Vector3             operator/(float scalar) const
        {
            assert(scalar != 0.0);
            return Vector3(x / scalar, y / scalar, z / scalar);
        }

        Vector3             operator/(const Vector3& rhs) const
        {
            assert((rhs.x != 0 && rhs.y != 0 && rhs.z != 0));
            return Vector3(x / rhs.x, y / rhs.y, z / rhs.z);
        }

        const Vector3&      operator+() const { return *this; }

        Vector3             operator-() const { return Vector3(-x, -y, -z); }

        // overloaded operators to help Vector3
        friend Vector3      operator*(float scalar, const Vector3& rhs)
        {
            return Vector3(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z);
        }

        friend Vector3      operator/(float scalar, const Vector3& rhs)
        {
            assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
            return Vector3(scalar / rhs.x, scalar / rhs.y, scalar / rhs.z);
        }

        friend Vector3      operator+(const Vector3& lhs, float rhs)
        {
            return Vector3(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs);
        }

        friend Vector3      operator+(float lhs, const Vector3& rhs)
        {
            return Vector3(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z);
        }

        friend Vector3      operator-(const Vector3& lhs, float rhs)
        {
            return Vector3(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs);
        }

        friend Vector3      operator-(float lhs, const Vector3& rhs)
        {
            return Vector3(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z);
        }

        // arithmetic updates
        Vector3&            operator+=(const Vector3& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        Vector3&            operator+=(float scalar)
        {
            x += scalar;
            y += scalar;
            z += scalar;
            return *this;
        }

        Vector3&            operator-=(const Vector3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        Vector3&            operator-=(float scalar)
        {
            x -= scalar;
            y -= scalar;
            z -= scalar;
            return *this;
        }

        Vector3&            operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        Vector3&            operator*=(const Vector3& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }

        Vector3&            operator/=(float scalar)
        {
            assert(scalar != 0.0);
            x /= scalar;
            y /= scalar;
            z /= scalar;
            return *this;
        }

        Vector3&            operator/=(const Vector3& rhs)
        {
            assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            return *this;
        }

        float               length() const { return std::hypot(x, y, z); }

        float               squaredLength() const { return x * x + y * y + z * z; }

        float               distance(const Vector3& rhs) const { return (*this - rhs).length(); }


        float               squaredDistance(const Vector3& rhs) const { return (*this - rhs).squaredLength(); }

        float               dotProduct(const Vector3& vec) const { return x * vec.x + y * vec.y + z * vec.z; }

        void                normalise()
        {
            float length = std::hypot(x, y, z);
            if (length == 0.f)
                return;

            float inv_lengh = 1.0f / length;
            x *= inv_lengh;
            y *= inv_lengh;
            z *= inv_lengh;
        }

        Vector3             crossProduct(const Vector3& rhs) const
        {
            return Vector3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
        }

        void                makeFloor(const Vector3& cmp)
        {
            if (cmp.x < x)
                x = cmp.x;
            if (cmp.y < y)
                y = cmp.y;
            if (cmp.z < z)
                z = cmp.z;
        }

        void                makeCeil(const Vector3& cmp)
        {
            if (cmp.x > x)
                x = cmp.x;
            if (cmp.y > y)
                y = cmp.y;
            if (cmp.z > z)
                z = cmp.z;
        }

        Radian              angleBetween(const Vector3& dest) const
        {
            float len_product = length() * dest.length();

            if (len_product < 1e-6f)
                len_product = 1e-6f;

            float f = dotProduct(dest) / len_product;

            f = Math::clamp(f, (float)-1.0, (float)1.0);
            return Math::acos(f);
        }

        Quaternion          getRotationTo(const Vector3& dest, const Vector3& fallback_axis = Vector3::ZERO) const
        {
            Quaternion q;
            Vector3 v0 = *this;
            Vector3 v1 = dest;
            v0.normalise();
            v1.normalise();

            float d = v0.dotProduct(v1);

            if (d >= 1.0f)
            {
                return Quaternion::IDENTITY;
            }
            if (d < (1e-6f - 1.0f))
            {
                if (fallback_axis != Vector3::ZERO)
                {
     
                    q.fromAngleAxis(Radian(PI), fallback_axis);
                }
                else
                {

                    Vector3 axis = Vector3::UNIT_X.crossProduct(*this);
                    if (axis.isZeroLength()) 
                        axis = Vector3::UNIT_Y.crossProduct(*this);
                    axis.normalise();
                    q.fromAngleAxis(Radian(PI), axis);
                }
            }
            else
            {
                float s = sqrt((1 + d) * 2);
                float invs = 1 / s;

                Vector3 c = v0.crossProduct(v1);

                q.x = c.x * invs;
                q.y = c.y * invs;
                q.z = c.z * invs;
                q.w = s * 0.5f;
                q.normalise();
            }
            return q;
        }

        bool                isZeroLength(void) const
        {
            float sqlen = (x * x) + (y * y) + (z * z);
            return (sqlen < (1e-06 * 1e-06));
        }

        bool                isZero() const { return x == 0.f && y == 0.f && z == 0.f; }

        Vector3             normalisedCopy(void) const
        {
            Vector3 ret = *this;
            ret.normalise();
            return ret;
        }

        Vector3             reflect(const Vector3& normal) const
        {
            return Vector3(*this - (2 * this->dotProduct(normal) * normal));
        }

        Vector3             project(const Vector3& normal) const { return Vector3(*this - (this->dotProduct(normal) * normal)); }

        Vector3             absoluteCopy() const { return Vector3(fabsf(x), fabsf(y), fabsf(z)); }

        static Vector3      lerp(const Vector3& lhs, const Vector3& rhs, float alpha) { return lhs + alpha * (rhs - lhs); }

        static Vector3      clamp(const Vector3& v, const Vector3& min, const Vector3& max)
        {
            return Vector3(
                Math::clamp(v.x, min.x, max.x), Math::clamp(v.y, min.y, max.y), Math::clamp(v.z, min.z, max.z));
        }

        static float        getMaxElement(const Vector3& v) { return Math::getMaxElement(v.x, v.y, v.z); }
        bool         isNaN() const { return Math::isNan(x) || Math::isNan(y) || Math::isNan(z); }
 
	public:
		float		x = .0f;
		float		y = .0f;
		float		z = .0f;

        static const Vector3 ZERO;
        static const Vector3 UNIT_X;
        static const Vector3 UNIT_Y;
        static const Vector3 UNIT_Z;
        static const Vector3 NEGATIVE_UNIT_X;
        static const Vector3 NEGATIVE_UNIT_Y;
        static const Vector3 NEGATIVE_UNIT_Z;
        static const Vector3 UNIT_SCALE;
	};
}