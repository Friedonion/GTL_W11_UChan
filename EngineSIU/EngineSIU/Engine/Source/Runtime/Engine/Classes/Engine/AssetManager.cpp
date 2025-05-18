#include "AssetManager.h"
#include "Engine.h"

#include <filesystem>

#include "FbxLoader.h"
#include "Animation/Skeleton.h"
#include "SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Components/Material/Material.h"
#include "Engine/FObjLoader.h"
#include "UObject/Casts.h"
#include "Asset/SkeletalMeshAsset.h"
#include "Asset/StaticMeshAsset.h"
#include "Serialization/MemoryArchive.h"
#include "UObject/ObjectFactory.h"

bool UAssetManager::IsInitialized()
{
    return GEngine && GEngine->AssetManager;
}

UAssetManager& UAssetManager::Get()
{
    if (UAssetManager* Singleton = GEngine->AssetManager)
    {
        return *Singleton;
    }
    else
    {
        UE_LOG(ELogLevel::Error, "Cannot use AssetManager if no AssetManagerClassName is defined!");
        assert(0);
        return *new UAssetManager; // never calls this
    }
}

UAssetManager* UAssetManager::GetIfInitialized()
{
    return GEngine ? GEngine->AssetManager : nullptr;
}

void UAssetManager::InitAssetManager()
{
    AssetRegistry = std::make_unique<FAssetRegistry>();

    LoadContentFiles();
}

const TMap<FName, FAssetInfo>& UAssetManager::GetAssetRegistry()
{
    return AssetRegistry->PathNameToAssetInfo;
}

TMap<FName, FAssetInfo>& UAssetManager::GetAssetRegistryRef()
{
    return AssetRegistry->PathNameToAssetInfo;
}

USkeletalMesh* UAssetManager::GetSkeletalMesh(const FName& Name)
{
    if (SkeletalMeshMap.Contains(Name))
    {
        return SkeletalMeshMap[Name];
    }
    return nullptr;
}

UStaticMesh* UAssetManager::GetStaticMesh(const FName& Name)
{
    if (StaticMeshMap.Contains(Name))
    {
        return StaticMeshMap[Name];
    }
    return nullptr;
}

USkeleton* UAssetManager::GetSkeleton(const FName& Name)
{
    if (SkeletonMap.Contains(Name))
    {
        return SkeletonMap[Name];
    }
    return nullptr;
}

UMaterial* UAssetManager::GetMaterial(const FName& Name)
{
    if (MaterialMap.Contains(Name))
    {
        return MaterialMap[Name];
    }
    return nullptr;
}

UAnimationAsset* UAssetManager::GetAnimation(const FName& Name)
{
    if (AnimationMap.Contains(Name))
    {
        return AnimationMap[Name];
    }
    return nullptr;
}

void UAssetManager::AddAssetInfo(const FAssetInfo& Info)
{
    AssetRegistry->PathNameToAssetInfo.Add(Info.AssetName, Info);
}

void UAssetManager::AddSkeleton(const FName& Key, USkeleton* Skeleton)
{
    SkeletonMap.Add(Key, Skeleton);
}

void UAssetManager::AddSkeletalMesh(const FName& Key, USkeletalMesh* SkeletalMesh)
{
    SkeletalMeshMap.Add(Key, SkeletalMesh);
}

void UAssetManager::AddMaterial(const FName& Key, UMaterial* Material)
{
    MaterialMap.Add(Key, Material);
}

void UAssetManager::AddStaticMesh(const FName& Key, UStaticMesh* StaticMesh)
{
    StaticMeshMap.Add(Key, StaticMesh);
}

void UAssetManager::AddAnimation(const FName& Key, UAnimationAsset* Animation)
{
    AnimationMap.Add(Key, Animation);
}

