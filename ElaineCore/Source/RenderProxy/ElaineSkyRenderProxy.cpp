#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineSkyRenderProxy.h"
#include "ElaineRenderQueue.h"

namespace Elaine
{
    SkyRenderProxy::SkyRenderProxy()
    {
        mType = EProxyType::Sky;
        mWorldAABB = AxisAlignedBox(Vector3(-2000), Vector3(2000));
    }

    SkyRenderProxy::~SkyRenderProxy()
    {
        CancelResourceNotifications();
    }

    void SkyRenderProxy::InitializeResourceBinding()
    {
        // Position is the only vertex input used by the sky shader.
        // All faces have the same outward winding; sampling uses world direction.
        const std::array<Vector3, 6> Normals = {Vector3::UNIT_X, -Vector3::UNIT_X,
            Vector3::UNIT_Y, -Vector3::UNIT_Y, Vector3::UNIT_Z, -Vector3::UNIT_Z};
        std::vector<Vector3> Vertices;
        for (const auto& Normal : Normals)
        {
            const Vector3 Up = std::abs(Normal.y) > 0.5f ? Vector3::UNIT_Z : Vector3::UNIT_Y;
            const Vector3 Right = Up.crossProduct(Normal);
            const Vector3 FaceUp = Normal.crossProduct(Right);
            const Vector3 Corners[] = {Normal - Right - FaceUp, Normal + Right - FaceUp,
                Normal + Right + FaceUp, Normal - Right + FaceUp};
            for (int Index : {0, 1, 2, 0, 2, 3}) Vertices.push_back(Corners[Index]);
        }
        auto VertexBuffer = RenderSystem::instance()->CreateBuffer(BufferUsageFlags::VertexBuffer,
            ERHIAccess::VertexOrIndexBuffer, Vertices.data(), Vertices.size() * sizeof(Vector3));
        mResourceBinding.mDrawData.mStreamInput.mIStreamBuffer[STREAM_VERTEXBUFFER] = VertexBuffer;

        mResourceBinding.mVertexCount = 36;
        mResourceBinding.mInstanceCount = 1;
        mMaterialProxy.BindToResourceBinding(mResourceBinding);


    }

    void SkyRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
    {
        if (!IsVisible() || !IsBindingsInitialized())
            return;

        // 使用 RenderMaterialProxy 的 IsReady 检查
        if (!mMaterialProxy.IsReady())
            return;

        mMaterialProxy.BindToResourceBinding(mResourceBinding);
        ShaderPass* SkyPass = mMaterialProxy.GetPass(Name("Sky"));
        if (SkyPass == nullptr)
            return;

        RenderQueue* CurrentRenderQueue = InRenderQueue->GetRenderQueue(RenderQueue_Sky);
        CurrentRenderQueue->UpdateRenderQueue(SkyPass, this, -1000);
    }

    void SkyRenderProxy::ApplyResourceUpdate(const RenderResourceUpdate& Update)
    {
        mMaterialProxy.ApplyResourceUpdate(Update);
        mMaterialProxy.BindToResourceBinding(mResourceBinding);

    }
    void SkyRenderProxy::PrepareResourceBinding()
    {

    }

}
