#pragma once
#include <fbxsdk.h>
#include <sstream>

#include "MeshResolver.hpp"
#include "ContainerReader.hpp"
#include "GameResolver.hpp"
#include "TextureResolver.hpp"
#include "SQLManager.hpp"

#include "granny.h"
#include "gstate.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

#define FT_TOOLKIT_MAJOR_VERSION 1
#define FT_TOOLKIT_MINOR_VERSION 0
#define FT_TOOLKIT_PATCH_VERSION 0
#define FT_TOOLKIT_BUILD_NUMBER  10
#define FT_TOOLKIT_SCM_BRANCH    "branch-1.0"
#define FT_TOOLKIT_SCM_TAGS      "release version-10"
#define FT_TOOLKIT_SCM_DATETIME  "2026-05-30T00:00:00.000Z"

namespace fmnext
{
    struct MaterialInstace
    {
        MaterialInstace() = default;
        MaterialInstace(std::string& path, std::shared_ptr<fmnext::BundleReader::BundleData> local, std::shared_ptr<fmnext::BundleReader::BundleData> instance) : path(path), local(local), instace(instance) {}

        std::string path;
        std::shared_ptr<fmnext::BundleReader::BundleData> local;
        std::shared_ptr<fmnext::BundleReader::BundleData> instace;
    };

    struct ModelItem
    {
        ModelItem() = default;
        ModelItem(uint32_t id, std::shared_ptr<fmnext::SceneReader::CarRenderModel11> model, std::shared_ptr<fmnext::BundleReader::BundleData> bundle, std::unordered_map<int32_t, MaterialInstace> materials, const std::string& schema, uint32_t type) : upgrade_id(id), model(model), bundle(bundle), materials(materials), schema(schema), type(type) {}

        uint32_t upgrade_id;
        std::shared_ptr<fmnext::SceneReader::CarRenderModel11> model;
        std::shared_ptr<fmnext::BundleReader::BundleData> bundle;
        std::unordered_map<int32_t, MaterialInstace> materials;
        std::string schema;
        uint32_t type;
    };
}

class DCCManager
{
public:
    DCCManager() = default;
    DCCManager(const std::string& iPath, const std::string& oPath, uint32_t lod = 0, uint32_t geo = 0, uint32_t opt = 0) : mInputPath(iPath), mOutputPath(oPath), m_lod(lod), m_geo(geo), m_opt(opt) {
    };

	~DCCManager() = default;

    bool Init();

    static std::string GetVersionString();
    
private:
	FbxManager* mManager = nullptr;
	FbxDocument* mDocument = nullptr;
    FbxScene* mScene = nullptr;
    FbxNode* mRootNode = nullptr;
    granny_file* m_CharacterFile = nullptr;

    std::filesystem::path mInputPath{};
    std::filesystem::path mOutputPath{};
    std::filesystem::path mTextureOutputPath{};
    std::filesystem::path mUIOutputPath{};
    std::filesystem::path mUITexturesOutputPath{};

    uint32_t m_lod = 0, m_geo = 0, m_opt = 0;

    std::vector<fmnext::ModelItem> list_items;

    const std::string GetContainerDirection(const std::string& container)
    {
        std::vector<std::string> lNodePos = { "LF", "RF", "RR", "LR", "LM", "RM" };

        for (const auto& position : lNodePos) {
            if (std::regex_search(container, std::regex(position))) {
                return position;
            }
        }

        return std::string();
    }

    fmnext::PartLocation GetPartDirection(const std::string& containerName)
    {
        std::string lcontainerPos = GetContainerDirection(containerName);
        std::vector<std::string> lNodePos = { "LF", "RF", "RR", "LR", "LM", "RM" };
        std::unordered_map<std::string, fmnext::PartLocation> lpartMap
        {
            { "LF", fmnext::PartLocation(fmnext::PartLocation::FRONT, fmnext::PartLocation::LEFT) },
            { "RF", fmnext::PartLocation(fmnext::PartLocation::FRONT, fmnext::PartLocation::RIGHT) },

            { "LR", fmnext::PartLocation(fmnext::PartLocation::REAR, fmnext::PartLocation::LEFT) },
            { "RR", fmnext::PartLocation(fmnext::PartLocation::REAR, fmnext::PartLocation::RIGHT) },

            { "LM", fmnext::PartLocation(fmnext::PartLocation::MID, fmnext::PartLocation::LEFT) },
            { "RM", fmnext::PartLocation(fmnext::PartLocation::MID, fmnext::PartLocation::RIGHT) }
        };

        for (const auto& position : lNodePos) {
            if (std::regex_match(lcontainerPos, std::regex(position))) {
                if (const auto& result = lpartMap.find(position); result != lpartMap.end()) {
                    return result->second;
                }
            }
        }

        return fmnext::PartLocation();
    }