void UAssetManager::LoadContentFiles()
{
    const std::string BasePathName = "Contents/";

    for (const auto& Entry : std::filesystem::recursive_directory_iterator(BasePathName))
    {
        if (!Entry.is_regular_file() || Entry.path().extension() == ".bin")
        {
            continue;
        }

        if (Entry.path().extension() == ".obj")
        {
            // Obj 파일 로드
            FAssetInfo NewAssetInfo;
            NewAssetInfo.AssetName = FName(Entry.path().filename().generic_string());
            NewAssetInfo.PackagePath = FName(Entry.path().parent_path().generic_string());
            NewAssetInfo.SourceFilePath = Entry.path().generic_string();
            NewAssetInfo.AssetType = EAssetType::StaticMesh; // obj 파일은 무조건 StaticMesh
            NewAssetInfo.Size = static_cast<uint32>(std::filesystem::file_size(Entry.path()));
            
            AssetRegistry->PathNameToAssetInfo.Add(NewAssetInfo.AssetName, NewAssetInfo);
            
            FString MeshName = NewAssetInfo.GetFullPath();
            FObjManager::CreateStaticMesh(MeshName);
        }
        else if (Entry.path().extension() == ".fbx")
        {
            FAssetInfo AssetInfo = {};
            AssetInfo.SourceFilePath = Entry.path().generic_string();
            AssetInfo.PackagePath = FName(Entry.path().parent_path().generic_string());
            AssetInfo.Size = static_cast<uint32>(std::filesystem::file_size(Entry.path()));

            HandleFBX(AssetInfo);
        }
    }

    OutputDebugStringA(std::format("FBX Load Time: {:.2f} s\nBinary Load Time: {:.2f} s", FbxLoadTime / 1000.0, BinaryLoadTime / 1000.0).c_str());
}

void UAssetManager::HandleFBX(const FAssetInfo& AssetInfo)
{
    // TODO : ControlEditorPanel Viwer Open과 코드 중복 다수
    // 경로, 이름 준비
    FString BaseName;

    FWString FilePath = AssetInfo.SourceFilePath.ToWideString();
    FWString FolderPath = FilePath.substr(0, FilePath.find_last_of(L"\\/") + 1);
    FWString FileName = FilePath.substr(FilePath.find_last_of(L"\\/") + 1);
    size_t DotPos = FileName.find_last_of('.');
    if (DotPos != std::string::npos)
    {
        BaseName = FileName.substr(0, DotPos);
    }
    else
    {
        BaseName = FileName;
    }

    bool bIsBinaryValid = false;
    const FString BinFilePath = FolderPath / BaseName + ".bin";
    std::filesystem::path BinFile = *BinFilePath;
    if (std::filesystem::exists(BinFile))
    {
        const std::filesystem::file_time_type BinTime = std::filesystem::last_write_time(BinFile);
        const std::filesystem::file_time_type FbxTime = std::filesystem::last_write_time(FilePath);

        if (FbxTime <= BinTime)
        {
            bIsBinaryValid = true;
        }
    }
    
    FAssetLoadResult Result;
    if (bIsBinaryValid)
    {
        LARGE_INTEGER StartTime;
        LARGE_INTEGER EndTime;
        
        QueryPerformanceCounter(&StartTime);

        // bin 파일 읽기
        bIsBinaryValid = LoadFbxBinary(BinFilePath, Result, BaseName, FolderPath);
        
        QueryPerformanceCounter(&EndTime);

        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        if (bIsBinaryValid)
        {
            BinaryLoadTime += (static_cast<double>(EndTime.QuadPart - StartTime.QuadPart) * 1000.f / static_cast<double>(Frequency.QuadPart));
        }
    }
    
    if (!bIsBinaryValid)
    {
        LARGE_INTEGER StartTime;
        LARGE_INTEGER EndTime;
        
        QueryPerformanceCounter(&StartTime);
        
        // FBX 로더로 파일 읽기
        FFbxLoader Loader;
        Result = Loader.LoadFBX(AssetInfo.SourceFilePath);

        QueryPerformanceCounter(&EndTime);

        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        FbxLoadTime += (static_cast<double>(EndTime.QuadPart - StartTime.QuadPart) * 1000.f / static_cast<double>(Frequency.QuadPart));
    }

    // 로드된 에셋 등록
    AddToAssetMap(Result, BaseName, AssetInfo);

    if (!bIsBinaryValid)
    {
        // 바이너리 작성
        SaveFbxBinary(BinFilePath, Result, BaseName, FolderPath);
    }
}

