#pragma once
#include "ElaineMath.h"
#include "ElaineQuaternion.h"
#include "ElaineVector3.h"

namespace Elaine
{
	class ElaineCoreExport Matrix3x3
	{
	public:
        Matrix3x3() { operator=(IDENTITY); }

        explicit Matrix3x3(float arr[3][3])
        {
            memcpy(m_mat[0], arr[0], 3 * sizeof(float));
            memcpy(m_mat[1], arr[1], 3 * sizeof(float));
            memcpy(m_mat[2], arr[2], 3 * sizeof(float));
        }

        Matrix3x3(float(&float_array)[9])
        {
            m_mat[0][0] = float_array[0];
            m_mat[0][1] = float_array[1];
            m_mat[0][2] = float_array[2];
            m_mat[1][0] = float_array[3];
            m_mat[1][1] = float_array[4];
            m_mat[1][2] = float_array[5];
            m_mat[2][0] = float_array[6];
            m_mat[2][1] = float_array[7];
            m_mat[2][2] = float_array[8];
        }

        Matrix3x3(float entry00,
            float entry01,
            float entry02,
            float entry10,
            float entry11,
            float entry12,
            float entry20,
            float entry21,
            float entry22)
        {
            m_mat[0][0] = entry00;
            m_mat[0][1] = entry01;
            m_mat[0][2] = entry02;
            m_mat[1][0] = entry10;
            m_mat[1][1] = entry11;
            m_mat[1][2] = entry12;
            m_mat[2][0] = entry20;
            m_mat[2][1] = entry21;
            m_mat[2][2] = entry22;
        }

        Matrix3x3(const Vector3& row0, const Vector3& row1, const Vector3& row2)
        {
            m_mat[0][0] = row0.x;
            m_mat[0][1] = row0.y;
            m_mat[0][2] = row0.z;
            m_mat[1][0] = row1.x;
            m_mat[1][1] = row1.y;
            m_mat[1][2] = row1.z;
            m_mat[2][0] = row2.x;
            m_mat[2][1] = row2.y;
            m_mat[2][2] = row2.z;
        }

        Matrix3x3(const Quaternion& q)
        {
            float yy = q.y * q.y;
            float zz = q.z * q.z;
            float xy = q.x * q.y;
            float zw = q.z * q.w;
            float xz = q.x * q.z;
            float yw = q.y * q.w;
            float xx = q.x * q.x;
            float yz = q.y * q.z;
            float xw = q.x * q.w;

            m_mat[0][0] = 1 - 2 * yy - 2 * zz;
            m_mat[0][1] = 2 * xy + 2 * zw;
            m_mat[0][2] = 2 * xz - 2 * yw;

            m_mat[1][0] = 2 * xy - 2 * zw;
            m_mat[1][1] = 1 - 2 * xx - 2 * zz;
            m_mat[1][2] = 2 * yz + 2 * xw;

            m_mat[2][0] = 2 * xz + 2 * yw;
            m_mat[2][1] = 2 * yz - 2 * xw;
            m_mat[2][2] = 1 - 2 * xx - 2 * yy;
        }

        void fromData(float(&float_array)[9])
        {
            m_mat[0][0] = float_array[0];
            m_mat[0][1] = float_array[1];
            m_mat[0][2] = float_array[2];
            m_mat[1][0] = float_array[3];
            m_mat[1][1] = float_array[4];
            m_mat[1][2] = float_array[5];
            m_mat[2][0] = float_array[6];
            m_mat[2][1] = float_array[7];
            m_mat[2][2] = float_array[8];
        }

        void toData(float(&float_array)[9]) const
        {
            float_array[0] = m_mat[0][0];
            float_array[1] = m_mat[0][1];
            float_array[2] = m_mat[0][2];
            float_array[3] = m_mat[1][0];
            float_array[4] = m_mat[1][1];
            float_array[5] = m_mat[1][2];
            float_array[6] = m_mat[2][0];
            float_array[7] = m_mat[2][1];
            float_array[8] = m_mat[2][2];
        }

        float* operator[](size_t row_index) const { return (float*)m_mat[row_index]; }

        Vector3 getColumn(size_t col_index) const
        {
            assert(0 <= col_index && col_index < 3);
            return Vector3(m_mat[0][col_index], m_mat[1][col_index], m_mat[2][col_index]);
        }

        void setColumn(size_t iCol, const Vector3& vec);
        void fromAxes(const Vector3& x_axis, const Vector3& y_axis, const Vector3& z_axis);

        // assignment and comparison
        bool operator==(const Matrix3x3& rhs) const
        {
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    if (m_mat[row_index][col_index] != rhs.m_mat[row_index][col_index])
                        return false;
                }
            }

