#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMaterialInterface.h"
#include "ElaineMaterialInstanceStatic.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineResourceBase.h"
#include "render/common/ElaineRHI.h"
#include "math/ElaineAxisAlignedBox.h"

namespace Elaine
{
	enum VertexRate
	{
		PerVertex,
		PerInstance
	};

	struct VertexAttributeDesc
	{
		VertexSemantic Semantic;          // POSITION / NORMAL / TEXCOORD / COLOR / CUSTOM
		uint32 Index;           // TEXCOORD0 / TEXCOORD1
		VertexElementType Format;       // Float3 / Float2 / UByte4N ...
		uint32 Offset;          // 在一个 vertex 里的偏移
	};

	struct VertexBufferDesc
	{
		uint32 Stride;
		VertexRate Rate;
		std::vector<VertexAttributeDesc> Attributes;
	};

	struct VertexLayoutDesc
	{
		std::vector<VertexBufferDesc> Buffers;
	};

	struct MaterialSlot
	{
		uint64 SlotHash;
		std::string SlotName;
		ResourceReference DefaultMaterial;
	};

	class ElaineCoreExport SubMesh
	{
	public:
		SubMesh() = default;
		~SubMesh() = default;

		/// 索引起始位置（Mesh Index Buffer 中）
		uint32 StartIndex = 0;

		/// 索引数量
		uint32 IndexCount = 0;

		/// 顶点起始（非 0 时用于 BaseVertex）
		int32 BaseVertex = 0;

		uint32 MaterialSlot;
		MaterialInterface* MaterialInstance = nullptr;

		Name DebugName;

        AxisAlignedBox LocalBounds;

	public:

		inline bool IsValid() const
		{
			return IndexCount > 0 && MaterialInstance != nullptr;
		}

	};

    struct SubMeshDrawData
    {
        uint32 StartIndex = 0, IndexCount = 0, MaterialSlot = 0;
        int32 BaseVertex = 0;
    };
    struct MeshRenderData
    {
        VertexLayoutDesc Layout;
        std::vector<RHIBufferHandle> VertexBuffers;
        RHIBufferHandle IndexBuffer;
        uint32 IndexStride = 4;
        std::vector<SubMeshDrawData> SubMeshes;
    };

	class ElaineCoreExport Mesh : public ResourceBase
	{
	public:
		Mesh() = default;
		Mesh(ResourceManager* pManager, const std::string& res_name);
		virtual ~Mesh() override;
		uint32 GetSubMeshCount() const;
        MeshRenderData CreateRenderData() const;
        const std::vector<RHIBufferHandle>& GetVertexBuffers() const { return mGPUVertexBuffers; }
        const RHIBufferHandle& GetIndexBuffer() const { return mIndexBuffer; }
        uint32 GetIndexStride() const { return mIndexStride; }
        const VertexLayoutDesc& GetVertexLayout() const { return mVertexLayout; }
        const AxisAlignedBox& GetLocalBounds() const { return mLocalBounds; }
        const std::vector<MaterialSlot>& GetMaterialSlots() const { return mMaterialSlots; }
        MaterialInstanceStaticPtr GetDefaultMaterial(uint32 Slot) const;
        const std::vector<SubMesh*>& GetSubMeshes() const { return mSubMeshes; }
	protected:
		void ApplyLoadedData(ResourceLoadData& Data) noexcept override;
        std::unique_ptr<ResourceLoadData> LoadImpl() override;
        void PrepareDependencies(ResourceLoadData& Data) override;
        bool InitializeGPU(ResourceLoadData& Data) override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;
	private:
        struct LoadData;

		VertexLayoutDesc mVertexLayout;
		std::vector<SubMesh*> mSubMeshes;
		std::vector<std::vector<uint8_t>> mCPUVertexBuffers;
		std::vector<uint32_t> mCPUIndexBuffer;
		std::vector<RHIBufferHandle> mGPUVertexBuffers;
        std::vector<uint8_t> mIndexBytes;
        uint32 mIndexStride = 4;
        AxisAlignedBox mLocalBounds;
		std::vector<MaterialSlot> mMaterialSlots;
        std::vector<MaterialInstanceStaticPtr> mDefaultMaterials;
		RHIBufferHandle mIndexBuffer;
	};

	using MeshPtr = ResourcePtr<Mesh>;
}
