#pragma once
#include "math/ElaineAxisAlignedBox.h"
#include <cstdint>

namespace Elaine
{
	// Interface for objects that can be inserted into the BVH
	class ElaineCoreExport ISpatialObject
	{
	public:
		virtual ~ISpatialObject() = default;

		virtual AxisAlignedBox GetBoundingBox() const = 0;
		virtual void* GetUserData() const = 0;
		virtual uint32_t GetUserType() const = 0;

		// The ID returned by the BVH upon insertion.
		// Objects must store this ID so they can be updated/removed later.
		virtual void SetBVHNodeID(int32_t ID) = 0;
		virtual int32_t GetBVHNodeID() const = 0;
	};
}