            return true;
        }

        bool operator!=(const Matrix3x3& rhs) const { return !operator==(rhs); }

        // arithmetic operations
        Matrix3x3 operator+(const Matrix3x3& rhs) const
        {
            Matrix3x3 sum;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    sum.m_mat[row_index][col_index] = m_mat[row_index][col_index] + rhs.m_mat[row_index][col_index];
                }
            }
            return sum;
        }

        Matrix3x3 operator-(const Matrix3x3& rhs) const
        {
            Matrix3x3 diff;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    diff.m_mat[row_index][col_index] = m_mat[row_index][col_index] - rhs.m_mat[row_index][col_index];
                }
            }
            return diff;
        }

        Matrix3x3 operator*(const Matrix3x3& rhs) const
        {
            Matrix3x3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                {
                    prod.m_mat[row_index][col_index] = m_mat[row_index][0] * rhs.m_mat[0][col_index] +
                        m_mat[row_index][1] * rhs.m_mat[1][col_index] +
                        m_mat[row_index][2] * rhs.m_mat[2][col_index];
                }
            }
            return prod;
        }

        Vector3 operator*(const Vector3& rhs) const
        {
            Vector3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                prod[row_index] =
                    m_mat[row_index][0] * rhs.x + m_mat[row_index][1] * rhs.y + m_mat[row_index][2] * rhs.z;
            }
            return prod;
        }

        friend Vector3 operator*(const Vector3& point, const Matrix3x3& rhs)
        {
            Vector3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                prod[row_index] = point.x * rhs.m_mat[0][row_index] + point.y * rhs.m_mat[1][row_index] +
                    point.z * rhs.m_mat[2][row_index];
            }
            return prod;
        }

        Matrix3x3 operator-() const
        {
            Matrix3x3 neg;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    neg[row_index][col_index] = -m_mat[row_index][col_index];
            }
            return neg;
        }

        Matrix3x3 operator*(float scalar) const
        {
            Matrix3x3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    prod[row_index][col_index] = scalar * m_mat[row_index][col_index];
            }
            return prod;
        }
        friend Matrix3x3 operator*(float scalar, const Matrix3x3& rhs)
        {
            Matrix3x3 prod;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    prod[row_index][col_index] = scalar * rhs.m_mat[row_index][col_index];
            }
            return prod;
        }

        Matrix3x3 transpose() const
        {
            Matrix3x3 transpose_v;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    transpose_v[row_index][col_index] = m_mat[col_index][row_index];
            }
            return transpose_v;
        }

        bool inverse(Matrix3x3& inv_mat, float fTolerance = 1e-06) const
        {

            float det = determinant();
            if (std::fabs(det) <= fTolerance)
                return false;

            inv_mat[0][0] = m_mat[1][1] * m_mat[2][2] - m_mat[1][2] * m_mat[2][1];
            inv_mat[0][1] = m_mat[0][2] * m_mat[2][1] - m_mat[0][1] * m_mat[2][2];
            inv_mat[0][2] = m_mat[0][1] * m_mat[1][2] - m_mat[0][2] * m_mat[1][1];
            inv_mat[1][0] = m_mat[1][2] * m_mat[2][0] - m_mat[1][0] * m_mat[2][2];
            inv_mat[1][1] = m_mat[0][0] * m_mat[2][2] - m_mat[0][2] * m_mat[2][0];
            inv_mat[1][2] = m_mat[0][2] * m_mat[1][0] - m_mat[0][0] * m_mat[1][2];
            inv_mat[2][0] = m_mat[1][0] * m_mat[2][1] - m_mat[1][1] * m_mat[2][0];
            inv_mat[2][1] = m_mat[0][1] * m_mat[2][0] - m_mat[0][0] * m_mat[2][1];
            inv_mat[2][2] = m_mat[0][0] * m_mat[1][1] - m_mat[0][1] * m_mat[1][0];

            float inv_det = 1.0f / det;
            for (size_t row_index = 0; row_index < 3; row_index++)
            {
                for (size_t col_index = 0; col_index < 3; col_index++)
                    inv_mat[row_index][col_index] *= inv_det;
            }

            return true;
        }

        Matrix3x3 inverse(float tolerance = 1e-06) const
        {
            Matrix3x3 inv = ZERO;
            inverse(inv, tolerance);
            return inv;
        }

        float determinant() const
        {
            float cofactor00 = m_mat[1][1] * m_mat[2][2] - m_mat[1][2] * m_mat[2][1];
            float cofactor10 = m_mat[1][2] * m_mat[2][0] - m_mat[1][0] * m_mat[2][2];
            float cofactor20 = m_mat[1][0] * m_mat[2][1] - m_mat[1][1] * m_mat[2][0];

            float det = m_mat[0][0] * cofactor00 + m_mat[0][1] * cofactor10 + m_mat[0][2] * cofactor20;

            return det;
        }

        void calculateQDUDecomposition(Matrix3x3& out_Q, Vector3& out_D, Vector3& out_U) const;

        // matrix must be orthonormal
        void toAngleAxis(Vector3& axis, Radian& angle) const;
        void toAngleAxis(Vector3& axis, Degree& angle) const
        {
            Radian r;
            toAngleAxis(axis, r);
            angle = r;
        }
        void fromAngleAxis(const Vector3& axis, const Radian& radian);

        static Matrix3x3 scale(const Vector3& scale)
        {
            Matrix3x3 mat = ZERO;

            mat.m_mat[0][0] = scale.x;
            mat.m_mat[1][1] = scale.y;
            mat.m_mat[2][2] = scale.z;
            
            return mat;
        }

        static const Matrix3x3 ZERO;
        static const Matrix3x3 IDENTITY;

	public:
		//float m_mat[3][3];
        union 
        {
        	struct
        	{
        		float m00, m01, m02;
        		float m10, m11, m12;
        		float m20, m21, m22;
        	};

        	float		m_mat[3][3];
        };

	};
}