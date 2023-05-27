#include "ElainePrecompiledHeader.h"
#include "math/ElaineAxisAlignedBox.h"

namespace Elaine
{

	void AxisAlignedBox::merge(const Vector3& new_point)
	{
		switch (m_extent)
		{
		case Extent_Null:
			setExtent(new_point, new_point);
			break;
		case Extent_Finite:
			mMaximum.makeCeil(new_point);
			mMinimum.makeFloor(new_point);
			break;
		case Extent_Infinite:
			break;
		default:
			break;
		}
	}

	void AxisAlignedBox::merge(const AxisAlignedBox& rhs)
	{
		if (rhs.m_extent == Extent_Null || m_extent == Extent_Infinite)
			return;

		if (rhs.m_extent == Extent_Infinite)
			m_extent = Extent_Infinite;
		else if (m_extent == Extent_Null)
			setExtent(rhs.mMinimum, rhs.mMaximum);
		else
		{
			Vector3 min = mMinimum;
			Vector3 max = mMaximum;
			max.makeCeil(rhs.mMaximum);
			min.makeFloor(rhs.mMinimum);
		}
	}

	void AxisAlignedBox::update(const Vector3& center, const Vector3& half_extent)
	{

	}

	void AxisAlignedBox::setNull()
	{
		m_extent = Extent_Null;
	}

	void AxisAlignedBox::setMinimum(Vector3 val)
	{
		m_extent = Extent_Finite;
		mMinimum = val;
		m_extent_val = mMaximum - mMinimum;
	}
	void AxisAlignedBox::setMinimum(float x, float y, float z)
	{
		m_extent = Extent_Finite;
		mMinimum.x = x;
		mMinimum.y = y;
		mMinimum.z = z;
		m_extent_val = mMaximum - mMinimum;
	}

	void AxisAlignedBox::setMaximum(Vector3 val)
	{
		m_extent = Extent_Finite;
		mMaximum = val;
		m_extent_val = mMaximum - mMinimum;
	}

	void AxisAlignedBox::setMaximum(float x, float y, float z)
	{
		m_extent = Extent_Finite;
		mMaximum.x = x;
		mMaximum.y = y;
		mMaximum.z = z;
		m_extent_val = mMaximum - mMinimum;
	}

	void AxisAlignedBox::setExtent(Vector3 min, Vector3 max)
	{
		m_extent = Extent_Finite;
		mMinimum = min;
		mMaximum = max;
		m_extent_val = mMaximum - mMinimum;
	}

	void AxisAlignedBox::setInfinite()
	{
		m_extent = Extent_Infinite;
	}

	const Vector3* AxisAlignedBox::getAllCorners()
	{
		assert(m_extent == Extent_Finite);
		if (!m_Corners)
			m_Corners = new Vector3[8];

		m_Corners[0] = mMinimum;
		m_Corners[1].x = mMinimum.x;
		m_Corners[1].y = mMaximum.y;
		m_Corners[1].z = mMinimum.z;

		m_Corners[2].x = mMaximum.x;
		m_Corners[2].y = mMaximum.y;
		m_Corners[2].z = mMinimum.z;

		m_Corners[3].x = mMaximum.x;
		m_Corners[3].y = mMinimum.y;
		m_Corners[3].z = mMinimum.z;

		m_Corners[4] = mMaximum;
		m_Corners[5].x = mMinimum.x;
		m_Corners[5].y = mMaximum.y;
		m_Corners[5].z = mMaximum.z;

		m_Corners[6].x = mMinimum.x;
		m_Corners[6].y = mMinimum.y;
		m_Corners[6].z = mMaximum.z;

		m_Corners[7].x = mMaximum.x;
		m_Corners[7].y = mMinimum.y;
		m_Corners[7].z = mMaximum.z;

		return m_Corners;
	}

	void AxisAlignedBox::transformAffine(const Matrix4x4& matrix)
	{
		if (m_extent != Extent_Finite)
			return;
		Vector3 halfSize = getHalfSize();
		Vector3 center = getCenter();
		Vector3 newCenter = matrix.transformAffine(center);

		Vector3 newHalfSize(
			Math::abs(matrix[0][0]) * halfSize.x + Math::abs(matrix[0][1]) * halfSize.y + Math::abs(matrix[0][2]) * halfSize.z,
			Math::abs(matrix[1][0]) * halfSize.x + Math::abs(matrix[1][1]) * halfSize.y + Math::abs(matrix[1][2]) * halfSize.z,
			Math::abs(matrix[2][0]) * halfSize.x + Math::abs(matrix[2][1]) * halfSize.y + Math::abs(matrix[2][2]) * halfSize.z);

		setExtent(newCenter - newHalfSize, newCenter + newHalfSize);
	}

	void AxisAlignedBox::scale(const Vector3& sca)
	{
		if (m_extent != Extent_Infinite)
			return;

		Vector3 min = mMinimum * sca;
		Vector3 max = mMaximum * sca;
		for (int i = 0; i < 3; ++i)
		{
			if (min[i] > max[i])
				std::swap(min[i], max[i]);
		}
		setExtent(min, max);
	}
}