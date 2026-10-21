#include "ElainePrecompiledHeader.h"
#include "ElaineMesh.h"
#include "ElaineDataStream.h"
#include "ElaineUniformGPUManager.h"
#ifdef _HAS_EDITOR_
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#endif
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
        std::vector<MaterialSlot> mMaterialSlots;
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

	//===================================================================

	static TextureSemantics AssimpTextureConvert(aiTextureType type)
	{
		switch (type)
		{
		case aiTextureType_DIFFUSE:  return TextureSemantics::BaseColor;
		case aiTextureType_NORMALS:  return TextureSemantics::Normal;
		case aiTextureType_METALNESS:return TextureSemantics::Metallic;
		case aiTextureType_DIFFUSE_ROUGHNESS:return TextureSemantics::Roughness;
		default: return TextureSemantics::BaseColor;
		}
	}

	struct ImportVertex
	{
		float Position[3];
		float Normal[3];
		float Tangent[3];
		float Color[1][4];
		float UV[2]; //todo if other uv use UV[2][2]
	};


#ifdef _HAS_EDITOR_
	class MeshConverter
	{
		friend void ImportMeshFromFile(const std::string& srcPath, const std::string& destRelPath);
	public:
		static MeshPtr ConvertToInternalMesh(const std::string& InModelPath, const std::string& InSavePath)
		{
			vertices.clear();
			indices.clear();
			submeshes.clear();
			materialSlots.clear();
			SlotMap.clear();

			Assimp::Importer AIModelImporter;

			const aiScene* AIScene = AIModelImporter.ReadFile(
				InModelPath,
				aiProcess_Triangulate |
				aiProcess_GenNormals |
				aiProcess_CalcTangentSpace |
				aiProcess_JoinIdenticalVertices |
				aiProcess_ImproveCacheLocality
			);

			if (!AIScene || !AIScene->HasMeshes())
			{
				LOG_ERROR("Failed to load model :", InModelPath);
				return nullptr;
			}

			// Pre Process Memory
			uint32 IndexCount = 0;
			uint32 SubMeshCount = 0;
			uint32 VertexRequireMemory = 0;
			uint32 VertexCount = 0;
			bool bInitializeVertexSize = false;

			MeshPtr OutMesh = MeshManager::instance()->CreateEmptyResource<Mesh>(InSavePath);

			OutMesh->mVertexLayout.Buffers.push_back({});

			VertexBufferDesc& BuffDesc = OutMesh->mVertexLayout.Buffers[0];
			BuffDesc.Rate = PerVertex;

			for (uint32 NumMesh = 0; NumMesh < AIScene->mNumMeshes; ++NumMesh)
			{
				aiMesh* MeshAssimp = AIScene->mMeshes[NumMesh];
				IndexCount += MeshAssimp->mNumFaces * 3;
				++SubMeshCount;
				VertexCount += MeshAssimp->mNumVertices;

				// VertexSemantic::POSITION   VET_Float3
				// VertexSemantic::NORMAL     VET_Float3
				// VertexSemantic::TANGENT    VET_Float3
				// VertexSemantic::COLOR      VET_Float4
				// VertexSemantic::TEXCOORD   VET_Float2

				if (!bInitializeVertexSize)
				{
					if (MeshAssimp->HasPositions())
					{
						BuffDesc.Attributes.push_back(
							{
								.Semantic = POSITION,
								.Index = 0,
								.Format = VET_Float3,
								.Offset = VertexRequireMemory
							}
						);

						VertexRequireMemory += GetRHIFormatSize(VET_Float3);
					}

					if (MeshAssimp->HasNormals())
					{
						BuffDesc.Attributes.push_back(
							{
								.Semantic = NORMAL,
								.Index = 0,
								.Format = VET_Float3,
								.Offset = VertexRequireMemory
							}
						);

						VertexRequireMemory += GetRHIFormatSize(VET_Float3);
					}

					if (MeshAssimp->HasTangentsAndBitangents())
					{
						BuffDesc.Attributes.push_back(
							{
								.Semantic = TANGENT,
								.Index = 0,
								.Format = VET_Float3,
								.Offset = VertexRequireMemory
							}
						);

						VertexRequireMemory += GetRHIFormatSize(VET_Float3);
					}

					if (MeshAssimp->HasVertexColors(0))
					{
						BuffDesc.Attributes.push_back(
							{
								.Semantic = COLOR,
								.Index = 0,
								.Format = VET_Float4,
								.Offset = VertexRequireMemory
							}
						);

						VertexRequireMemory += GetRHIFormatSize(VET_Float4);
					}

					if (MeshAssimp->HasTextureCoords(0))
					{
						BuffDesc.Attributes.push_back(
							{
								.Semantic = TEXCOORD,
								.Index = 0,
								.Format = VET_Float2,
								.Offset = VertexRequireMemory
							}
						);

						VertexRequireMemory += GetRHIFormatSize(VET_Float2);
					}

					BuffDesc.Stride = VertexRequireMemory;
					bInitializeVertexSize = true;
				}
			}

			VertexRequireMemory *= VertexCount;

			OutMesh->mCPUIndexBuffer.reserve(IndexCount);
			OutMesh->mCPUVertexBuffers.resize(1);
			OutMesh->mCPUVertexBuffers[0].resize(VertexRequireMemory);
			OutMesh->mSubMeshes.resize(SubMeshCount);
			OutMesh->mMaterialSlots.reserve(AIScene->mNumMaterials);

			uint32 BaseVertex = 0;
			uint32 BaseIndex = 0;

			for (uint32 NumMesh = 0; NumMesh < AIScene->mNumMeshes; ++NumMesh)
			{
				aiMesh* MeshAssimp = AIScene->mMeshes[NumMesh];

				OutMesh->mSubMeshes[NumMesh] = new SubMesh();
				SubMesh* SubMeshPtr = OutMesh->mSubMeshes[NumMesh];
				SubMeshPtr->StartIndex = BaseIndex;
				SubMeshPtr->IndexCount = MeshAssimp->mNumFaces * 3;

				if (MeshAssimp->mMaterialIndex >= 0)
				{
					aiMaterial* MaterialAssimp = AIScene->mMaterials[MeshAssimp->mMaterialIndex];
					uint32 slot = GetOrCreateSlot(MaterialAssimp);
					SubMeshPtr->MaterialSlot = slot;

					//auto ParseTextures = [&](aiTextureType InType)
					//{
					//	if (MaterialAssimp->GetTextureCount(InType) > 0)
					//	{

					//		aiString AssimpTexPath;
					//		MaterialAssimp->GetTexture(InType, 0, &AssimpTexPath);

					//		BinSubMesh.TextureDescs.push_back({ AssimpTexPath.C_Str(), AssimpTextureConvert(InType) });
					//	}
					//};

					//ParseTextures(aiTextureType_DIFFUSE);
					//ParseTextures(aiTextureType_NORMALS);
					//ParseTextures(aiTextureType_METALNESS);
					//ParseTextures(aiTextureType_DIFFUSE_ROUGHNESS);
				}

				// vertices
				uint8* VertexPosition = OutMesh->mCPUVertexBuffers[0].data();

				for (uint32 NumVertex = 0; NumVertex < MeshAssimp->mNumVertices; ++NumVertex)
				{
					if (MeshAssimp->HasPositions())
					{
						memcpy(VertexPosition, &MeshAssimp->mVertices[NumVertex], sizeof(float) * 3);
						VertexPosition += (sizeof(float) * 3);
					}

					if (MeshAssimp->HasNormals())
					{
						memcpy(VertexPosition, &MeshAssimp->mNormals[NumVertex], sizeof(float) * 3);
						VertexPosition += (sizeof(float) * 3);
					}

					if (MeshAssimp->HasTangentsAndBitangents())
					{
						memcpy(VertexPosition, &MeshAssimp->mTangents[NumVertex], sizeof(float) * 3);
						VertexPosition += (sizeof(float) * 3);
					}

					for (uint32 NumColor = 0; NumColor < 1; ++NumColor)
					{
						if (MeshAssimp->HasVertexColors(NumColor))
						{
							memcpy(VertexPosition, &MeshAssimp->mColors[NumVertex], sizeof(float) * 4);
							VertexPosition += (sizeof(float) * 4);
						}
					}

					if (MeshAssimp->HasTextureCoords(0))
					{
						memcpy(VertexPosition, &MeshAssimp->mTextureCoords[NumVertex][0], sizeof(float) * 2);
						VertexPosition += (sizeof(float) * 2);
					}
				}

				// indices
				for (uint32 FaceIndex = 0; FaceIndex < MeshAssimp->mNumFaces; ++FaceIndex)
				{
					const aiFace& face = MeshAssimp->mFaces[FaceIndex];
					OutMesh->mCPUIndexBuffer.push_back(face.mIndices[0] + BaseVertex);
					OutMesh->mCPUIndexBuffer.push_back(face.mIndices[1] + BaseVertex);
					OutMesh->mCPUIndexBuffer.push_back(face.mIndices[2] + BaseVertex);
				}

				BaseVertex += MeshAssimp->mNumVertices;
				BaseIndex += SubMeshPtr->IndexCount;
			}

			OutMesh->mMaterialSlots = materialSlots;
		}
	private:
		static bool LoadAndParse(const std::string& InPath)
		{
			vertices.clear();
			indices.clear();
			submeshes.clear();
			materialSlots.clear();
			SlotMap.clear();

			Assimp::Importer AIModelImporter;

			const aiScene* AIScene = AIModelImporter.ReadFile(
				InPath,
				aiProcess_Triangulate |
				aiProcess_GenNormals |
				aiProcess_CalcTangentSpace |
				aiProcess_JoinIdenticalVertices |
				aiProcess_ImproveCacheLocality
			);

			if (!AIScene || !AIScene->HasMeshes())
			{
				LOG_ERROR("Failed to load model :", InPath);
				return false;
			}

			uint32 BaseVertex = 0;
			uint32 BaseIndex = 0;

			for (uint32 NumMesh = 0; NumMesh < AIScene->mNumMeshes; ++NumMesh)
			{
				aiMesh* MeshAssimp = AIScene->mMeshes[NumMesh];

				SubMeshBinary BinSubMesh{};
				BinSubMesh.StartIndex = BaseIndex;
				BinSubMesh.IndexCount = MeshAssimp->mNumFaces * 3;

				if (MeshAssimp->mMaterialIndex >= 0)
				{
					aiMaterial* MaterialAssimp = AIScene->mMaterials[MeshAssimp->mMaterialIndex];
					uint32 slot = GetOrCreateSlot(MaterialAssimp);
					BinSubMesh.MaterialSlotIndex = slot;
					auto ParseTextures = [&](aiTextureType InType)
					{
						if (MaterialAssimp->GetTextureCount(InType) > 0)
						{

							aiString AssimpTexPath;
							MaterialAssimp->GetTexture(InType, 0, &AssimpTexPath);

							//BinSubMesh.TextureDescs.push_back({ AssimpTexPath.C_Str(), AssimpTextureConvert(InType) });
						}
					};

					ParseTextures(aiTextureType_DIFFUSE);
					ParseTextures(aiTextureType_NORMALS);
					ParseTextures(aiTextureType_METALNESS);
					ParseTextures(aiTextureType_DIFFUSE_ROUGHNESS);
				}

				// vertices
				for (uint32 NumVertex = 0; NumVertex < MeshAssimp->mNumVertices; ++NumVertex)
				{
					ImportVertex InterVertexData{};

					memcpy(InterVertexData.Position, &MeshAssimp->mVertices[NumVertex], sizeof(float) * 3);

					if (MeshAssimp->HasNormals())
						memcpy(InterVertexData.Normal, &MeshAssimp->mNormals[NumVertex], sizeof(float) * 3);

					if (MeshAssimp->HasTangentsAndBitangents())
					{
						InterVertexData.Tangent[0] = MeshAssimp->mTangents[NumVertex].x;
						InterVertexData.Tangent[1] = MeshAssimp->mTangents[NumVertex].y;
						InterVertexData.Tangent[2] = MeshAssimp->mTangents[NumVertex].z;
					}

					for (uint32 NumColor = 0; NumColor < 1; ++NumColor)
					{
						if (MeshAssimp->HasVertexColors(NumColor))
						{
							InterVertexData.Color[NumColor][0] = MeshAssimp->mColors[NumColor]->r;
							InterVertexData.Color[NumColor][1] = MeshAssimp->mColors[NumColor]->g;
							InterVertexData.Color[NumColor][2] = MeshAssimp->mColors[NumColor]->b;
							InterVertexData.Color[NumColor][3] = MeshAssimp->mColors[NumColor]->a;
						}
					}

					if (MeshAssimp->HasTextureCoords(0))
					{
						InterVertexData.UV[0] = MeshAssimp->mTextureCoords[0][NumVertex].x;
						InterVertexData.UV[1] = MeshAssimp->mTextureCoords[0][NumVertex].y;
					}

					vertices.push_back(InterVertexData);
				}

				// indices
				for (uint32 FaceIndex = 0; FaceIndex < MeshAssimp->mNumFaces; ++FaceIndex)
				{
					const aiFace& face = MeshAssimp->mFaces[FaceIndex];
					indices.push_back(face.mIndices[0] + BaseVertex);
					indices.push_back(face.mIndices[1] + BaseVertex);
					indices.push_back(face.mIndices[2] + BaseVertex);
				}

				BaseVertex += MeshAssimp->mNumVertices;
				BaseIndex += BinSubMesh.IndexCount;

				submeshes.push_back(BinSubMesh);
			}
			return true;
		}

		static void ExportToFile(const std::string& InPath)
		{
			CheckAndCreateFile(Root::instance()->GetResourcePath() + InPath);
			DataStream MeshDataStream(Root::instance()->GetResourcePath() + InPath, DataStream::Out | DataStream::Binary);
			MeshFileHeader FileHeader{};
			FileHeader.Magic = 'EMSH';
			FileHeader.Version = 1;
			FileHeader.VertexBufferCount = 1;
			FileHeader.SubMeshCount = (uint32)submeshes.size();
			FileHeader.IndexCount = (uint32)indices.size();
			FileHeader.IndexStride = 4;
			FileHeader.MaterialSlotCount = (uint32)materialSlots.size();

			VertexBufferLayoutBinary VBLayout{};
			VBLayout.Stride = sizeof(ImportVertex);
			VBLayout.ElementCount = 5;

			VertexElementBinary elems[] =
			{
				{ (uint8)VertexSemantic::POSITION, 0, VET_Float3, 0 },
				{ (uint8)VertexSemantic::NORMAL,   0, VET_Float3, 12 },
				{ (uint8)VertexSemantic::TANGENT,  0, VET_Float3, 24 },
				{ (uint8)VertexSemantic::COLOR,  0, VET_Float4, 36 },
				{ (uint8)VertexSemantic::TEXCOORD, 0, VET_Float2, 52 }
			};

			MeshDataStream.Write((char*)&FileHeader, sizeof(FileHeader));

			for (uint32 i = 0; i < materialSlots.size(); ++i)
			{
				uint32 nameSize = materialSlots[i].SlotName.size();
				MeshDataStream.Write((char*)(&(materialSlots[i].SlotHash)), sizeof(MaterialSlot::SlotHash));
				MeshDataStream.Write((char*)(&(nameSize)), sizeof(uint32));
				MeshDataStream.Write(materialSlots[i].SlotName.c_str(), nameSize);
			}

			// Vertex layout
			MeshDataStream.Write((char*)&VBLayout, sizeof(VBLayout));
			MeshDataStream.Write((char*)elems, sizeof(elems));

			// SubMeshes
			for (uint32 i = 0; i < FileHeader.SubMeshCount; ++i)
			{
				MeshDataStream.Write((char*)(&(submeshes[i])), sizeof(SubMeshBinary));
			}

			// Vertex buffer
			uint32 vbSize = vertices.size() * sizeof(ImportVertex);
			MeshDataStream.Write((char*)&vbSize, sizeof(uint32));
			MeshDataStream.Write((char*)vertices.data(), vbSize);

			// Index buffer
			uint32 ibSize = indices.size() * sizeof(uint32);
			MeshDataStream.Write((char*)&ibSize, sizeof(uint32));
			MeshDataStream.Write((char*)indices.data(), ibSize);
		}

		static uint32 GetOrCreateSlot(aiMaterial* mat)
		{
			aiString name;
			mat->Get(AI_MATKEY_NAME, name);

			Name slotName(name.C_Str());
			uint64 hash = slotName.GetHashValue();

			auto it = SlotMap.find(hash);
			if (it != SlotMap.end())
				return it->second;

			uint32 index = (uint32)materialSlots.size();
			materialSlots.push_back({ hash, name.C_Str() });
			SlotMap[hash] = index;
			return index;
		}
	private:
		static std::vector<ImportVertex> vertices;
		static std::vector<uint32> indices;
		static std::vector<SubMeshBinary> submeshes;
		static std::vector<MaterialSlot> materialSlots;
		static std::unordered_map<uint64, uint32> SlotMap;
	};

	// Static member definitions for MeshConverter
	std::vector<ImportVertex> MeshConverter::vertices;
	std::vector<uint32> MeshConverter::indices;
	std::vector<SubMeshBinary> MeshConverter::submeshes;
	std::vector<MaterialSlot> MeshConverter::materialSlots;
	std::unordered_map<uint64, uint32> MeshConverter::SlotMap;


