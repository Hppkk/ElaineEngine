#pragma once
#include "ElaineVector3.h"

namespace Elaine
{
	class ElaineCoreExport Ray
	{
	public:
		Ray() : mOrigin(Vector3::ZERO), mDirection(Vector3::UNIT_Z) {}
		Ray(const Vector3& origin, const Vector3& direction)
			: mOrigin(origin), mDirection(direction) {}

		const Vector3& GetOrigin() const { return mOrigin; }
		const Vector3& GetDirection() const { return mDirection; }

		void SetOrigin(const Vector3& origin) { mOrigin = origin; }
		void SetDirection(const Vector3& direction) { mDirection = direction; }

		Vector3 GetPoint(float distance) const
		{
			return mOrigin + mDirection * distance;
		}

	private:
		Vector3 mOrigin;
		Vector3 mDirection;
	};
}