void UAssetManager::AddToAssetMap(const FAssetLoadResult& Result, const FString& BaseName, const FAssetInfo& BaseAssetInfo)
{
    for (int32 i = 0; i < Result.Skeletons.Num(); ++i)
    {
        USkeleton* Skeleton = Result.Skeletons[i];
        FString BaseAssetName = BaseName + "_Skeleton";

        FAssetInfo Info = BaseAssetInfo;
        Info.AssetName = i > 0 ? FName(BaseAssetName + FString::FromInt(i)) : FName(BaseAssetName);
        Info.AssetType = EAssetType::Skeleton;
        
        FString Key = Info.GetFullPath();
        AssetRegistry->PathNameToAssetInfo.Add(Key, Info);

        SkeletonMap.Add(Key, Skeleton);
    }

    for (int32 i = 0; i < Result.SkeletalMeshes.Num(); ++i)
    {
        USkeletalMesh* SkeletalMesh = Result.SkeletalMeshes[i];
        FString BaseAssetName = BaseName;

        FAssetInfo Info = BaseAssetInfo;
        Info.AssetName = i > 0 ? FName(BaseAssetName + FString::FromInt(i)) : FName(BaseAssetName);
        Info.AssetType = EAssetType::SkeletalMesh;
        
        FString Key = Info.GetFullPath();
        AssetRegistry->PathNameToAssetInfo.Add(Key, Info);

        SkeletalMeshMap.Add(Key, SkeletalMesh);
    }

    for (int32 i = 0; i < Result.StaticMeshes.Num(); ++i)
    {
        UStaticMesh* StaticMesh = Result.StaticMeshes[i];
        FString BaseAssetName = BaseName;

        FAssetInfo Info = BaseAssetInfo;
        Info.AssetName = i > 0 ? FName(BaseAssetName + FString::FromInt(i)) : FName(BaseAssetName);
        Info.AssetType = EAssetType::StaticMesh;
        
        FString Key = Info.GetFullPath();
        AssetRegistry->PathNameToAssetInfo.Add(Key, Info);

        StaticMeshMap.Add(Key, StaticMesh);
    }

    for (int32 i = 0; i < Result.Materials.Num(); ++i)
    {
        UMaterial* Material = Result.Materials[i];
        FString BaseAssetName = Material->GetName();

        FAssetInfo Info = BaseAssetInfo;
        Info.AssetName = FName(BaseAssetName);
        Info.AssetType = EAssetType::Material;
        
        FString Key = Info.GetFullPath();
        AssetRegistry->PathNameToAssetInfo.Add(Key, Info);

        MaterialMap.Add(Key, Material);
    }

    for (int32 i = 0; i < Result.Animations.Num(); ++i)
    {
        UAnimationAsset* Animation = Result.Animations[i];
        FString BaseAssetName = Animation->GetName();

        FAssetInfo Info = BaseAssetInfo;
        Info.AssetName = FName(BaseAssetName);
        Info.AssetType = EAssetType::Animation;
        
        FString Key = Info.GetFullPath();
        AssetRegistry->PathNameToAssetInfo.Add(Key, Info);

        AnimationMap.Add(Key, Animation);
    }
}

bool UAssetManager::LoadFbxBinary(const FString& FilePath, FAssetLoadResult& Result, const FString& BaseName, const FString& FolderPath)
{
    std::filesystem::path Path = FilePath.ToWideString();

    TArray<uint8> LoadData;
    {
        std::ifstream InputStream{ Path, std::ios::binary | std::ios::ate };
        if (!InputStream.is_open())
        {
            return false;
        }

        const std::streamsize FileSize = InputStream.tellg();
        if (FileSize < 0)
        {
            // Error getting size
            InputStream.close();
            return false;
        }
        if (FileSize == 0)
        {
            // Empty file is valid
            InputStream.close();
            return false; // Buffer remains empty
        }

        InputStream.seekg(0, std::ios::beg);

        LoadData.SetNum(static_cast<int32>(FileSize));
        InputStream.read(reinterpret_cast<char*>(LoadData.GetData()), FileSize);

        if (InputStream.fail() || InputStream.gcount() != FileSize)
        {
            return false;
        }
        InputStream.close();
    }

    FMemoryReader Reader(LoadData);

    SerializeVersion(Reader);
    SerializeAssetLoadResult(Reader, Result, BaseName, FolderPath);

    return true;
}

bool UAssetManager::SaveFbxBinary(const FString& FilePath, FAssetLoadResult& Result, const FString& BaseName, const FString& FolderPath)
{
    std::filesystem::path Path = FilePath.ToWideString();

    TArray<uint8> SaveData;
    FMemoryWriter Writer(SaveData);

    SerializeVersion(Writer);
    bool bSerialized = SerializeAssetLoadResult(Writer, Result, BaseName, FolderPath);
    if (!bSerialized)
    {
        return false;
    }

    std::ofstream OutputStream{ Path, std::ios::binary | std::ios::trunc };
    if (!OutputStream.is_open())
    {
        return false;
    }

    if (SaveData.Num() > 0)
    {
        OutputStream.write(reinterpret_cast<const char*>(SaveData.GetData()), SaveData.Num());

        if (OutputStream.fail())
        {
            return false;
        }
    }

    OutputStream.close();
    return true;
}

bool UAssetManager::SerializeVersion(FArchive& Ar)
{
    if (Ar.IsSaving())
    {
        Ar << Version;
        return true;
    }

    uint32 FileVersion;
    Ar << FileVersion;

    if (FileVersion != Version)
    {
        UE_LOGFMT(ELogLevel::Error, "MeshAsset version mismatch: {} != {}", FileVersion, Version);
        return false;
    }

    return true;
}