    std::string build_number = {};

    std::array<std::string, 7> lodstr { "LODS", "LOD0", "LOD1", "LOD2", "LOD3", "LOD4", "LOD5" };

    std::unique_ptr<fmnext::GameResolver> m_game = nullptr;
    //std::unique_ptr<fmnext::ContainerReader> m_container = nullptr;
    std::unique_ptr<fmnext::SceneReader::Scene> m_scene = nullptr;
    std::shared_ptr<fmnext::DataBaseRecords> m_records = nullptr;

    std::shared_ptr<fmnext::BundleReader::BundleData> m_tire = nullptr;
    std::shared_ptr<fmnext::BundleReader::BundleData> m_skel = nullptr;
    std::shared_ptr<fmnext::BundleReader::BundleData> m_proxyLOD = nullptr;
    std::shared_ptr<fmnext::BundleReader::BundleData> m_colors = nullptr;
    std::unique_ptr<fmnext::BundleReader::BundleData> m_thumbnail = nullptr;

    std::unordered_map<std::string, std::shared_ptr<fmnext::BundleReader::BundleData>> m_tires;

    std::map<int, bool> car_bodies{};
    std::map<int, bool> car_upgrades{};

    struct aligned_deleter { void operator()(void* p) { _aligned_free(p); } };

    fmnext::ContainerReader m_container = {};

    std::vector<fmnext::ContainerReader> materials_container;
    std::vector<fmnext::ContainerReader> textures_container;

    std::unordered_map<std::string, std::vector<char>> material_list;
    std::unordered_map<std::string, std::string> texture_list;

    void Initialize(std::shared_ptr<fmnext::DataBaseRecords> p_records);

	void InitializeSdkObjects(FbxManager*& pManager)
	{
		//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
		pManager = FbxManager::Create();
		if (!pManager)
		{
			FBXSDK_printf("Error: Unable to create FBX Manager!\n");
			exit(1);
		}
		else FBXSDK_printf("\tAutodesk FBX SDK %s\n", pManager->GetVersion());

		//Create an IOSettings object. This object holds all import/export settings.
		FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
		pManager->SetIOSettings(ios);
	}

	void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
	{
		//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
		if (pManager) pManager->Destroy();
		//if (pExitStatus) FBXSDK_printf("Finished!\n");
	}

    // Export document, the format is ascii by default
    bool SaveDocument(FbxManager* pManager, FbxDocument* pDocument, const char* pFilename, int pFileFormat = -1, bool pEmbedMedia = false)
    {
        int lMajor, lMinor, lRevision;
        bool lStatus = true;

        // Create an exporter.
        FbxExporter* lExporter = FbxExporter::Create(pManager, "");

        // Initialize the exporter by providing a filename.
        if (lExporter->Initialize(pFilename, -1, pManager->GetIOSettings()) == false)
        {
            FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
            FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
            return false;
        }

        FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
        FBXSDK_printf("FBX version number for this version of the FBX SDK is %d.%d.%d\n", lMajor, lMinor, lRevision);
        FBXSDK_printf("FBX saved to %s \n\n", pFilename);

        // Export the scene.
        //lStatus = lExporter->Export(pDocument);
        lStatus = lExporter->Export(mScene);
        
        // Destroy the exporter.
        lExporter->Destroy();
        return lStatus;
    }

    bool StringContains(const std::string& pSource, const std::string& pName)
    {
        return std::regex_search(pSource, std::regex(pName, std::regex::icase));
    }

    bool SetupOutpuDirectory()
    {
        mOutputPath /= std::filesystem::path(mInputPath).stem();
        mOutputPath.make_preferred();

        if (!std::filesystem::exists(mOutputPath))
        {
            return std::filesystem::create_directory(mOutputPath);
        }

        return false;
    }

    bool SetupOutputTextures()
    {
        mTextureOutputPath = std::filesystem::path(mOutputPath);
        mTextureOutputPath /= "Textures";
        mTextureOutputPath.make_preferred();

        if (!std::filesystem::exists(mTextureOutputPath))
        {
            return std::filesystem::create_directory(mTextureOutputPath);
        }

        return false;
    }

    bool SetupOutputDigitalGauge()
    {
        mUIOutputPath = std::filesystem::path(mOutputPath);
        mUIOutputPath /= "UI";
        mUIOutputPath.make_preferred();

        if (!std::filesystem::exists(mUIOutputPath))
        {
            return std::filesystem::create_directory(mUIOutputPath);
        }

        return false;
    }