#endif

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
        DataStream File(Root::instance()->GetResourcePath() + mResourceName, DataStream::In | DataStream::Binary);
        File.ReadAll();
        if (!File.GetDataStream() || File.GetSize() <= 1) return {};
        const auto* Bytes = reinterpret_cast<const uint8_t*>(File.GetDataStream());
        // DataStream includes its extra null terminator in GetSize().
        const size_t ByteCount = File.GetSize() - 1;
        size_t Offset = 0;
        auto Read = [&](void* Dest, size_t Size) {
            if (Size > ByteCount - Offset) throw std::runtime_error("Truncated mesh data");
            memcpy(Dest, Bytes + Offset, Size); Offset += Size;
        };
        MeshFileHeader Header{}; Read(&Header, sizeof(Header));
        if (Header.Magic != 'EMSH' || Header.Version != 1 || (Header.IndexStride != 2 && Header.IndexStride != 4)) return {};
        if (!Header.VertexBufferCount || !Header.IndexCount || Header.VertexBufferCount > 16 || Header.VertexBufferCount > ByteCount / sizeof(VertexBufferLayoutBinary) || Header.SubMeshCount > ByteCount / sizeof(SubMeshBinary) || Header.MaterialSlotCount > ByteCount / 12) return {};
        for (uint32 i = 0; i < Header.MaterialSlotCount; ++i)
        {
            MaterialSlot Slot{}; uint32 Size = 0;
            Read(&Slot.SlotHash, sizeof(Slot.SlotHash)); Read(&Size, sizeof(Size));
            if (Size > ByteCount - Offset) return {};
            Slot.SlotName.resize(Size); Read(Slot.SlotName.data(), Size);
            Data.mMaterialSlots.push_back(std::move(Slot));
        }
        for (uint32 i = 0; i < Header.VertexBufferCount; ++i)
        {
            VertexBufferLayoutBinary Layout{}; Read(&Layout, sizeof(Layout));
            if (!Layout.Stride || !Layout.ElementCount || Layout.ElementCount > (ByteCount - Offset) / sizeof(VertexElementBinary)) return {};
            VertexBufferDesc Desc{}; Desc.Stride = Layout.Stride; Desc.Rate = PerVertex;
            for (uint32 e = 0; e < Layout.ElementCount; ++e)
            {
                VertexElementBinary Element{}; Read(&Element, sizeof(Element));
                if (Element.Type == VET_None || Element.Type >= VET_MAX || Element.Offset > Layout.Stride || GetRHIFormatSize(static_cast<VertexElementType>(Element.Type)) > Layout.Stride - Element.Offset) return {};
                Desc.Attributes.push_back({static_cast<VertexSemantic>(Element.Semantic), Element.Index, static_cast<VertexElementType>(Element.Type), Element.Offset});
            }
            Data.mVertexLayout.Buffers.push_back(std::move(Desc));
        }
        for (uint32 i = 0; i < Header.SubMeshCount; ++i)
        {
            SubMeshBinary Source{}; Read(&Source, sizeof(Source));
            if (Source.MaterialSlotIndex >= Header.MaterialSlotCount || Source.StartIndex > Header.IndexCount || Source.IndexCount > Header.IndexCount - Source.StartIndex) return {};
            auto Sub = std::make_unique<SubMesh>(); Sub->StartIndex = Source.StartIndex; Sub->IndexCount = Source.IndexCount; Sub->MaterialSlot = Source.MaterialSlotIndex;
            Data.mSubMeshes.push_back(Sub.get());
            Sub.release();
        }
        Data.mCPUVertexBuffers.resize(Header.VertexBufferCount);
        for (uint32 i = 0; i < Header.VertexBufferCount; ++i)
        {
            uint32 Size = 0; Read(&Size, sizeof(Size));
            if (!Size || Size > ByteCount - Offset || Size % Data.mVertexLayout.Buffers[i].Stride || (i && Size / Data.mVertexLayout.Buffers[i].Stride != Data.mCPUVertexBuffers[0].size() / Data.mVertexLayout.Buffers[0].Stride)) return {};
            Data.mCPUVertexBuffers[i].resize(Size); Read(Data.mCPUVertexBuffers[i].data(), Size);
        }
        uint32 IndexBytes = 0; Read(&IndexBytes, sizeof(IndexBytes));
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
        Data.MemoryUsage = ByteCount;
        return Pending;
    }

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
        mVertexLayout.Buffers.clear(); mMaterialSlots.clear();
    }

    void Mesh::SaveResourceImpl()
    {
        if (mVertexLayout.Buffers.size() != mCPUVertexBuffers.size() || mVertexLayout.Buffers.empty()) return;
        DataStream File(Root::instance()->GetResourcePath() + mResourceName, DataStream::Out | DataStream::Binary);
        auto Write = [&](const void* Data, size_t Size) { File.Write(static_cast<const char*>(Data), Size); };
        MeshFileHeader Header{}; Header.Magic = 'EMSH'; Header.Version = 1;
        Header.VertexBufferCount = static_cast<uint32>(mCPUVertexBuffers.size());
        Header.SubMeshCount = static_cast<uint32>(mSubMeshes.size()); Header.MaterialSlotCount = static_cast<uint32>(mMaterialSlots.size());
        Header.IndexCount = static_cast<uint32>(mCPUIndexBuffer.size()); Header.IndexStride = mIndexBytes.empty() ? 4 : mIndexStride;
        Write(&Header, sizeof(Header));
        for (const auto& Slot : mMaterialSlots)
        {
            uint32 Size = static_cast<uint32>(Slot.SlotName.size());
            Write(&Slot.SlotHash, sizeof(Slot.SlotHash)); Write(&Size, sizeof(Size)); Write(Slot.SlotName.data(), Size);
        }
        for (const auto& Buffer : mVertexLayout.Buffers)
        {
            VertexBufferLayoutBinary Layout{}; Layout.Stride = Buffer.Stride; Layout.ElementCount = static_cast<uint32>(Buffer.Attributes.size());
            Write(&Layout, sizeof(Layout));
            for (const auto& Attr : Buffer.Attributes)
            {
                VertexElementBinary Element{}; Element.Semantic = Attr.Semantic; Element.Index = Attr.Index; Element.Type = Attr.Format; Element.Offset = Attr.Offset;
                Write(&Element, sizeof(Element));
            }
        }
        for (const auto* Sub : mSubMeshes)
        {
            SubMeshBinary Binary{}; Binary.StartIndex = Sub->StartIndex; Binary.IndexCount = Sub->IndexCount; Binary.MaterialSlotIndex = Sub->MaterialSlot;
            Write(&Binary, sizeof(Binary));
        }
        for (const auto& Buffer : mCPUVertexBuffers)
        { uint32 Size = static_cast<uint32>(Buffer.size()); Write(&Size, sizeof(Size)); Write(Buffer.data(), Size); }
        uint32 Size = Header.IndexCount * Header.IndexStride; Write(&Size, sizeof(Size));
        Write(mIndexBytes.empty() ? static_cast<const void*>(mCPUIndexBuffer.data()) : mIndexBytes.data(), Size);
    }



#ifdef _HAS_EDITOR_
	void ImportMeshFromFile(const std::string& srcPath, const std::string& destRelPath)
	{
		MeshConverter::LoadAndParse(srcPath);
		MeshConverter::ExportToFile(destRelPath);
		LOG_INFO("Mesh imported successfully: ", destRelPath);
	}
#endif

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
        std::swap(mIndexBuffer, Data.mIndexBuffer);
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
