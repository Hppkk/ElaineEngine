#pragma once
#include "ElaineVector3.h"
#include "ElaineMatrix4.h"

namespace Elaine
{
	class ElaineCoreExport AxisAlignedBox
	{
	public:
		enum Extent
		{
			Extent_Null,
			Extent_Finite,
			Extent_Infinite,
		};
	public:
		AxisAlignedBox() :mMinimum(Vector3::ZERO), mMaximum(Vector3::UNIT_SCALE), m_center(0)
		{
			setMinimum(FLT_MAX, FLT_MAX, FLT_MAX);
			setMaximum(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			m_extent = Extent_Null;
		}

		AxisAlignedBox(const Vector3& min, const Vector3& max) :mMinimum(Vector3::ZERO), mMaximum(Vector3::UNIT_SCALE), m_center(0)
		{
			setMinimum(min);
			setMaximum(max);
			m_extent_val = max - min;
			m_extent = Extent_Finite;
		}

		AxisAlignedBox(const AxisAlignedBox& box) :mMinimum(Vector3::ZERO), mMaximum(Vector3::UNIT_SCALE), m_center(0)
		{
			if (box.isNull())
				setNull();
			else if (box.m_extent == Extent_Infinite)
				setInfinite();
			else
				setExtent(box.mMinimum, box.mMaximum);
		}
		~AxisAlignedBox() { SAFE_DELETE(m_Corners); }

		void			merge(const Vector3& new_point);
		void			merge(const AxisAlignedBox& rhs);

		const Vector3&	getCenter() const { return m_center; }
		const Vector3	getHalfSize() const { return m_extent_val * 0.5; }
		const Vector3&	getSize() const { return m_extent_val; }
		const Vector3&	getMin() const { return mMinimum; }
		const Vector3&	getMax() const { return mMaximum; }
		void			setMinimum(Vector3 val);
		void			setMinimum(float x, float y, float z);
		void			setMaximum(Vector3 val);
		void			setMaximum(float x, float y, float z);
		void			setExtent(Vector3 min, Vector3 max);
		void			setInfinite();
		const Vector3*	getAllCorners();
		void			transformAffine(const Matrix4x4& matrix);
		void			scale(const Vector3& scale);

		void			setNull();
		bool			isNull() const { return m_extent == Extent_Null; }
	public:
		Vector3			m_center{ Vector3::ZERO };
		Vector3			m_extent_val{ Vector3::ZERO };
		Vector3*		m_Corners = nullptr;
		Extent			m_extent;
		union
		{
			Vector3		mMinimum;
			Vector3		vMin;
		};
		union 
		{
			Vector3		mMaximum;
			Vector3		vMax;
		};
	};
}