    bool SetupOutputDigitalGaugeTextures()
    {
        mUITexturesOutputPath = std::filesystem::path(mUIOutputPath);
        mUITexturesOutputPath /= "Textures";
        mUITexturesOutputPath.make_preferred();

        if (!std::filesystem::exists(mUITexturesOutputPath))
        {
            return std::filesystem::create_directory(mUITexturesOutputPath);
        }

        return false;
    }

    std::string MakeCarRelativePath(const std::string& path) {
        std::string result = "game:/media/cars/";
        result += m_scene->media_name;
        result += "/";
        result += path;

        std::replace(result.begin(), result.end(), '/', '\\');

        return result;
    }

    uint32_t GetHexColor(uint32_t RR, uint32_t GG, uint32_t BB) {
        return ((RR & 0xff) << 16) + ((GG & 0xff) << 8) + (BB & 0xff);
    }

    std::shared_ptr<fmnext::BundleReader::BundleData> SetBundleData(const std::shared_ptr<fmnext::SceneReader::CarRenderModel11>& model)
    {
        std::string path = fmnext::GameResolver::Remove(model->path, m_scene->media_name).string();
        std::replace(path.begin(), path.end(), '\\', '/');

        std::vector<char> bundle_buffer{};
        if (m_container.findName(path, bundle_buffer))
        {
            auto reader = fmnext::BundleReader(bundle_buffer);

            if (reader.Init())
            {
                return std::make_shared<fmnext::BundleReader::BundleData>(reader.bundle);
            }
        }

        return nullptr;
    }

    std::shared_ptr<fmnext::BundleReader::BundleData> GetBundleData(const std::string& path)
    {
        std::vector<char> bundle_buffer{};
        if (m_container.findName(path, bundle_buffer))
        {
            auto reader = fmnext::BundleReader(bundle_buffer);

            if (reader.Init())
            {
                return std::make_shared<fmnext::BundleReader::BundleData>(reader.bundle);
            }
        }

        return nullptr;
    }

    std::shared_ptr<fmnext::BundleReader::BundleData> GetBundleData(std::vector<char>& data)
    {
        auto reader = fmnext::BundleReader(data);

        if (reader.Init())
        {
            return std::make_shared<fmnext::BundleReader::BundleData>(reader.bundle);
        }

        return nullptr;
    }

    void SetSkeleton(const std::string& pPath)
    {
        std::string lPath = fmnext::GameResolver::Remove(pPath, m_scene->media_name).string();
        std::replace(lPath.begin(), lPath.end(), '\\', '/');

        std::vector<char> skel_buffer{};
        if (m_container.findName(lPath, skel_buffer))
        {
            m_skel = GetBundleData(skel_buffer);
        }  
    }

    void SetProxyLOD(const std::string& pPath)
    {
        std::string lPath = fmnext::GameResolver::Remove(pPath, m_scene->media_name).string();
        std::replace(lPath.begin(), lPath.end(), '\\', '/');

        std::vector<char> proxyLOD_buffer{};
        if (m_container.findName(lPath, proxyLOD_buffer))
        {
            m_proxyLOD = GetBundleData(proxyLOD_buffer);
        }
    }

    FbxNode* CreateLocator(const DirectX::XMMATRIX& xmMatrix)
    {
        // create a FbxNode
        FbxNode* lNode = FbxNode::Create(mManager, "");

        SetNodeTransformation(lNode, xmMatrix);

        return lNode;
    }

    FbxNode* CreateLocator(const std::string& pName = "")
    {
        return FbxNode::Create(mManager, pName.c_str());
    }
    
    DirectX::XMFLOAT3 XMQuaternionToEuler(const DirectX::XMMATRIX& pXMMatrix)
    {
        DirectX::XMFLOAT4X4 pDest;
        DirectX::XMStoreFloat4x4(&pDest, pXMMatrix);

        float cy = sqrtf(pDest._11 * pDest._11 + pDest._21 * pDest._21);

        if (cy > 16 * FLT_EPSILON) {
            return DirectX::XMFLOAT3(DirectX::XMConvertToDegrees(atan2f(pDest._32, pDest._33)), DirectX::XMConvertToDegrees(atan2f(-pDest._31, cy)), DirectX::XMConvertToDegrees(atan2f(pDest._21, pDest._11)));
        }
        else {
            DirectX::XMFLOAT3(DirectX::XMConvertToDegrees(atan2f(-pDest._23, pDest._22)), DirectX::XMConvertToDegrees(atan2f(-pDest._31, cy)), DirectX::XMConvertToDegrees(0));
        }
        
        return DirectX::XMFLOAT3();
    }

    void SetNodeTransformation(FbxNode* pNode, DirectX::XMMATRIX pXMMatrix)
    {
        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, pXMMatrix);