bool UAssetManager::SerializeAssetLoadResult(FArchive& Ar, FAssetLoadResult& Result, const FString& BaseName, const FString& FolderPath)
{
    uint8 MaterialNum = 0;
    uint8 SkeletonNum = 0;
    uint8 SkeletalMeshNum = 0;
    uint8 StaticMeshNum = 0;
    uint8 AnimationNum = 0;

    if (Ar.IsSaving())
    {
        MaterialNum = Result.Materials.Num();
        SkeletonNum = Result.Skeletons.Num();
        SkeletalMeshNum = Result.SkeletalMeshes.Num();
        StaticMeshNum = Result.StaticMeshes.Num();
        AnimationNum = Result.Animations.Num();
    }

    Ar << MaterialNum << SkeletonNum << SkeletalMeshNum << StaticMeshNum << AnimationNum;

    // Load 과정에서 Key를 통해 오브젝트를 찾기 위한 맵
    TMap<FName, USkeleton*> TempSkeletonMap;
    TMap<FName, UMaterial*> TempMaterialMap;

    for (int i = 0; i < MaterialNum; ++i)
    {
        FAssetInfo Info;
        if (Ar.IsSaving())
        {
            FName Key = FolderPath / Result.Materials[i]->GetName();
            if (AssetRegistry->PathNameToAssetInfo.Contains(Key))
            {
                Info = AssetRegistry->PathNameToAssetInfo[Key];
            }
            else
            {
                return false;
            }
        }
        Info.Serialize(Ar);

        UMaterial* Material = nullptr;
        if (Ar.IsLoading())
        {
            Material = FObjectFactory::ConstructObject<UMaterial>(nullptr, Info.AssetName);
            Result.Materials.Add(Material);
        }
        else
        {
            Material = Result.Materials[i];
        }

        Material->SerializeAsset(Ar);

        if (Ar.IsLoading())
        {
            TempMaterialMap.Add(Info.GetFullPath(), Material);

            // 텍스처 렌더 리소스 생성
            for (const FTextureInfo& Info : Material->GetMaterialInfo().TextureInfos)
            {
                FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Info.TexturePath.c_str(), Info.bIsSRGB);
            }
        }
    }

    for (int i = 0; i < SkeletonNum; ++i)
    {
        FAssetInfo Info;
        if (Ar.IsSaving())
        {
            FString BaseAssetName = BaseName + "_Skeleton";
            FName Key = FolderPath / (i > 0 ? BaseAssetName + FString::FromInt(i) : BaseAssetName);
            if (AssetRegistry->PathNameToAssetInfo.Contains(Key))
            {
                Info = AssetRegistry->PathNameToAssetInfo[Key];
            }
            else
            {
                return false;
            }
        }
        Info.Serialize(Ar);

        USkeleton* Skeleton = nullptr;
        if (Ar.IsLoading())
        {
            Skeleton = FObjectFactory::ConstructObject<USkeleton>(nullptr, Info.AssetName);
            Result.Skeletons.Add(Skeleton);
        }
        else
        {
            Skeleton = Result.Skeletons[i];
        }
        
        Skeleton->SerializeAsset(Ar);

        if (Ar.IsLoading())
        {
            TempSkeletonMap.Add(Info.GetFullPath(), Skeleton);
        }
    }

    for (int i = 0; i < SkeletalMeshNum; ++i)
    {
        FAssetInfo Info;
        if (Ar.IsSaving())
        {
            FName Key = FolderPath / (i > 0 ? BaseName + FString::FromInt(i) : BaseName);
            if (AssetRegistry->PathNameToAssetInfo.Contains(Key))
            {
                Info = AssetRegistry->PathNameToAssetInfo[Key];
            }
            else
            {
                return false;
            }
        }
        Info.Serialize(Ar);

        USkeletalMesh* SkeletalMesh = nullptr;
        FName SkeletonKey;
        TArray<FName> MaterialKeys;
        if (Ar.IsLoading())
        {
            SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
            Result.SkeletalMeshes.Add(SkeletalMesh);
        }
        else
        {
            SkeletalMesh = Result.SkeletalMeshes[i];

            // SkeletonMap의 Value와 비교하면서 Key를 찾고 저장.
            // Key를 저장하면, binary 파일을 읽을 때 Key를 통해 원하는 오브젝트를 찾을 수 있음.
            for (const auto& [Key, Object] : SkeletonMap)
            {
                if (Object == SkeletalMesh->GetSkeleton())
                {
                    SkeletonKey = Key;
                    break;
                }
            }

            for (const FMaterialInfo& Mat : SkeletalMesh->GetRenderData()->Materials)
            {
                MaterialKeys.Add(FolderPath / Mat.MaterialName);
            }
        }

        Ar << SkeletonKey << MaterialKeys;
        SkeletalMesh->SerializeAsset(Ar);

        if (Ar.IsLoading())
        {
            // 스켈레톤 지정
            USkeleton* Skeleton = nullptr;
            for (const auto& [Key, Object] : TempSkeletonMap)
            {
                if (Key == SkeletonKey)
                {
                    Skeleton = Object;
                    break;
                }
            }

            if (Skeleton)
            {
                SkeletalMesh->SetSkeleton(Skeleton);
            }
            else
            {
                UE_LOG(ELogLevel::Error, "Failed assign skeleton to {}", SkeletalMesh->GetName());
            }

            /**
             * TODO: 스켈레탈 메시의 머티리얼은 FMaterialInfo로 관리되고 있으므로, USkeletalMesh::SerializeAsset에서 이미 읽어 옴.
             *       나중에 FMaterialInfo 대신 UMaterial을 참조하여 사용한다면 여기에서 UMatrial을 찾는 코드를 작성해야 함.
             *       스켈레톤 찾는 방법 참고.
             */
        }
    }

    for (int i = 0; i < StaticMeshNum; ++i)
    {
        FAssetInfo Info;
        if (Ar.IsSaving())
        {
            FName Key = FolderPath / (i > 0 ? BaseName + FString::FromInt(i) : BaseName);
            if (AssetRegistry->PathNameToAssetInfo.Contains(Key))
            {
                Info = AssetRegistry->PathNameToAssetInfo[Key];
            }
            else
            {
                return false;
            }
        }
        Info.Serialize(Ar);

        UStaticMesh* StaticMesh = nullptr;
        TArray<FName> MaterialKeys;
        if (Ar.IsLoading())
        {
            StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr);
            Result.StaticMeshes.Add(StaticMesh);
        }
        else
        {
            StaticMesh = Result.StaticMeshes[i];

            for (const FMaterialInfo& Mat : StaticMesh->GetRenderData()->Materials)
            {
                MaterialKeys.Add(FolderPath / Mat.MaterialName);
            }
        }

        Ar << MaterialKeys;
        StaticMesh->SerializeAsset(Ar);

        if (Ar.IsLoading())
        {
            /**
             * TODO: 스태틱 메시의 머티리얼은 FMaterialInfo로 관리되고 있으므로, UStaticMesh::SerializeAsset에서 이미 읽어 옴.
             *       나중에 FMaterialInfo 대신 UMaterial을 참조하여 사용한다면 여기에서 UMatrial을 찾는 코드를 작성해야 함.
             *       스켈레톤 찾는 방법 참고.
             */
        }
    }

    for (int i = 0; i < AnimationNum; ++i)
    {
        FAssetInfo Info;
        if (Ar.IsSaving())
        {
            FName Key = FolderPath / Result.Animations[i]->GetName();
            if (AssetRegistry->PathNameToAssetInfo.Contains(Key))
            {
                Info = AssetRegistry->PathNameToAssetInfo[Key];
            }
            else
            {
                return false;
            }
        }
        Info.Serialize(Ar);

        UAnimationAsset* Animation = nullptr;
        FName SkeletonKey;
        if (Ar.IsLoading())
        {
            Animation = FObjectFactory::ConstructObject<UAnimSequence>(nullptr, Info.AssetName);
            Result.Animations.Add(Animation);
        }
        else
        {
            Animation = Result.Animations[i];

            // SkeletonMap의 Value와 비교하면서 Key를 찾고 저장.
            // Key를 저장하면, binary 파일을 읽을 때 Key를 통해 원하는 오브젝트를 찾을 수 있음.
            for (const auto& [Key, Object] : SkeletonMap)
            {
                if (Object == Animation->GetSkeleton())
                {
                    SkeletonKey = Key;
                    break;
                }
            }
        }

        Ar << SkeletonKey;

        if (Ar.IsLoading())
        {
            // 스켈레톤 지정
            USkeleton* Skeleton = nullptr;
            for (const auto& [Key, Object] : TempSkeletonMap)
            {
                if (Key == SkeletonKey)
                {
                    Skeleton = Object;
                    break;
                }
            }

            if (Skeleton)
            {
                Animation->SetSkeleton(Skeleton);
            }
            else
            {
                UE_LOG(ELogLevel::Error, "Failed assign skeleton to {}", Animation->GetName());
            }
        }

        Animation->SerializeAsset(Ar);
    }

    return true;
}
