#include "ElainePrecompiledHeader.h"
#include "ElaineMeshImporter.h"
#include "ElaineTextureImporter.h"
#include "Resource/ElaineMesh.h"
#include "Resource/ElaineMeshAsset.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/GltfMaterial.h"
#include <bit>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace Elaine
{
    namespace fs = std::filesystem;
    namespace
    {
        void UInt(std::vector<uint8_t>& Out, uint64_t Value, size_t Width = 4)
        { for (size_t I = 0; I < Width; ++I) Out.push_back(static_cast<uint8_t>(Value >> (I * 8))); }
        std::vector<uint8_t> JsonBytes(const JsonCpp& Json) { auto Text = Json.dump(); return {Text.begin(), Text.end()}; }
        std::string SafeName(std::string Name)
        {
            for (auto& C : Name) if (!std::isalnum(static_cast<unsigned char>(C)) && C != '-' && C != '_') C = '_';
            if (Name.empty()) Name = "Resource";
            return Name.substr(0, 80);
        }
        std::string Filename(const std::string& NameText, const std::string& Key)
        { std::ostringstream Text; Text << SafeName(NameText) << '_' << std::hex << Name(Key.c_str()).GetHashValue() << ".asset"; return Text.str(); }
        std::vector<uint8_t> ReadFile(const fs::path& File)
        {
            std::ifstream Stream(File, std::ios::binary | std::ios::ate);
            if (!Stream) throw std::runtime_error("Missing model texture: " + File.string());
            auto Size = Stream.tellg(); if (Size <= 0 || uint64_t(Size) > INT_MAX) throw std::runtime_error("Invalid model texture size: " + File.string());
            std::vector<uint8_t> Bytes(static_cast<size_t>(Size)); Stream.seekg(0);
            if (!Stream.read(reinterpret_cast<char*>(Bytes.data()), Size)) throw std::runtime_error("Truncated model texture: " + File.string());
            return Bytes;
        }
        struct TextureSource { std::string Key, Name; std::vector<uint8_t> Bytes; };
        TextureSource ReadTexture(const aiScene& Scene, const fs::path& Model, const std::string& NameText)
        {
            if (const auto* Embedded = Scene.GetEmbeddedTexture(NameText.c_str()))
            {
                TextureSource Source; Source.Key = "embedded:" + NameText; Source.Name = Embedded->mFilename.C_Str();
                if (Source.Name.empty()) Source.Name = SafeName(NameText) + (Embedded->mHeight ? ".png" : "." + std::string(Embedded->achFormatHint));
                if (!Embedded->mHeight)
                {
                    const auto* Begin = reinterpret_cast<const uint8_t*>(Embedded->pcData);
                    Source.Bytes.assign(Begin, Begin + Embedded->mWidth);
                }
                else
                {
                    if (uint64_t(Embedded->mWidth) * Embedded->mHeight > UINT32_MAX / 4) throw std::runtime_error("Embedded model texture too large");
                    std::vector<uint8_t> Pixels(size_t(Embedded->mWidth) * Embedded->mHeight * 4);
                    for (size_t I = 0; I < Pixels.size() / 4; ++I)
                    { const auto& P = Embedded->pcData[I]; Pixels[I*4]=P.r; Pixels[I*4+1]=P.g; Pixels[I*4+2]=P.b; Pixels[I*4+3]=P.a; }
                    Source.Bytes = TextureImporter::EncodePNG(Embedded->mWidth, Embedded->mHeight, Pixels.data());
                }
                return Source;
            }
            auto Normalized = NameText; std::replace(Normalized.begin(), Normalized.end(), '\\', '/');
            auto File = fs::weakly_canonical(Model.parent_path() / fs::path(Normalized));
            return {"external:" + File.lexically_relative(Model.parent_path()).generic_string(), File.filename().string(), ReadFile(File)};
        }
        std::string TextureName(const aiMaterial& Material, std::initializer_list<aiTextureType> Types)
        {
            for (auto Type : Types) if (Material.GetTextureCount(Type))
            { aiString Path; if (Material.GetTexture(Type, 0, &Path) == AI_SUCCESS) return Path.C_Str(); }
            return {};
        }
        float Sample(const ImportedImage& Image, float U, float V, int Channel)
        {
            float X = std::clamp(U * Image.Width - 0.5f, 0.0f, float(Image.Width - 1));
            float Y = std::clamp(V * Image.Height - 0.5f, 0.0f, float(Image.Height - 1));
            int X0 = static_cast<int>(X), Y0 = static_cast<int>(Y);
            auto At = [&](int PX, int PY) {
                size_t Index = (size_t(PY) * Image.Width + PX) * 4 + Channel;
                if (!Image.HDR) return Image.Pixels[Index] / 255.0f;
                float Value; memcpy(&Value, Image.Pixels.data() + Index * 4, 4);
                if (!std::isfinite(Value)) throw std::runtime_error("Non-finite material texture component"); return Value;
            };
            float A = std::lerp(At(X0, Y0), At(std::min(X0+1,Image.Width-1), Y0), X-X0);
            float B = std::lerp(At(X0,std::min(Y0+1,Image.Height-1)), At(std::min(X0+1,Image.Width-1),std::min(Y0+1,Image.Height-1)), X-X0);
            return std::lerp(A, B, Y-Y0);
        }
    }

    JsonCpp MeshImporter::GetSettingsDescription() const
    { return {{"scale", {{"type", "float"}, {"default", 1.0}}}, {"material", {{"type", "Material"}, {"default", "material/ImportedPBR.asset"}}}}; }
    std::shared_ptr<const ResourceImportSettings> MeshImporter::ReadSettings(const JsonCpp& Json) const
    { auto Settings = std::make_shared<MeshImportSettings>(); Settings->Scale = Json.value("scale", 1.0f); Settings->Material = Json.value("material", Settings->Material); return Settings; }

    ResourceImportOutput MeshImporter::Build(const ResourceImportContext& Context) const
    {
        const auto* Settings = dynamic_cast<const MeshImportSettings*>(Context.Request.Settings.get());
        if (!Settings || !std::isfinite(Settings->Scale) || Settings->Scale <= 0) throw std::runtime_error("Model scale must be finite and positive");
        if (Context.Request.Sources.size() != 1) throw std::runtime_error("Mesh import requires one model source");
        const auto Model = Context.Request.Sources.front();
        Assimp::Importer Parser;
        const auto* Scene = Parser.ReadFile(Model.string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality | aiProcess_ValidateDataStructure);
        if (!Scene || !Scene->mRootNode || !Scene->HasMeshes()) throw std::runtime_error("Model import failed: " + std::string(Parser.GetErrorString()));
        ResourceImportOutput Output; Output.Assets.push_back(Context.MakeAsset("primary", "Mesh", Context.Request.Destination));
        if (Scene->HasAnimations()) Output.Warnings.push_back("Animations are not imported; only the static model pose is used");
        auto Parent = fs::path(Context.Request.Destination); Parent.replace_extension();
        const auto MaterialFile = Context.Root / CanonicalResourcePath(Settings->Material);
        const auto Master = AssetFile::ReadDescriptor(MaterialFile);
        if (Master.Type != "Material") throw std::runtime_error("Model import requires a PBR mother material");
        const auto MotherData = AssetFile::ReadJson(MaterialFile, "Material");
        for (const auto* Parameter : {"BaseColor", "Normal", "MetallicRoughness", "Emissive", "AO", "BaseColorFactor", "EmissiveFactor", "Metallic", "Roughness", "Occlusion", "AlphaCutoff"})
            if (!MotherData.at("parameters").contains(Parameter)) throw std::runtime_error("PBR mother material is missing parameter: " + std::string(Parameter));
        const ResourceReference MasterRef{Master.Id, CanonicalResourcePath(Settings->Material)};
        std::map<std::string, ResourceReference> Textures;
        auto ImportTexture = [&](const TextureSource& Source, const std::string& Usage, bool SRGB) {
            const auto Key = "texture:" + Source.Key + ":" + Usage + (SRGB ? ":srgb" : ":linear");
            if (auto It = Textures.find(Key); It != Textures.end()) return It->second;
            auto Asset = Context.MakeAsset(Key, "Texture", (Parent / "Textures" / Filename(fs::path(Source.Name).stem().string(), Key)).generic_string());
            Asset = TextureImporter::BuildTexture(std::move(Asset), {"2d", Usage, SRGB}, {{Source.Name, Source.Bytes}});
            const ResourceReference Reference{Asset.Descriptor.Id, Asset.Descriptor.Path};
            Output.Assets.push_back(std::move(Asset)); Textures[Key] = Reference; return Reference;
        };
        std::map<unsigned, ResourceReference> Materials;
        auto ImportMaterial = [&](unsigned Index) {
            if (auto It = Materials.find(Index); It != Materials.end()) return It->second;
            if (Index >= Scene->mNumMaterials) throw std::runtime_error("Invalid model material index");
            const auto& Material = *Scene->mMaterials[Index]; aiString NameText; Material.Get(AI_MATKEY_NAME, NameText);
            std::string SourceName = NameText.C_Str();
            // Source table identity is independent of output order and the editable material label.
            auto Key = "material:source-index:" + std::to_string(Index);
            auto Asset = Context.MakeAsset(Key, "MaterialInstance", (Parent / "Materials" / Filename(SourceName, Key)).generic_string());
            JsonCpp Parameters = JsonCpp::object(), TextureRefs = JsonCpp::object();
            aiColor4D Base(1,1,1,1); if (Material.Get(AI_MATKEY_BASE_COLOR, Base) != AI_SUCCESS) Material.Get(AI_MATKEY_COLOR_DIFFUSE, Base);
            aiColor3D Emissive(0,0,0); Material.Get(AI_MATKEY_COLOR_EMISSIVE, Emissive);
            float Metallic = 0, Roughness = 0.5f, Cutoff = 0.5f, Opacity = 1.0f;
            const bool HasMetallicFactor = Material.Get(AI_MATKEY_METALLIC_FACTOR, Metallic) == AI_SUCCESS;
            const bool HasRoughnessFactor = Material.Get(AI_MATKEY_ROUGHNESS_FACTOR, Roughness) == AI_SUCCESS;
            Material.Get(AI_MATKEY_OPACITY, Opacity);
            aiString AlphaMode; Material.Get(AI_MATKEY_GLTF_ALPHAMODE, AlphaMode); Material.Get(AI_MATKEY_GLTF_ALPHACUTOFF, Cutoff);
            for (float Value : {Base.r,Base.g,Base.b,Base.a,Emissive.r,Emissive.g,Emissive.b,Metallic,Roughness,Cutoff,Opacity})
                if (!std::isfinite(Value)) throw std::runtime_error("Non-finite material parameter: " + SourceName);
            int Shading = aiShadingMode_PBR_BRDF;
            Material.Get(AI_MATKEY_SHADING_MODEL, Shading);
            if (Shading != aiShadingMode_PBR_BRDF && Shading != aiShadingMode_Phong && Shading != aiShadingMode_Blinn && Shading != aiShadingMode_Gouraud && Shading != aiShadingMode_Flat)
                Output.Warnings.push_back("Source shading model is approximated with PBR: " + SourceName);
            if (std::string(AlphaMode.C_Str()) == "BLEND" || Opacity < 1.0f) Output.Warnings.push_back("Blended transparency is unsupported; importing opaque material: " + SourceName);
            Parameters["BaseColorFactor"] = {Base.r,Base.g,Base.b,Base.a}; Parameters["EmissiveFactor"] = {Emissive.r,Emissive.g,Emissive.b,1.0f};
            Parameters["Metallic"] = Metallic; Parameters["Roughness"] = Roughness; Parameters["Occlusion"] = 1.0f;
            Parameters["AlphaCutoff"] = std::string(AlphaMode.C_Str()) == "MASK" ? Cutoff : 0.0f;
            auto Map = [&](const char* Parameter, std::initializer_list<aiTextureType> Types, const std::string& Usage, bool SRGB) {
                auto Path = TextureName(Material, Types);
                if (!Path.empty()) TextureRefs[Parameter] = ImportTexture(ReadTexture(*Scene, Model, Path), Usage, SRGB).ToJson();
            };
            Map("BaseColor", {aiTextureType_BASE_COLOR,aiTextureType_DIFFUSE}, "color", true);
            Map("Normal", {aiTextureType_NORMALS,aiTextureType_NORMAL_CAMERA}, "normal", false);
            Map("Emissive", {aiTextureType_EMISSION_COLOR,aiTextureType_EMISSIVE}, "color", true);
            Map("AO", {aiTextureType_AMBIENT_OCCLUSION,aiTextureType_LIGHTMAP}, "data", false);
            auto MetalPath = TextureName(Material, {aiTextureType_METALNESS});
            auto RoughPath = TextureName(Material, {aiTextureType_DIFFUSE_ROUGHNESS});
            if (!MetalPath.empty() && !HasMetallicFactor) Parameters["Metallic"] = 1.0f;
            if (!RoughPath.empty() && !HasRoughnessFactor) Parameters["Roughness"] = 1.0f;
            aiString Packed; Material.GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &Packed);
            if (Packed.length) TextureRefs["MetallicRoughness"] = ImportTexture(ReadTexture(*Scene, Model, Packed.C_Str()), "data", false).ToJson();
            else if (!MetalPath.empty() || !RoughPath.empty())
            {
                TextureSource M, R; ImportedImage MI, RI;
                if (!MetalPath.empty()) { M = ReadTexture(*Scene,Model,MetalPath); MI = TextureImporter::Decode(M.Bytes); }
                if (!RoughPath.empty()) { R = ReadTexture(*Scene,Model,RoughPath); RI = TextureImporter::Decode(R.Bytes); }
                int Width = std::max(MI.Width, RI.Width), Height = std::max(MI.Height, RI.Height);
                if (uint64_t(Width)*Height > UINT32_MAX/4) throw std::runtime_error("Packed material texture is too large");
                std::vector<uint8_t> Pixels(size_t(Width)*Height*4,255);
                for (int Y=0;Y<Height;++Y) for (int X=0;X<Width;++X)
                {
                    float U=(X+0.5f)/Width,V=(Y+0.5f)/Height; auto Offset=(size_t(Y)*Width+X)*4;
                    if (RI.Width) Pixels[Offset+1]=static_cast<uint8_t>(std::clamp(Sample(RI,U,V,0),0.0f,1.0f)*255.0f+0.5f);
                    if (MI.Width) Pixels[Offset+2]=static_cast<uint8_t>(std::clamp(Sample(MI,U,V,0),0.0f,1.0f)*255.0f+0.5f);
                }
                auto Ref = ImportTexture({"packed:"+M.Key+"|"+R.Key,"MetallicRoughness.png",TextureImporter::EncodePNG(Width,Height,Pixels.data())},"data",false);
                TextureRefs["MetallicRoughness"] = Ref.ToJson();
                auto& Converted = *std::find_if(Output.Assets.begin(), Output.Assets.end(),
                    [&](const auto& OutputAsset) { return OutputAsset.Descriptor.Id == Ref.Id; });
                if (!M.Bytes.empty()) Converted.Sections["metallicSource"] = M.Bytes;
                if (!R.Bytes.empty()) Converted.Sections["roughnessSource"] = R.Bytes;
            }
            for (unsigned Type=aiTextureType_NONE+1;Type<=AI_TEXTURE_TYPE_MAX;++Type)
                if (Material.GetTextureCount(static_cast<aiTextureType>(Type)) &&
                    Type!=aiTextureType_BASE_COLOR && Type!=aiTextureType_DIFFUSE && Type!=aiTextureType_NORMALS && Type!=aiTextureType_NORMAL_CAMERA &&
                    Type!=aiTextureType_EMISSION_COLOR && Type!=aiTextureType_EMISSIVE && Type!=aiTextureType_AMBIENT_OCCLUSION && Type!=aiTextureType_LIGHTMAP &&
                    Type!=aiTextureType_METALNESS && Type!=aiTextureType_DIFFUSE_ROUGHNESS && Type!=aiTextureType_UNKNOWN)
                    Output.Warnings.push_back("Unsupported source texture semantic " + std::to_string(Type) + " in " + SourceName);
            Asset.Data = {{"version","2.0.0"},{"origin",MasterRef.ToJson()},{"parameters",Parameters},{"textures",TextureRefs}};
            Asset.Descriptor.Tags = {{"Overrides",Parameters.size()}};
            auto Ref = ResourceReference{Asset.Descriptor.Id,Asset.Descriptor.Path}; Output.Assets.push_back(std::move(Asset)); Materials[Index]=Ref; return Ref;
        };

        struct Vertex { float Position[3], Normal[3], Tangent[4], UV[2]; };
        std::vector<Vertex> Vertices; std::vector<uint32_t> Indices;
        struct Part { uint32_t Start, Count, Slot; }; std::vector<Part> Parts;
        std::vector<MaterialSlot> Slots; std::map<unsigned,uint32_t> SlotMap;
        std::function<void(const aiNode&,const aiMatrix4x4&)> Visit = [&](const aiNode& Node,const aiMatrix4x4& ParentMatrix) {
            auto Matrix = ParentMatrix * Node.mTransformation; aiMatrix3x3 Linear(Matrix), Normal(Linear);
            float Determinant=Linear.Determinant();
            if (!std::isfinite(Determinant) || std::abs(Determinant)<1e-12f) throw std::runtime_error("Singular model node transform: "+std::string(Node.mName.C_Str()));
            Normal.Inverse().Transpose();
            for (unsigned I=0;I<Node.mNumMeshes;++I)
            {
                if (Node.mMeshes[I]>=Scene->mNumMeshes) throw std::runtime_error("Invalid model node mesh");
                const auto& Mesh=*Scene->mMeshes[Node.mMeshes[I]];
                if (Mesh.HasBones()) Output.Warnings.push_back("Skinning is not imported; retaining static vertices");
                if (Mesh.HasTextureCoords(1)) Output.Warnings.push_back("Only UV0 is imported; additional UV channels are omitted");
                if (Mesh.HasVertexColors(0)) Output.Warnings.push_back("Vertex colors are not used by the imported PBR material");
                if (uint64_t(Vertices.size())+Mesh.mNumVertices>UINT32_MAX/sizeof(Vertex)) throw std::runtime_error("Imported Mesh too large");
                auto Base=static_cast<uint32_t>(Vertices.size());
                for (unsigned V=0;V<Mesh.mNumVertices;++V)
                {
                    auto P=(Matrix*Mesh.mVertices[V])*Settings->Scale;
                    auto N=Normal*(Mesh.HasNormals()?Mesh.mNormals[V]:aiVector3D(0,1,0)); N.Normalize();
                    auto T=Linear*(Mesh.HasTangentsAndBitangents()?Mesh.mTangents[V]:aiVector3D(1,0,0));
                    T-=N*(T*N); if(T.SquareLength()<1e-12f) T=(std::abs(N.y)<0.9f?aiVector3D(0,1,0):aiVector3D(1,0,0))^N; T.Normalize();
                    auto B=Mesh.HasTangentsAndBitangents()?Linear*Mesh.mBitangents[V]:(N^T);
                    auto UV=Mesh.HasTextureCoords(0)?Mesh.mTextureCoords[0][V]:aiVector3D();
                    Vertex Out{{P.x,P.y,P.z},{N.x,N.y,N.z},{T.x,T.y,T.z,((N^T)*B)<0?-1.0f:1.0f},{UV.x,UV.y}};
                    for(float F:{P.x,P.y,P.z,N.x,N.y,N.z,T.x,T.y,T.z,UV.x,UV.y}) if(!std::isfinite(F)) throw std::runtime_error("Model contains non-finite vertex data");
                    Vertices.push_back(Out);
                }
                uint32_t Slot;
                if (auto It=SlotMap.find(Mesh.mMaterialIndex);It!=SlotMap.end()) Slot=It->second;
                else
                {
                    auto Ref=ImportMaterial(Mesh.mMaterialIndex); aiString Label; Scene->mMaterials[Mesh.mMaterialIndex]->Get(AI_MATKEY_NAME,Label);
                    Slot=static_cast<uint32_t>(Slots.size()); SlotMap[Mesh.mMaterialIndex]=Slot;
                    Slots.push_back({Name(Label.C_Str()).GetHashValue(),Label.C_Str(),Ref});
                }
                Part PartData{static_cast<uint32_t>(Indices.size()),0,Slot};
                for(unsigned F=0;F<Mesh.mNumFaces;++F)
                {
                    const auto& Face=Mesh.mFaces[F];
                    if (Face.mNumIndices!=3) throw std::runtime_error("Only triangle model geometry is supported");
                    for(unsigned K=0;K<3;++K) if(Face.mIndices[K]>=Mesh.mNumVertices) throw std::runtime_error("Model index out of bounds");
                    Indices.push_back(Base+Face.mIndices[0]); Indices.push_back(Base+Face.mIndices[Determinant<0?2:1]); Indices.push_back(Base+Face.mIndices[Determinant<0?1:2]);
                }
                PartData.Count=static_cast<uint32_t>(Indices.size())-PartData.Start; if(PartData.Count) Parts.push_back(PartData);
            }
            for(unsigned I=0;I<Node.mNumChildren;++I) Visit(*Node.mChildren[I],Matrix);
        };
        Visit(*Scene->mRootNode,aiMatrix4x4());
        if(Vertices.empty()||Indices.empty()) throw std::runtime_error("Model contains no triangle geometry");
        auto& Asset=Output.Assets.front(); auto& Geometry=Asset.Sections["data"];
        uint32_t Stride=Vertices.size()<=65536?2:4;
        for(uint32_t V:{uint32_t('EMSH'),1u,1u,static_cast<uint32_t>(Parts.size()),static_cast<uint32_t>(Indices.size()),Stride,static_cast<uint32_t>(Slots.size())}) UInt(Geometry,V);
        for(const auto& Slot:Slots) { UInt(Geometry,Slot.SlotHash,8); UInt(Geometry,Slot.SlotName.size()); Geometry.insert(Geometry.end(),Slot.SlotName.begin(),Slot.SlotName.end()); }
        UInt(Geometry,sizeof(Vertex)); UInt(Geometry,4);
        for(auto Element:std::vector<std::array<uint8_t,4>>{{POSITION,0,VET_Float3,0},{NORMAL,0,VET_Float3,12},{TANGENT,0,VET_Float4,24},{TEXCOORD,0,VET_Float2,40}})
            for(auto V:Element) UInt(Geometry,V,1);
        for(const auto& PartData:Parts) { UInt(Geometry,PartData.Start); UInt(Geometry,PartData.Count); UInt(Geometry,PartData.Slot); }
        UInt(Geometry,Vertices.size()*sizeof(Vertex));
        for(const auto& V:Vertices) for(float F:{V.Position[0],V.Position[1],V.Position[2],V.Normal[0],V.Normal[1],V.Normal[2],V.Tangent[0],V.Tangent[1],V.Tangent[2],V.Tangent[3],V.UV[0],V.UV[1]}) UInt(Geometry,std::bit_cast<uint32_t>(F));
        UInt(Geometry,Indices.size()*Stride); for(auto I:Indices) UInt(Geometry,I,Stride);
        auto Metadata=BuildMeshAssetMetadata(Geometry);
        for(size_t I=0;I<Slots.size();++I) Metadata["materialSlots"][I]["defaultMaterial"]=Slots[I].DefaultMaterial.ToJson();
        Asset.Sections["mesh"]=JsonBytes(Metadata); Asset.Descriptor.DataVersion=2;
        Asset.Descriptor.Dependencies=AssetFile::CollectDependencies(Metadata,"Mesh");
        Asset.Descriptor.Tags={{"SubMeshes",Parts.size()},{"IndexCount",Indices.size()},{"IndexStride",Stride},{"VertexBuffers",1},{"MaterialSlots",Slots.size()},{"Bounds",Metadata["bounds"]}};
        return Output;
    }

    JsonCpp BuildMeshAssetMetadata(const std::vector<uint8_t>& Geometry)
    {
        size_t Offset=0;
        auto Read=[&](size_t Width=4) { if(Width>Geometry.size()-Offset) throw std::runtime_error("Truncated mesh geometry"); uint64_t V=0; for(size_t I=0;I<Width;++I) V|=uint64_t(Geometry[Offset++])<<(I*8); return V; };
        if(Read()!=uint32_t('EMSH')||Read()!=1) throw std::runtime_error("Unsupported mesh geometry version");
        auto VBCount=Read(),SubCount=Read(),IndexCount=Read(),IndexStride=Read(),SlotCount=Read();
        if(!VBCount||VBCount>16||(IndexStride!=2&&IndexStride!=4)||SlotCount>Geometry.size()/12||SubCount>Geometry.size()/12) throw std::runtime_error("Invalid mesh header");
        JsonCpp Metadata={{"version",1},{"materialSlots",JsonCpp::array()},{"subMeshes",JsonCpp::array()}};
        for(size_t I=0;I<SlotCount;++I) { Read(8); auto Size=Read(); if(Size>Geometry.size()-Offset) throw std::runtime_error("Truncated material slot"); Offset+=Size; Metadata["materialSlots"].push_back(JsonCpp::object()); }
        struct Layout { uint32_t Stride=0,Position=0; bool HasPosition=false; };
        std::vector<Layout> Layouts;
        for(size_t I=0;I<VBCount;++I)
        {
            Layout L; L.Stride=static_cast<uint32_t>(Read()); auto Count=Read();
            if(!L.Stride||!Count||Count>(Geometry.size()-Offset)/4) throw std::runtime_error("Invalid mesh vertex layout");
            for(size_t E=0;E<Count;++E)
            {
                auto Semantic=Read(1),Index=Read(1),Format=Read(1),Position=Read(1);
                if(Semantic==POSITION&&Index==0&&(Format==VET_Float3||Format==VET_Float4))
                { if(Position+12>L.Stride) throw std::runtime_error("Invalid mesh position offset"); L.Position=static_cast<uint32_t>(Position); L.HasPosition=true; }
            }
            Layouts.push_back(L);
        }
        struct Part { uint64_t Start,Count; }; std::vector<Part> Parts;
        for(size_t I=0;I<SubCount;++I)
        { auto Start=Read(),Count=Read(),Slot=Read(); if(Start>IndexCount||Count>IndexCount-Start||Slot>=SlotCount) throw std::runtime_error("Invalid mesh subrange"); Parts.push_back({Start,Count}); }
        std::vector<Vector3> Positions;
        AxisAlignedBox Bounds; bool Known=false;
        for(const auto& L:Layouts)
        {
            auto Size=Read(); if(Size>Geometry.size()-Offset||Size%L.Stride) throw std::runtime_error("Invalid mesh vertex bytes");
            if(L.HasPosition)
            {
                if(Known) throw std::runtime_error("Duplicate mesh position stream"); Known=true;
                for(size_t V=0;V<Size;V+=L.Stride)
                {
                    float P[3]; memcpy(P,Geometry.data()+Offset+V+L.Position,12);
                    for(float F:P) if(!std::isfinite(F)) throw std::runtime_error("Mesh position is non-finite");
                    Positions.emplace_back(P[0],P[1],P[2]); Bounds.merge(Positions.back());
                }
            }
            Offset+=Size;
        }
        auto IndexBytes=Read(); if(IndexCount>UINT32_MAX/IndexStride||IndexBytes!=IndexCount*IndexStride||IndexBytes>Geometry.size()-Offset) throw std::runtime_error("Invalid mesh indices");
        std::vector<uint32_t> Indices;
        for(size_t I=0;I<IndexCount;++I) { auto Index=Read(IndexStride); if(Known&&Index>=Positions.size()) throw std::runtime_error("Mesh index exceeds vertex count"); Indices.push_back(static_cast<uint32_t>(Index)); }
        if(Offset!=Geometry.size()) throw std::runtime_error("Mesh has trailing data");
        if(!Known) Bounds.setInfinite();
        Metadata["bounds"]=MeshBoundsToJson(Bounds);
        for(const auto& PartData:Parts)
        { AxisAlignedBox Sub; if(Known) for(size_t I=0;I<PartData.Count;++I) Sub.merge(Positions[Indices[PartData.Start+I]]); else Sub.setInfinite(); Metadata["subMeshes"].push_back({{"bounds",MeshBoundsToJson(Sub)}}); }
        return Metadata;
    }
    bool UpgradeMeshAsset(const fs::path& File)
    {
        auto Descriptor=AssetFile::ReadDescriptor(File); if(Descriptor.Type!="Mesh") return false;
        auto Metadata=AssetFile::ReadMetadata(File);
        if(Metadata.at("sections").contains("mesh")) return false;
        AssetFile::Sections Sections;
        for(const auto& Entry:Metadata.at("sections").items()) Sections[Entry.key()]=AssetFile::ReadSection(File,Entry.key());
        auto Mesh=BuildMeshAssetMetadata(Sections.at("data")); Sections["mesh"]=JsonBytes(Mesh);
        Descriptor.DataVersion=2; Descriptor.Tags["Bounds"]=Mesh["bounds"];
        AssetFile::Write(File,Descriptor,Sections); return true;
    }
}
