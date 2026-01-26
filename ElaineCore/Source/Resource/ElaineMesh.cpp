#include "ElainePrecompiledHeader.h"
#include "ElaineMesh.h"
#include "ElaineDataStream.h"
#include "ElaineUniformGPUManager.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "ElaineMeshManager.h"
#include "ElaineMaterialInstanceStatic.h"

namespace Elaine
{
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

	class MeshConverter
	{
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
		}

		static void ExportToFile(const std::string& InPath)
		{
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

	uint32 Mesh::GetSubMeshCount() const
	{
		return mSubMeshes.size();
	}

	bool Mesh::LoadImpl()
	{
		DataStream MeshDataStream(Root::instance()->GetResourcePath() + mResourceName);

		const uint8* DataBegin = (const uint8*)MeshDataStream.GetDataStream();
		const uint8* DataEnd = DataBegin + MeshDataStream.GetSize();

		// =============================
		// 1. Header
		// =============================
		MeshFileHeader FileHeader;
		MeshDataStream.Read(FileHeader);

		if (FileHeader.Magic != 'EMSH')
		{
			//ELAINE_LOG_ERROR("Invalid mesh magic");
			return false;
		}

		if (FileHeader.Version != 1)
		{
			//ELAINE_LOG_ERROR("Unsupported mesh version");
			return false;
		}

		// =============================
		// 2. Material Slots
		// =============================
		for (uint32 i = 0; i < FileHeader.MaterialSlotCount; ++i)
		{
			MaterialSlot slot;
			MeshDataStream.Read(slot.SlotHash);
			uint32 slotSize = 0;
			MeshDataStream.Read(slotSize);
			slot.SlotName.resize(slotSize);
			MeshDataStream.Read(slot.SlotName.data(), slotSize);
		}

		// =============================
		// 3. Vertex Layout
		// =============================
		mVertexLayout.Buffers.clear();

		for (uint32 vb = 0; vb < FileHeader.VertexBufferCount; ++vb)
		{
			VertexBufferLayoutBinary VBLayout;
			MeshDataStream.Read(VBLayout);

			VertexBufferDesc desc{};
			desc.Stride = VBLayout.Stride;
			desc.Rate = VertexRate::PerVertex;

			for (uint32 e = 0; e < VBLayout.ElementCount; ++e)
			{
				VertexElementBinary elem;
				MeshDataStream.Read(elem);

				VertexAttributeDesc attr{};
				attr.Semantic = (VertexSemantic)elem.Semantic;
				attr.Index = elem.Index;
				attr.Format = (VertexElementType)elem.Type;
				attr.Offset = elem.Offset;

				desc.Attributes.push_back(attr);
			}

			mVertexLayout.Buffers.push_back(desc);
		}

		// =============================
		// 4. SubMeshes
		// =============================
		mSubMeshes.clear();
		mSubMeshes.reserve(FileHeader.SubMeshCount);

		for (uint32 i = 0; i < FileHeader.SubMeshCount; ++i)
		{
			SubMeshBinary sm;
			MeshDataStream.Read(sm);

			SubMesh* sub = new SubMesh();
			sub->StartIndex = sm.StartIndex;
			sub->IndexCount = sm.IndexCount;
			sub->MaterialSlot = sm.MaterialSlotIndex;
			//sub->MaterialInstance =
			//	MaterialManager::instance()->GetResource(
			//		Name(sm.MaterialNameHash)
			//	);

			mSubMeshes.push_back(sub);
		}

		// =============================
		// 5. Vertex Buffers
		// =============================
		mCPUVertexBuffers.clear();
		mGPUVertexBuffers.clear();
		mCPUVertexBuffers.resize(FileHeader.VertexBufferCount);
		for (uint32 vb = 0; vb < FileHeader.VertexBufferCount; ++vb)
		{
			uint32 vbSize;
			MeshDataStream.Read(vbSize);
			void* vbData = nullptr;
			MeshDataStream.Read(vbData, vbSize);
			mCPUVertexBuffers[vb].resize(vbSize);
			Memory::MemoryCopy(mCPUVertexBuffers[vb].data(), vbData, vbSize);

			RHIBuffer* VertexBuf = RenderSystem::instance()->CreateBuffer(BufferUsageFlags::VertexBuffer, ERHIAccess::VertexOrIndexBuffer, vbData, vbSize);
			mGPUVertexBuffers.push_back(VertexBuf);

			DataBegin += vbSize;
		}

		// =============================
		// 6. Index Buffer
		// =============================
		uint32 ibSize;
		MeshDataStream.Read(ibSize);
		mCPUIndexBuffer.resize(ibSize);
		void* ibData = nullptr;
		MeshDataStream.Read(ibData, ibSize);
		Memory::MemoryCopy(mCPUIndexBuffer.data(), ibData, ibSize);

		mIndexBuffer = RenderSystem::instance()->CreateBuffer(BufferUsageFlags::IndexBuffer, ERHIAccess::VertexOrIndexBuffer, ibData, ibSize);
		DataBegin += ibSize;

		// =============================
		// 7. Sanity Check
		// =============================
		if (DataBegin != DataEnd)
		{
			LOG_WARN("Mesh binary not fully consumed.");
		}

		return true;
	}

	void Mesh::UnloadImpl()
	{

	}

	void Mesh::SaveResourceImpl()
	{
		if (mVertexLayout.Buffers.size() <= 0)
			return;

		DataStream MeshDataStream(Root::instance()->GetResourcePath() + mResourceName, DataStream::Out | DataStream::Binary);
		MeshFileHeader FileHeader{};
		FileHeader.Magic = 'EMSH';
		FileHeader.Version = 1;
		FileHeader.VertexBufferCount = 1;
		FileHeader.SubMeshCount = (uint32)mSubMeshes.size();
		FileHeader.IndexCount = (uint32)mCPUIndexBuffer.size();
		FileHeader.IndexStride = 4;
		FileHeader.MaterialSlotCount = (uint32)mMaterialSlots.size();

		VertexBufferLayoutBinary VBLayout{};
		std::vector<VertexElementBinary> VexElems;
		uint32 ElemOffset = 0;
		for (auto&& Att : mVertexLayout.Buffers[0].Attributes)
		{
			VBLayout.Stride += GetRHIFormatSize(Att.Format);
			VertexElementBinary VexElem{};
			VexElem.Index = 0;
			VexElem.Semantic = Att.Semantic;
			VexElem.Offset = ElemOffset;
			VexElem.Type = Att.Format;
			ElemOffset += GetRHIFormatSize(Att.Format);
		}
		VBLayout.ElementCount = mVertexLayout.Buffers[0].Attributes.size();

		// File Header
		MeshDataStream.Write((char*)&FileHeader, sizeof(FileHeader));

		for (uint32 i = 0; i < mMaterialSlots.size(); ++i)
		{
			uint32 NameSize = mMaterialSlots[i].SlotName.size();
			MeshDataStream.Write((char*)(&(mMaterialSlots[i].SlotHash)), sizeof(MaterialSlot::SlotHash));
			MeshDataStream.Write((char*)(&(NameSize)), sizeof(uint32));
			MeshDataStream.Write(mMaterialSlots[i].SlotName.c_str(), NameSize);
		}

		// Vertex layout
		MeshDataStream.Write((char*)&VBLayout, sizeof(VBLayout));
		MeshDataStream.Write((char*)VexElems.data(), sizeof(VertexElementBinary) * VexElems.size());

		// SubMeshes
		for (uint32 i = 0; i < FileHeader.SubMeshCount; ++i)
		{
			SubMeshBinary SubMeshBin{};
			SubMeshBin.StartIndex = mSubMeshes[i]->StartIndex;
			SubMeshBin.IndexCount = mSubMeshes[i]->IndexCount;
			SubMeshBin.MaterialSlotIndex = mSubMeshes[i]->MaterialSlot;
			MeshDataStream.Write((char*)(&(SubMeshBin)), sizeof(SubMeshBinary));
		}

		// Vertex buffer
		uint32 vbSize = mCPUVertexBuffers[0].size();
		MeshDataStream.Write((char*)&vbSize, sizeof(uint32));
		MeshDataStream.Write((char*)mCPUVertexBuffers[0].data(), vbSize);

		// Index buffer
		uint32 ibSize = mCPUIndexBuffer.size() * sizeof(uint32);
		MeshDataStream.Write((char*)&ibSize, sizeof(uint32));
		MeshDataStream.Write((char*)mCPUIndexBuffer.data(), ibSize);
	}

	void Mesh::ResourceArrivedImpl()
	{

	}
}