        DirectX::XMVECTOR outRotQuat_updated = DirectX::XMVectorSet(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat));
        //DirectX::XMVECTOR outRotQuat_inverse = DirectX::XMQuaternionInverse(outRotQuat_updated);

        DirectX::XMFLOAT3 rotation = XMQuaternionToEuler(DirectX::XMMatrixRotationQuaternion(outRotQuat_updated));

        pNode->LclTranslation.Set(FbxDouble3(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans)));
        pNode->LclRotation.Set({ static_cast<double>(rotation.x), static_cast<double>(rotation.y), static_cast<double>(rotation.z) });
        pNode->LclScaling.Set(FbxDouble3(DirectX::XMVectorGetX(outScale), DirectX::XMVectorGetZ(outScale), DirectX::XMVectorGetY(outScale)));
    }

    FbxMesh* RemoveIsolatedVertices(FbxMesh* pMesh);
    
    FbxNode* CreateMesh(const std::vector<DirectX::XMFLOAT3>& vertices, const std::vector<uint32_t>& indices, const std::vector<DirectX::XMFLOAT3>& normals, const std::vector<std::vector<DirectX::XMFLOAT2>>& uvs, const std::string& Name, FbxSurfaceMaterial* material, bool useQuads);

    FbxSurfaceLambert* CreateMaterialfromMemory(const std::string& pName, const std::shared_ptr<fmnext::BundleReader::BundleData>& pMaterialBundle)
    {
        FbxSurfaceLambert* lMaterial = FbxSurfaceLambert::Create(mScene, pName.c_str());
        FbxString lShadingModelName = true ? "Lambert" : "Phong";

        // Generate primary and secondary colors.
        //lMaterial->Emissive.Set(lBlack);
        //lMaterial->Ambient.Set(lRed);
        //lMaterial->Diffuse.Set(FbxDouble3(1.0, 0.0, 0.0));
        //lMaterial->TransparencyFactor.Set(0.0);
        lMaterial->ShadingModel.Set(lShadingModelName);

        if (pMaterialBundle)
        {
            //MGlobal::displayWarning("Using shader information");
            std::vector<DirectX::XMFLOAT4> colors;

            for (const auto& param : pMaterialBundle->ShaderParameters)
            {
                if (param.type == fmnext::ShaderParameter_Color)
                {
                    colors.push_back(std::any_cast<DirectX::XMFLOAT4>(param.value));

                    DirectX::XMFLOAT4 diffuse = std::any_cast<DirectX::XMFLOAT4>(param.value);
                }

                if (param.type == fmnext::ShaderParameter_Texture2D)
                {
                    std::string path = std::any_cast<std::string>(param.value);
                    //MGlobal::displayInfo(path.c_str());
                }
            }


            if (!colors.empty())
            {
                lMaterial->Diffuse.Set(FbxDouble3(colors[0].x, colors[0].y, colors[0].z));
            }
            else
            {
                lMaterial->Diffuse.Set(FbxDouble3(0.106f, 0.086f, 0.082f));
            }
        }

        return lMaterial;
    }

    FbxSurfaceLambert* CreateCarpaintfromMemory(const std::string& pName, const DirectX::XMFLOAT3& pColor)
    {
        FbxSurfaceLambert* lMaterial = FbxSurfaceLambert::Create(mScene, pName.c_str());
        FbxString lShadingModelName = true ? "Lambert" : "Phong";

        // Generate primary and secondary colors.
        //lMaterial->Emissive.Set(lBlack);
        //lMaterial->Ambient.Set(lRed);
        //lMaterial->Diffuse.Set(FbxDouble3(1.0, 0.0, 0.0));
        //lMaterial->TransparencyFactor.Set(0.0);
        lMaterial->ShadingModel.Set(lShadingModelName);

        lMaterial->Diffuse.Set(FbxDouble3(pColor.x, pColor.y, pColor.z));

        return lMaterial;
    }

    FbxSurfaceLambert* CreateGlassfromMemory(const std::string& pName)
    {
        FbxSurfaceLambert* lMaterial = FbxSurfaceLambert::Create(mScene, pName.c_str());
        FbxString lShadingModelName = true ? "Lambert" : "Phong";

        // Generate primary and secondary colors.
        //lMaterial->Emissive.Set(lBlack);
        //lMaterial->Ambient.Set(lRed);
        //lMaterial->Diffuse.Set(FbxDouble3(1.0, 0.0, 0.0));
        lMaterial->TransparencyFactor.Set(1.0);
        lMaterial->ShadingModel.Set(lShadingModelName);

        lMaterial->Diffuse.Set(FbxDouble3(1.0, 1.0, 1.0));

        return lMaterial;
    }

    std::vector<char> FindAssetInContainer(const std::string& path, int type = 0)
    {
        std::vector<char> blob{};

        switch (type)
        {
        case 0:
        {
            for (auto& mat_container : materials_container)
            {
                if (mat_container.findName(path, blob))
                {
                    return blob;
                }
            }
            break;
        }
        case 1:
        {
            for (auto& tex_container : textures_container)
            {
                if (tex_container.findName(path, blob))
                {
                    return blob;
                }
            }
            break;
        }
        }

        return blob;
    }

    std::unordered_map<int32_t, fmnext::MaterialInstace> HandleShaders(const std::shared_ptr<fmnext::SceneReader::CarRenderModel11>& model, const std::shared_ptr<fmnext::BundleReader::BundleData>& bundle, const std::string& scheme)
    {
        std::unordered_map<int32_t, fmnext::MaterialInstace> result;

        for (auto& materialBundle : bundle->MaterialInstanceBundles)
        {
            if (!materialBundle.data.empty())
            {
                std::shared_ptr<fmnext::BundleReader::BundleData> primary_material = GetBundleData(materialBundle.data);

                // std::any_cast<std::string>(materialBundle.metadata["Name"]))

                if (primary_material)
                {
                    for (auto& param : primary_material->ShaderParameters)
                    {
                        if (param.type == fmnext::ShaderParameter_Texture2D)
                        {
                            std::string texture = std::any_cast<std::string>(param.value);

                            texture_list.try_emplace(texture, std::filesystem::path(texture).filename().string());
                        }
                    }

                    for (auto& instance : primary_material->MaterialInstances)
                    {
                        if (fmnext::GameResolver::Contains(instance, m_scene->media_name))
                        {
                            std::string path = m_game->Remove(instance, m_scene->media_name).string();
                            std::replace(path.begin(), path.end(), '\\', '/');

                            std::vector<char> material_buffer{};
                            if (m_container.findName(path, material_buffer))
                            {
                                auto secondary_material = GetBundleData(material_buffer);

                                material_list.try_emplace(path, material_buffer);

                                result.emplace(std::any_cast<int32_t>(materialBundle.metadata["Id"]), fmnext::MaterialInstace(instance, primary_material, secondary_material));

                                for (const auto& param : secondary_material->ShaderParameters)
                                {
                                    if (param.type == fmnext::ShaderParameter_Texture2D)
                                    {
                                        std::string texture = std::any_cast<std::string>(param.value);
                                        texture_list.try_emplace(texture, std::filesystem::path(texture).filename().string());
                                    }
                                }
                            }

                        }
                        else
                        {
                            std::string path = m_game->Remove(instance).string();
                            std::replace(path.begin(), path.end(), '\\', '/');

                            std::vector<char> material_buffer = FindAssetInContainer(path);
                            if (!material_buffer.empty())
                            {
                                auto secondary_material = GetBundleData(material_buffer);

                                material_list.try_emplace(path, material_buffer);

                                result.emplace(std::any_cast<int32_t>(materialBundle.metadata["Id"]), fmnext::MaterialInstace(instance, primary_material, secondary_material));

                                for (const auto& param : secondary_material->ShaderParameters)
                                {
                                    if (param.type == fmnext::ShaderParameter_Texture2D)
                                    {
                                        std::string texture = std::any_cast<std::string>(param.value);
                                        texture_list.try_emplace(texture, std::filesystem::path(texture).filename().string());
                                    }
                                }

                            }
                        }

                    }
                }
            }
        }

        return result;
    }

    std::string GetBuildNumber(const std::vector<char>& buffer)
    {
        return std::string(buffer.begin(), buffer.end() - 2);
    }

    std::string GetStringGUID(GUID guid)
    {
        LPOLESTR lpsz;
        if (StringFromCLSID(guid, &lpsz) == S_OK)
        {
            std::string result{};
            result.resize(39);
            wcstombs_s(nullptr, result.data(), 39, lpsz, _TRUNCATE);
            CoTaskMemFree(lpsz);

            return result;
        };

        return std::string("{}");
    }

    std::string GetStringGUIDWithoutBraces(const std::string& guid)
    {
        return std::string(guid.begin() + 1, guid.end() - 2 /* 2 = with nullterminator */);
    }

    void HandleProxyLOD()
    {
        auto resolver = fmnext::MeshResolver(m_proxyLOD, m_lod, static_cast<fmnext::GeometryType>(m_geo));

        for (auto& mesh : resolver.GetMeshes())
        {
            FbxNode* mesh_obj = nullptr;
            FbxSurfaceLambert* material_obj = nullptr;

            mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, "ProxyLOD", material_obj, static_cast<fmnext::GeometryType>(m_geo));
            SetNodeTransformation(mesh_obj, mesh.matrix);

            mRootNode->AddChild(mesh_obj);
        }
    }

    void HandleSkeleton(const std::vector<fmnext::BundleReader::Bones>& skeleton)
    {
        std::vector<FbxNode*> joints;
        joints.reserve(skeleton.size());

        for (size_t i = 0; i < skeleton.size(); ++i)
        {
            std::string bone_name = (skeleton[i].name == "<root>") ? "BundleRootBone" : skeleton[i].name;
            std::string Name(UpdateNodeName(bone_name).c_str());

            FbxSkeleton* lSkeletonAttribute = FbxSkeleton::Create(mScene, Name.c_str());
            lSkeletonAttribute->SetSkeletonType((skeleton[i].parent_index == GrannyNoParentBone) ? FbxSkeleton::eRoot : FbxSkeleton::eLimbNode);

            FbxNode* lSkeletonJoint = FbxNode::Create(mScene, Name.c_str());
            lSkeletonJoint->SetNodeAttribute(lSkeletonAttribute);

            SetNodeTransformation(lSkeletonJoint, skeleton[i].transform);

            joints.push_back(lSkeletonJoint);

            if (skeleton[i].parent_index == GrannyNoParentBone)
            {
                mRootNode->AddChild(joints[i]);
            }

            if (skeleton[i].parent_index != GrannyNoParentBone)
            {
                int32_t ParentIndex = skeleton[i].parent_index;

                joints[ParentIndex]->AddChild(joints[i]);
            }

        }
    }


    void HandleSkeleton(granny_skeleton* Skeleton)
    {
        std::string skel_name(Skeleton->Name);

        std::vector<FbxNode*> joints;
        joints.reserve(Skeleton->BoneCount);

        for (size_t i = 0; i < Skeleton->BoneCount; ++i)
        {
            granny_transform LocalTransform = Skeleton->Bones[i].LocalTransform; // Position[3], Orientation[4], ScaleShear[3]
            std::string Name(UpdateNodeName(Skeleton->Bones[i].Name).c_str());

            FbxSkeleton* lSkeletonAttribute = FbxSkeleton::Create(mScene, Name.c_str());
            lSkeletonAttribute->SetSkeletonType((Skeleton->Bones[i].ParentIndex == GrannyNoParentBone) ? FbxSkeleton::eRoot : FbxSkeleton::eLimbNode);

            FbxNode* lSkeletonJoint = FbxNode::Create(mScene, Name.c_str());
            lSkeletonJoint->SetNodeAttribute(lSkeletonAttribute);
            lSkeletonJoint->LclTranslation.Set(FbxVector4(LocalTransform.Position[0], LocalTransform.Position[2], LocalTransform.Position[1]));

            DirectX::XMVECTOR outRotQuat_updated = DirectX::XMVectorSet(LocalTransform.Orientation[0], LocalTransform.Orientation[2], LocalTransform.Orientation[1], LocalTransform.Orientation[3]);
            DirectX::XMFLOAT3 rotation = XMQuaternionToEuler(DirectX::XMMatrixRotationQuaternion(outRotQuat_updated));

            lSkeletonJoint->LclRotation.Set(FbxVector4(rotation.x, rotation.y, rotation.z));
            //lSkeletonJoint->LclScaling.Set( FbxVector4( static_cast<double>(*LocalTransform.ScaleShear[0]), static_cast<double>(*LocalTransform.ScaleShear[2]), static_cast<double>(*LocalTransform.ScaleShear[1]) ) );

            joints.push_back(lSkeletonJoint);

            if (Skeleton->Bones[i].ParentIndex == GrannyNoParentBone)
            {
                mRootNode->AddChild(joints[i]);
            }

            if (Skeleton->Bones[i].ParentIndex != GrannyNoParentBone)
            {
                granny_int32 ParentIndex = Skeleton->Bones[i].ParentIndex;

                joints[ParentIndex]->AddChild(joints[i]);
            }
        }
    }

    bool SetSkeleton(const std::vector<char>& blob)
    {
        m_CharacterFile = GrannyReadEntireFileFromMemory(static_cast<granny_int32x>(blob.size()), blob.data());
        granny_file_info* m_GrannyFileInfo = GrannyGetFileInfo(m_CharacterFile);

        if (m_CharacterFile == 0 && m_GrannyFileInfo == 0)
        {
            printf("Could not extract a granny_file_info from the file.\n");
        }
        else
        {
            if (m_GrannyFileInfo->Skeletons != nullptr)
            {
                // Assume there is only a single skeleton on each gr2
                HandleSkeleton(m_GrannyFileInfo->Skeletons[0]);
            }

            return true;
        }

        return false;
    }

    bool GetSkeleton()
    {
        printf("Granny Version: %s \n", GrannyGetVersionString());

        if (m_CharacterFile != nullptr)
        {
            granny_file_info* m_GrannyFileInfo = GrannyGetFileInfo(m_CharacterFile);

            if (m_CharacterFile == 0 && m_GrannyFileInfo == 0)
            {
                printf("Could not extract a granny_file_info from the file.\n");
            }
            else
            {
                if (m_GrannyFileInfo->Skeletons != nullptr)
                {
                    // Assume there is only a single skeleton on each gr2
                    HandleSkeleton(m_GrannyFileInfo->Skeletons[0]);
                }

                return true;
            }
        }

        return false;
    }

    //ContextUID_state_machine
    int InitStateMachine(const std::vector<char>& state);

    FbxAnimLayer* CreateAnimationLayer(const std::string& pName)
    {
        FbxAnimStack* mAnimStack = FbxAnimStack::Create(mScene, pName.c_str());

        // Create the base layer (this is mandatory)
        FbxAnimLayer* myAnimBaseLayer = FbxAnimLayer::Create(mScene, "");
        mAnimStack->AddMember(myAnimBaseLayer);

        return myAnimBaseLayer;
    }

    bool HandleAnimation(granny_skeleton* Skeleton = nullptr, granny_animation* Animation = nullptr, const std::string& pLayerName = "Base Layer")
    {
        if (Skeleton && Animation)
        {
            // Assume there is only a single animation/track on each gr2
            granny_track_group* TrackGroup = Animation->TrackGroups[0];

            uint32_t TotalSamples = (uint32_t)(Animation->Duration * 60);

            FbxAnimLayer* lAnimLayer = CreateAnimationLayer(pLayerName);

            for (granny_int32 TrackIndex = 0; TrackIndex < TrackGroup->TransformTrackCount; ++TrackIndex) {

                std::string boneName = TrackGroup->TransformTracks[TrackIndex].Name;

                granny_int32x BoneIndex{};
                if (!GrannyFindBoneByName(Skeleton, TrackGroup->TransformTracks[TrackIndex].Name, &BoneIndex)) {

                    std::string message = "Granny: Unable to find bone ";
                    message += boneName.c_str();
                    message += " \n";

                    //std::cout << message;

                    continue;
                }
               
                if (BoneIndex != GrannyNoParentBone)
                {
                    FbxNode* lObjectNode = mScene->FindNodeByName(UpdateNodeName(boneName).c_str());

                    if (!lObjectNode)
                    {
                        return false;
                    }

                    FbxAnimCurveNode* myAnimCurveNode = lObjectNode->LclTranslation.GetCurveNode(lAnimLayer, true);

                    if (!lObjectNode)
                    {
                        std::string message = "Unable to find FbxNode for bone ";
                        message += boneName;
                        message += " \n";

                        std::cout << message;

                        return false;
                    }

                    int lKeyIndex = 0;

                    FbxAnimCurve* lTranXCurve = lObjectNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    FbxAnimCurve* lTranYCurve = lObjectNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    FbxAnimCurve* lTranZCurve = lObjectNode->LclTranslation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

                    FbxAnimCurve* lRotXCurve = lObjectNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
                    FbxAnimCurve* lRotYCurve = lObjectNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
                    FbxAnimCurve* lRotZCurve = lObjectNode->LclRotation.GetCurve(lAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

                    auto setKey = [&](FbxAnimCurve* myAnimCurve, granny_real32 pCurrentTime, int lKeyIndex, float value) {

                        FbxTime lTime;
                        myAnimCurve->KeyModifyBegin();
                        lTime.SetSecondDouble(pCurrentTime);
                        lKeyIndex = myAnimCurve->KeyAdd(lTime);

                        myAnimCurve->KeySet(lKeyIndex, lTime, value, FbxAnimCurveDef::eInterpolationLinear);

                        myAnimCurve->KeyModifyEnd();
                    };

                    for (uint32_t CurrentFrame = 0; CurrentFrame < TotalSamples; ++CurrentFrame)
                    {
                        granny_real32 CurrentTime = CurrentFrame * (Animation->Duration / (TotalSamples - 1));

                        // Get the three curves that specify the transform
                        granny_curve2* PositionCurve = &TrackGroup->TransformTracks[TrackIndex].PositionCurve;
                        granny_curve2* OrientationCurve = &TrackGroup->TransformTracks[TrackIndex].OrientationCurve;
                        granny_curve2* ScaleShearCurve = &TrackGroup->TransformTracks[TrackIndex].ScaleShearCurve; // going to ignore this for a while

                        granny_transform EvalResult{};
                        GrannyMakeIdentity(&EvalResult);

                        GrannyEvaluateCurveAtT(3, false, false, PositionCurve, true, Animation->Duration, CurrentTime, EvalResult.Position, GrannyCurveIdentityPosition);
                        GrannyEvaluateCurveAtT(4, false, false, OrientationCurve, true, Animation->Duration, CurrentTime, EvalResult.Orientation, GrannyCurveIdentityOrientation);

                        setKey(lTranXCurve, CurrentTime, CurrentFrame, EvalResult.Position[0]);
                        setKey(lTranYCurve, CurrentTime, CurrentFrame, EvalResult.Position[2]);
                        setKey(lTranZCurve, CurrentTime, CurrentFrame, EvalResult.Position[1]);

                        DirectX::XMVECTOR outRotQuat_updated = DirectX::XMVectorSet(EvalResult.Orientation[0], EvalResult.Orientation[2], EvalResult.Orientation[1], EvalResult.Orientation[3]);
                        DirectX::XMFLOAT3 rotation = XMQuaternionToEuler(DirectX::XMMatrixRotationQuaternion(outRotQuat_updated));

                        setKey(lRotXCurve, CurrentTime, CurrentFrame, rotation.x);
                        setKey(lRotYCurve, CurrentTime, CurrentFrame, rotation.y);
                        setKey(lRotZCurve, CurrentTime, CurrentFrame, rotation.z);

                        //printf("Track: %s, Frame: %i: Position[% .3f, % .3f, % .3f]\n", TrackGroup->TransformTracks[TrackIndex].Name, CurrentFrame, EvalResult.Position[0], EvalResult.Position[1], EvalResult.Position[2]);
                        //printf("Time: %.3f: Rotation[% .3f, % .3f, % .3f, % .3f]\n", CurrentTime, EvalResult.Orientation[0], EvalResult.Orientation[1], EvalResult.Orientation[2], EvalResult.Orientation[3]);
                    }

                }
            }

            return true;
        }

        return false;
    }

    static std::string UpdateNodeName(const std::string& str)
    {
        return std::regex_replace(str, std::regex("([^a-zA-Z0-9\\w]+|\\s+|^[0-9]+)"), std::string("_"));
    }

    static std::filesystem::path DeduplicatePath(const std::string& path) {
        std::filesystem::path result(path);
        int index = 1;

        while (std::filesystem::exists(result))
        {
            std::string filename = std::filesystem::path(path).stem().string();
            filename += "_";
            filename += std::to_string(index);
            filename += std::filesystem::path(path).extension().string();

            result = std::filesystem::path(path).replace_filename(filename);

            ++index;
        }

        return result;
    }

    std::vector<char> HandleLibrary(const std::string& path)
    {
        std::string base_path = path;
        std::replace(base_path.begin(), base_path.end(), '\\', '/');

        std::vector<std::string> libraries{ "cars/_library", "media/_library", std::string("media/cars/" + m_scene->media_name) };
        std::vector<char> blob{};
        std::smatch match_result;
        for (auto& library : libraries)
        {
            if (std::regex_search(base_path, match_result, std::regex(library, std::regex_constants::icase)))
                break;
        }

        // lowercase the result
        std::string library = match_result.str();
        std::transform(library.begin(), library.end(), library.begin(), [](char c) { return std::tolower(c); });

        auto media = std::find(libraries.begin(), libraries.end(), library);

        if (media != std::end(libraries))
        {
            int64_t type = std::distance(libraries.begin(), media);

            switch (type)
            {
            case 0:
            case 1:
            {
                std::string path_fix = m_game->Remove(path).string();
                std::replace(path_fix.begin(), path_fix.end(), '\\', '/');

                blob = FindAssetInContainer(path_fix, 1);

                if (!blob.empty())
                {
                    return blob;
                }

                break;
            }
            case 2:
            {
                std::string path_fix = m_game->Remove(path, m_scene->media_name).string();
                std::replace(path_fix.begin(), path_fix.end(), '\\', '/');

                if (m_container.findName(path_fix, blob))
                {
                    return blob;
                }

                break;
            }
            }

        }

        return blob;
    }
    
    rapidjson::Value StringToValue(const std::string& value, rapidjson::Document::AllocatorType& allocator);
    std::string GetHexHash(int value);
    rapidjson::Value GetShaderParametersArray(std::shared_ptr<fmnext::BundleReader::BundleData> bundle, rapidjson::Document::AllocatorType& allocator);
    void ExportManufacturerColors();
};
