#include "ElainePrecompiledHeader.h"
#include <bit>
#include "ElaineMesh.h"
#include "Resource/ElaineMeshAsset.h"
#include "ElaineDataStream.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineMeshManager.h"
#include "ElaineMaterialInstanceStatic.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
    struct Mesh::LoadData : ResourceLoadData
    {
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
        ~LoadData() override { for (auto* Sub : mSubMeshes) delete Sub; }
    };


	namespace fs = std::filesystem;

	void CheckAndCreateFile(const std::string& filePath)
	{
		fs::path p = fs::path(filePath);

		if (!fs::exists(p))
		{
			std::ofstream ofs(p);

			if (ofs.is_open())
			{
				ofs.close();
			}
		}
	}

	inline static VertexRate ParseVertexRate(const std::string& InRate)
	{
		static const std::unordered_map<std::string, VertexRate> sMap =
		{
			{ "PerVertex",          PerVertex },
			{ "PerInstance",        PerInstance }
		};

		auto it = sMap.find(InRate);
		if (it != sMap.end())
			return it->second;

		return PerVertex;
	}

	static VertexElementType ParseRHIFormat(const std::string& InFormat)
	{
		std::string fmt = InFormat;
		std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);

		static const std::unordered_map<std::string, VertexElementType> sFormatMap =
		{
			// Float
			{ "float1",     VET_Float1 },
			{ "float2",     VET_Float2 },
			{ "float3",     VET_Float3 },
			{ "float4",     VET_Float4 },

			// Half
			{ "half2",      VET_Half2 },
			{ "half4",      VET_Half4 },

			// Packed / Normalized
			{ "packednormal", VET_PackedNormal },
			{ "rgb10a2n",     VET_URGB10A2N },

			// Byte
			{ "ubyte4",     VET_UByte4 },
			{ "ubyte4n",    VET_UByte4N },

			// Short
			{ "short2",     VET_Short2 },
			{ "short4",     VET_Short4 },
			{ "short2n",    VET_Short2N },
			{ "short4n",    VET_Short4N },

			// UShort
			{ "ushort2",    VET_UShort2 },
			{ "ushort4",    VET_UShort4 },
			{ "ushort2n",   VET_UShort2N },
			{ "ushort4n",   VET_UShort4N },

			// Int
			{ "uint",       VET_UInt },

			// Color aliases
			{ "color",      VET_Color },
			{ "rgba8",      VET_Color },
			{ "rgba8n",     VET_Color },
		};

		auto it = sFormatMap.find(fmt);
		if (it != sFormatMap.end())
		{
			return it->second;
		}

		LOG_ERROR("Unknown vertex format: {}", InFormat);
		return VET_None;
	}

	static uint32 GetRHIFormatSize(VertexElementType InFormat)
	{
		static const std::unordered_map<VertexElementType, uint32> sFormatMap =
		{
			// Float
			{ VET_Float1, 4 },
			{ VET_Float2, 8 },
			{ VET_Float3, 12 },
			{ VET_Float4, 16 },

			// Half
			{ VET_Half2, 4 },
			{ VET_Half4, 8 },

			// Packed / Normalized
			{ VET_PackedNormal, 4 },
			{ VET_URGB10A2N, 4 },

			// Byte
			{ VET_UByte4, 4 },
			{ VET_UByte4N, 4 },

			// Short
			{ VET_Short2, 4 },
			{ VET_Short4, 8 },
			{ VET_Short2N, 4 },
			{ VET_Short4N, 8 },

			// UShort
			{ VET_UShort2, 4 },
			{ VET_UShort4, 8 },
			{ VET_UShort2N, 4 },
			{ VET_UShort4N, 8 },

			// Int
			{ VET_UInt, 4 },

			// Color aliases
			{ VET_Color, 4 }
		};

		auto it = sFormatMap.find(InFormat);
		if (it != sFormatMap.end())
		{
			return it->second;
		}

		LOG_ERROR("Unknown vertex format: {}", InFormat);
		return 4;
	}


	//===================== Mesh Binary Data Stream =====================

	struct MeshFileHeader
	{
		uint32 Magic;          // 'EMSH'
		uint32 Version;        // 1
		uint32 VertexBufferCount;
		uint32 SubMeshCount;
		uint32 IndexCount;
		uint32 IndexStride;    // 2 or 4 (short or uint)
		uint32 MaterialSlotCount;
	};

	struct VertexElementBinary
	{
		uint8 Semantic;        // VertexSemantic enum
		uint8 Index;           // TEXCOORD0/1
		uint8 Type;            // VertexElementType
		uint8 Offset;
	};

	struct VertexBufferLayoutBinary
	{
		uint32 Stride;
		uint32 ElementCount;
	};

	struct SubMeshBinary
	{
		uint32 StartIndex;
		uint32 IndexCount;
		uint32 MaterialSlotIndex;
	};

    static void WriteMeshUInt(std::vector<uint8_t>& Out, uint64_t Value, size_t Width = 4)
    { for (size_t I = 0; I < Width; ++I) Out.push_back(static_cast<uint8_t>(Value >> (8 * I))); }
    static void WriteMeshHeader(std::vector<uint8_t>& Out, const MeshFileHeader& H)
    {
        for (uint32_t Value : {H.Magic, H.Version, H.VertexBufferCount, H.SubMeshCount, H.IndexCount, H.IndexStride, H.MaterialSlotCount}) WriteMeshUInt(Out, Value);
    }
	//===================================================================

	Mesh::Mesh(ResourceManager* pManager, const std::string& res_name)
		: ResourceBase(pManager, res_name)
	{

	}

	Mesh::~Mesh()
	{
		for (auto&& Sub : mSubMeshes)
		{
			SAFE_DELETE(Sub);
		}
	}

	uint32 Mesh::GetSubMeshCount() const { return static_cast<uint32>(mSubMeshes.size()); }

    std::unique_ptr<ResourceLoadData> Mesh::LoadImpl()
    {
        auto Pending = std::make_unique<LoadData>();
        auto& Data = *Pending;
        const auto Payload = AssetFile::ReadSection(std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName, "data");
        const auto* Bytes = Payload.data();
        const size_t ByteCount = Payload.size();
        size_t Offset = 0;
        auto Read = [&](void* Dest, size_t Size) {
            if (Size > ByteCount - Offset) throw std::runtime_error("Truncated mesh data");
            memcpy(Dest, Bytes + Offset, Size); Offset += Size;
        };
        auto UInt = [&](size_t Width = 4) { uint8_t Field[8]{}; Read(Field, Width); uint64_t Value = 0; for (size_t I=0; I<Width; ++I) Value |= uint64_t(Field[I]) << (I*8); return Value; };
        MeshFileHeader Header{};
        Header.Magic = static_cast<uint32_t>(UInt()); Header.Version = static_cast<uint32_t>(UInt());
        Header.VertexBufferCount = static_cast<uint32_t>(UInt()); Header.SubMeshCount = static_cast<uint32_t>(UInt());
        Header.IndexCount = static_cast<uint32_t>(UInt()); Header.IndexStride = static_cast<uint32_t>(UInt()); Header.MaterialSlotCount = static_cast<uint32_t>(UInt());
        if (Header.Magic != 'EMSH' || Header.Version != 1 || (Header.IndexStride != 2 && Header.IndexStride != 4)) return {};
        if (!Header.VertexBufferCount || !Header.IndexCount || Header.VertexBufferCount > 16 || Header.VertexBufferCount > ByteCount / sizeof(VertexBufferLayoutBinary) || Header.SubMeshCount > ByteCount / sizeof(SubMeshBinary) || Header.MaterialSlotCount > ByteCount / 12) return {};
        for (uint32 i = 0; i < Header.MaterialSlotCount; ++i)
        {
            MaterialSlot Slot{}; uint32 Size = 0;
            Slot.SlotHash = UInt(8); Size = static_cast<uint32_t>(UInt());
            if (Size > ByteCount - Offset) return {};
            Slot.SlotName.resize(Size); Read(Slot.SlotName.data(), Size);
            Data.mMaterialSlots.push_back(std::move(Slot));
        }
        for (uint32 i = 0; i < Header.VertexBufferCount; ++i)
        {
            VertexBufferLayoutBinary Layout{}; Layout.Stride = static_cast<uint32_t>(UInt()); Layout.ElementCount = static_cast<uint32_t>(UInt());
            if (!Layout.Stride || !Layout.ElementCount || Layout.ElementCount > (ByteCount - Offset) / sizeof(VertexElementBinary)) return {};
            VertexBufferDesc Desc{}; Desc.Stride = Layout.Stride; Desc.Rate = PerVertex;
            for (uint32 e = 0; e < Layout.ElementCount; ++e)
            {
                VertexElementBinary Element{}; Element.Semantic=static_cast<uint8_t>(UInt(1)); Element.Index=static_cast<uint8_t>(UInt(1)); Element.Type=static_cast<uint8_t>(UInt(1)); Element.Offset=static_cast<uint8_t>(UInt(1));
                if (Element.Type == VET_None || Element.Type >= VET_MAX || Element.Offset > Layout.Stride || GetRHIFormatSize(static_cast<VertexElementType>(Element.Type)) > Layout.Stride - Element.Offset) return {};
                Desc.Attributes.push_back({static_cast<VertexSemantic>(Element.Semantic), Element.Index, static_cast<VertexElementType>(Element.Type), Element.Offset});
            }
            Data.mVertexLayout.Buffers.push_back(std::move(Desc));
        }
        for (uint32 i = 0; i < Header.SubMeshCount; ++i)
        {
            SubMeshBinary Source{}; Source.StartIndex=static_cast<uint32_t>(UInt()); Source.IndexCount=static_cast<uint32_t>(UInt()); Source.MaterialSlotIndex=static_cast<uint32_t>(UInt());
            if (Source.MaterialSlotIndex >= Header.MaterialSlotCount || Source.StartIndex > Header.IndexCount || Source.IndexCount > Header.IndexCount - Source.StartIndex) return {};
            auto Sub = std::make_unique<SubMesh>(); Sub->StartIndex = Source.StartIndex; Sub->IndexCount = Source.IndexCount; Sub->MaterialSlot = Source.MaterialSlotIndex;
            Data.mSubMeshes.push_back(Sub.get());
            Sub.release();
        }
        Data.mCPUVertexBuffers.resize(Header.VertexBufferCount);
        for (uint32 i = 0; i < Header.VertexBufferCount; ++i)
        {
            uint32 Size = static_cast<uint32_t>(UInt());
            if (!Size || Size > ByteCount - Offset || Size % Data.mVertexLayout.Buffers[i].Stride || (i && Size / Data.mVertexLayout.Buffers[i].Stride != Data.mCPUVertexBuffers[0].size() / Data.mVertexLayout.Buffers[0].Stride)) return {};
            Data.mCPUVertexBuffers[i].resize(Size); Read(Data.mCPUVertexBuffers[i].data(), Size);
        }
        uint32 IndexBytes = static_cast<uint32_t>(UInt());
        if (uint64_t(Header.IndexCount) * Header.IndexStride != IndexBytes || IndexBytes > ByteCount - Offset) return {};
        Data.mIndexStride = Header.IndexStride;
        Data.mIndexBytes.resize(IndexBytes); Read(Data.mIndexBytes.data(), IndexBytes);
        Data.mCPUIndexBuffer.resize(Header.IndexCount);
        for (uint32 i = 0; i < Header.IndexCount; ++i)
        {
            uint32 Index = 0; memcpy(&Index, Data.mIndexBytes.data() + size_t(i) * Data.mIndexStride, Data.mIndexStride);
            Data.mCPUIndexBuffer[i] = Index;
            if (!Data.mCPUVertexBuffers.empty() && Index >= Data.mCPUVertexBuffers.front().size() / Data.mVertexLayout.Buffers.front().Stride) return {};
        }
        if (Offset != ByteCount) return {};
        const auto File = std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName;
        const auto MetadataBytes = AssetFile::ReadSection(File, "mesh");
        const auto Metadata = JsonCpp::parse(MetadataBytes.begin(), MetadataBytes.end());
        if (Metadata.at("version") != 1 || Metadata.at("subMeshes").size() != Data.mSubMeshes.size() ||
            Metadata.at("materialSlots").size() != Data.mMaterialSlots.size()) throw std::runtime_error("Mesh metadata does not match geometry");
        Data.mLocalBounds = MeshBoundsFromJson(Metadata.at("bounds"));
        if (Data.mLocalBounds.isNull()) throw std::runtime_error("Mesh bounds are missing");
        for (size_t I=0; I<Data.mSubMeshes.size(); ++I)
        {
            Data.mSubMeshes[I]->LocalBounds = MeshBoundsFromJson(Metadata.at("subMeshes")[I].at("bounds"));
            if (Data.mSubMeshes[I]->IndexCount && Data.mSubMeshes[I]->LocalBounds.isNull()) throw std::runtime_error("SubMesh bounds are missing");
            const auto& B = Data.mSubMeshes[I]->LocalBounds;
            if (Data.mLocalBounds.m_extent == AxisAlignedBox::Extent_Finite && B.m_extent == AxisAlignedBox::Extent_Finite)
                for (size_t C=0; C<3; ++C) if (B.getMin()[C] < Data.mLocalBounds.getMin()[C] || B.getMax()[C] > Data.mLocalBounds.getMax()[C])
                    throw std::runtime_error("SubMesh bounds exceed Mesh bounds");
        }
        for (size_t I=0; I<Data.mMaterialSlots.size(); ++I)
        {
            const auto& Slot = Metadata.at("materialSlots")[I];
            if (Slot.contains("defaultMaterial")) Data.mMaterialSlots[I].DefaultMaterial = ResourceReference::FromJson(Slot.at("defaultMaterial"));
        }
        AssetFile::ValidateDependencies(AssetFile::ReadDescriptor(File), Metadata);
        Data.MemoryUsage = ByteCount;
        return Pending;
    }

    void Mesh::PrepareDependencies(ResourceLoadData& InData)
    {
        auto& Data = static_cast<LoadData&>(InData);
        Data.mDefaultMaterials.resize(Data.mMaterialSlots.size());
        for (size_t I=0; I<Data.mMaterialSlots.size(); ++I)
            if (Data.mMaterialSlots[I].DefaultMaterial.Id.IsValid())
                Data.mDefaultMaterials[I] = static_pointer_cast<MaterialInstanceStatic>(Data.GetDependency(Data.mMaterialSlots[I].DefaultMaterial, "MaterialInstance"));
    }
    MaterialInstanceStaticPtr Mesh::GetDefaultMaterial(uint32 Slot) const
    { return Slot < mDefaultMaterials.size() ? mDefaultMaterials[Slot] : MaterialInstanceStaticPtr{}; }

    bool Mesh::InitializeGPU(ResourceLoadData& InData)
    {
        auto& Data = static_cast<LoadData&>(InData);
        CheckRHICreationThread();
        auto* Context = GetDynamicRHI()->GetDefaultCommandContext();
        for (size_t i = 0; i < Data.mCPUVertexBuffers.size(); ++i)
        {
            auto& Bytes = Data.mCPUVertexBuffers[i];
            auto Buffer = Context->RHICreateBuffer(static_cast<uint32>(Bytes.size()), BufferUsageFlags::VertexBuffer | BufferUsageFlags::Static,
                Data.mVertexLayout.Buffers[i].Stride, ERHIAccess::Unknown, nullptr);
            if (Buffer) Context->UploadBuffer(Buffer.get(), Bytes.data(), static_cast<uint32>(Bytes.size()), ERHIAccess::VertexOrIndexBuffer);
            if (!Buffer) return false;
            Data.mGPUVertexBuffers.push_back(std::move(Buffer));
        }
        Data.mIndexBuffer = Context->RHICreateBuffer(static_cast<uint32>(Data.mIndexBytes.size()), BufferUsageFlags::IndexBuffer | BufferUsageFlags::Static,
            Data.mIndexStride, ERHIAccess::Unknown, nullptr);
        if (Data.mIndexBuffer) Context->UploadBuffer(Data.mIndexBuffer.get(), Data.mIndexBytes.data(), static_cast<uint32>(Data.mIndexBytes.size()), ERHIAccess::VertexOrIndexBuffer);
        return Data.mIndexBuffer != nullptr;
    }

	void Mesh::UnloadImpl()
    {
        for (auto* Sub : mSubMeshes) delete Sub;
        mSubMeshes.clear(); mCPUVertexBuffers.clear(); mCPUIndexBuffer.clear();
        mGPUVertexBuffers.clear(); mIndexBuffer.reset(); mIndexBytes.clear();
        mVertexLayout.Buffers.clear(); mMaterialSlots.clear(); mDefaultMaterials.clear();
        mLocalBounds.setNull();
    }

    void Mesh::SaveResourceImpl()
    {
        if (mVertexLayout.Buffers.size() != mCPUVertexBuffers.size() || mVertexLayout.Buffers.empty()) return;
        std::vector<uint8_t> Payload;
        auto Write = [&](const void* Data, size_t Size) { const auto* Bytes=static_cast<const uint8_t*>(Data); Payload.insert(Payload.end(),Bytes,Bytes+Size); };
        MeshFileHeader Header{}; Header.Magic = 'EMSH'; Header.Version = 1;
        Header.VertexBufferCount = static_cast<uint32>(mCPUVertexBuffers.size());
        Header.SubMeshCount = static_cast<uint32>(mSubMeshes.size()); Header.MaterialSlotCount = static_cast<uint32>(mMaterialSlots.size());
        Header.IndexCount = static_cast<uint32>(mCPUIndexBuffer.size()); Header.IndexStride = mIndexBytes.empty() ? 4 : mIndexStride;
        WriteMeshHeader(Payload, Header);
        for (const auto& Slot : mMaterialSlots)
        {
            uint32 Size = static_cast<uint32>(Slot.SlotName.size());
            WriteMeshUInt(Payload, Slot.SlotHash, 8); WriteMeshUInt(Payload, Size); Write(Slot.SlotName.data(), Size);
        }
        for (const auto& Buffer : mVertexLayout.Buffers)
        {
            VertexBufferLayoutBinary Layout{}; Layout.Stride = Buffer.Stride; Layout.ElementCount = static_cast<uint32>(Buffer.Attributes.size());
            WriteMeshUInt(Payload, Layout.Stride); WriteMeshUInt(Payload, Layout.ElementCount);
            for (const auto& Attr : Buffer.Attributes)
            {
                VertexElementBinary Element{}; Element.Semantic = Attr.Semantic; Element.Index = Attr.Index; Element.Type = Attr.Format; Element.Offset = Attr.Offset;
                for (uint8_t V : {Element.Semantic,Element.Index,Element.Type,Element.Offset}) WriteMeshUInt(Payload, V, 1);
            }
        }
        for (const auto* Sub : mSubMeshes)
        {
            SubMeshBinary Binary{}; Binary.StartIndex = Sub->StartIndex; Binary.IndexCount = Sub->IndexCount; Binary.MaterialSlotIndex = Sub->MaterialSlot;
            WriteMeshUInt(Payload, Binary.StartIndex); WriteMeshUInt(Payload, Binary.IndexCount); WriteMeshUInt(Payload, Binary.MaterialSlotIndex);
        }
        for (const auto& Buffer : mCPUVertexBuffers)
        { uint32 Size = static_cast<uint32>(Buffer.size()); WriteMeshUInt(Payload, Size); Write(Buffer.data(), Size); }
        uint32 Size = Header.IndexCount * Header.IndexStride; WriteMeshUInt(Payload, Size);
        for (const auto Index : mCPUIndexBuffer) WriteMeshUInt(Payload, Index, Header.IndexStride);
        auto Descriptor = GetDescriptor(); Descriptor.Tags = {{"SubMeshes", Header.SubMeshCount}, {"IndexCount", Header.IndexCount}, {"IndexStride", Header.IndexStride}, {"VertexBuffers", Header.VertexBufferCount}, {"MaterialSlots", Header.MaterialSlotCount}};
        const auto File = std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName;
        AssetFile::Sections Sections;
        const auto Existing = AssetFile::ReadMetadata(File);
        for (const auto& Entry : Existing.at("sections").items()) if (Entry.key() != "data") Sections[Entry.key()] = AssetFile::ReadSection(File, Entry.key());
        auto Metadata = JsonCpp::parse(Sections.at("mesh").begin(), Sections.at("mesh").end());
        Metadata["bounds"] = MeshBoundsToJson(mLocalBounds);
        for (size_t I=0; I<mSubMeshes.size(); ++I) Metadata["subMeshes"][I]["bounds"] = MeshBoundsToJson(mSubMeshes[I]->LocalBounds);
        for (size_t I=0; I<mMaterialSlots.size(); ++I)
        {
            auto& Slot = Metadata["materialSlots"][I];
            if (mMaterialSlots[I].DefaultMaterial.Id.IsValid()) Slot["defaultMaterial"] = mMaterialSlots[I].DefaultMaterial.ToJson();
            else Slot.erase("defaultMaterial");
        }
        auto Text = Metadata.dump(); Sections["mesh"] = {Text.begin(), Text.end()}; Sections["data"] = std::move(Payload);
        Descriptor.DataVersion = 2; Descriptor.Tags["Bounds"] = Metadata["bounds"];
        Descriptor.Dependencies = AssetFile::CollectDependencies(Metadata, "Mesh");
        AssetFile::Write(File, Descriptor, Sections);
    }



    void Mesh::ApplyLoadedData(ResourceLoadData& InData) noexcept
    {
        auto& Data = static_cast<LoadData&>(InData);
        std::swap(mVertexLayout, Data.mVertexLayout);
        std::swap(mSubMeshes, Data.mSubMeshes);
        std::swap(mCPUVertexBuffers, Data.mCPUVertexBuffers);
        std::swap(mCPUIndexBuffer, Data.mCPUIndexBuffer);
        std::swap(mGPUVertexBuffers, Data.mGPUVertexBuffers);
        std::swap(mIndexBytes, Data.mIndexBytes);
        std::swap(mIndexStride, Data.mIndexStride);
        std::swap(mMaterialSlots, Data.mMaterialSlots);
        std::swap(mDefaultMaterials, Data.mDefaultMaterials);
        std::swap(mIndexBuffer, Data.mIndexBuffer);
        std::swap(mLocalBounds, Data.mLocalBounds);
    }

    MeshRenderData Mesh::CreateRenderData() const
    {
        MeshRenderData Data;
        Data.Layout = mVertexLayout; Data.VertexBuffers = mGPUVertexBuffers;
        Data.IndexBuffer = mIndexBuffer; Data.IndexStride = mIndexStride;
        for (const auto* Sub : mSubMeshes)
            Data.SubMeshes.push_back({Sub->StartIndex, Sub->IndexCount, Sub->MaterialSlot, Sub->BaseVertex});
        return Data;
    }

}
