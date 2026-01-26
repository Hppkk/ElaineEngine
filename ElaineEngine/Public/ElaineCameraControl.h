#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineVector3.h"
#include "ElaineQuaternion.h"

namespace Elaine
{
	class Camera;

	class ElaineEngineExport CameraControl
	{
	public:
		CameraControl(Camera* InCamera);
		~CameraControl();
		void Move(const Vector3& InDelta);
		void Rotation(const Quaternion& InDelta);
		void Rotation(const Vector3& InDelta);
	private:
		Camera* mCamera;
	};
}