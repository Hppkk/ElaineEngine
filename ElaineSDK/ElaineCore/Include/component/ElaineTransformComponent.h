#pragma once
#include "component/ElaineComponent.h"
#include "math/ElaineTransform.h"

struct cJSON;
namespace Elaine
{
	class ElaineCoreExport TransformComponentInfo :public EComponentInfo
	{
	public:
		TransformComponentInfo();
		virtual ~TransformComponentInfo();
		virtual void			exportDataImpl(cJSON* jsonNode) override;
		virtual void			importDataImpl(cJSON* jsonNode) override;
	public:
		Transform*				m_pTransform;
	};


	class ElaineCoreExport TransformComponent :public EComponent
	{
		friend class EGameObject;
	public:
		TransformComponent();
		virtual ~TransformComponent();
		Matrix4x4		getWorldMatrix();
		Vector3			getWorldScale();
		Vector3			getWorldPosition();
		Quaternion		getWorldRotation();
		void			setWorldScale(const Vector3& rhs);
		void			setWorldPosition(const Vector3& rhs);
		void			setWorldRotation(const Quaternion& rhs);
	protected:
		virtual void					beginFrameTick(float dt) override;
	protected:
		static std::string				m_sType;
	private:
		TransformComponentInfo*			m_info;
	};

	class TransformComponentFactory :public ComponentFactory
	{
	public:
		virtual ~TransformComponentFactory(){ }
		virtual EComponent* createComponent() { return new TransformComponent(); }
	};
}