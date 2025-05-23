#include "PropertyEditorPanel.h"

#include <algorithm>
#include <filesystem>
#include <string>
//#include <windows.h>
//#include <tchar.h>

#include "World/World.h"
#include "Actors/Player.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleModuleRequired.h"
#include "Components/ParticleSystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/AmbientLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/FObjLoader.h"
#include "UnrealEd/ImGuiWidget.h"
#include "UObject/ObjectFactory.h"
#include "Engine/Engine.h"
#include "Components/HeightFogComponent.h"
#include "Components/ProjectileMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Contents/AnimInstance/MyAnimInstance.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "GameFramework/SpringArmComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Math/JungleMath.h"
#include "Renderer/ShadowManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"
#include "LuaScripts/LuaScriptComponent.h"
#include "LuaScripts/LuaScriptFileUtils.h"
#include "imgui/imgui_bezier.h"
#include "imgui/imgui_curve.h"
#include "Math/Transform.h"
#include "Animation/AnimStateMachine.h"
#include "Runtime/CoreUObject/UObject/Property.h"
#include "Actors/ParticleSystemActor.h"
#include "Engine/Classes/Engine/ResourceMgr.h"
#include "Define.h"
#include "Particles/TypeData/ParticleModuleTypeDataMesh.h"

void PropertyEditorPanel::Render()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }
    
    /* Pre Setup */
    float PanelWidth;
    float PanelHeight;

    float PanelPosX;
    float PanelPosY;

    if (Engine->ActiveWorld)
    {
        if (Engine->ActiveWorld->WorldType == EWorldType::Editor or Engine->ActiveWorld->WorldType == EWorldType::PIE)
        {
            PanelWidth = (Width) * 0.2f - 5.0f;
            PanelHeight = (Height)-((Height) * 0.3f + 10.0f) - 32.0f;

            PanelPosX = (Width) * 0.8f + 4.0f;
            PanelPosY = (Height) * 0.3f + 10.0f;
        }
        else if (Engine->ActiveWorld->WorldType == EWorldType::EditorPreview)
        {
            if (GetPreviewType() == EPreviewTypeBitFlag::SkeletalMesh)
            {
                PanelWidth = (Width) * 0.2f - 5.0f;
                PanelHeight = (Height)-((Height) * 0.3f + 10.0f) - 32.0f;

                PanelPosX = (Width) * 0.8f + 4.0f;
                PanelPosY = (Height) * 0.3f + 10.0f;
            }
            else if (GetPreviewType() == EPreviewTypeBitFlag::ParticleSystem)
            {
                PanelWidth = (Width) * 0.3f;
                PanelHeight = (Height) * 0.35f;

                PanelPosX = 0.f;
                PanelPosY = Height * 0.65f;
            }
            else
            {
                return;
            }
        }
    }

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    /* Render Start */
    ImGui::Begin("Detail", nullptr, PanelFlags);

    // 파티클 시스템 프리뷰 모드인 경우 파티클 속성 렌더링
    if (Engine->ActiveWorld && Engine->ActiveWorld->WorldType == EWorldType::EditorPreview &&
        GetPreviewType() == EPreviewTypeBitFlag::ParticleSystem && CurrentParticleSystemComponent)
    {
        // 파티클 시스템 자체가 선택된 경우
        if (CurrentParticleSelection.SelectedEmitter == nullptr && CurrentParticleSelection.SelectedModule == nullptr)
        {
            UParticleSystem* ParticleSystem = CurrentParticleSystemComponent->Template;
            if (ParticleSystem)
            {
                RenderForParticleSystem(ParticleSystem);
            }
        }
        // 에미터가 선택된 경우
        else if (CurrentParticleSelection.SelectedEmitter != nullptr && CurrentParticleSelection.SelectedModule == nullptr)
        {
            RenderForParticleEmitter(CurrentParticleSelection.SelectedEmitter);
        }
        // 모듈이 선택된 경우
        else if (CurrentParticleSelection.SelectedModule != nullptr)
        {
            RenderForParticleModule(CurrentParticleSelection.SelectedModule);
        }

        ImGui::End();
        return;
    }

    // 에디터 모드에서 일반 속성 렌더링 (기존 코드)
    AActor* SelectedActor = Engine->GetSelectedActor();
    USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
    USceneComponent* TargetComponent = nullptr;

    if (SelectedComponent != nullptr)
    {
        TargetComponent = SelectedComponent;
    }
    else if (SelectedActor != nullptr)
    {
        TargetComponent = SelectedActor->GetRootComponent();
    }

    if (TargetComponent != nullptr)
    {
        AEditorPlayer* Player = Engine->GetEditorPlayer();
        RenderForSceneComponent(TargetComponent, Player);
    }
    if (SelectedActor)
    {
        RenderForActor(SelectedActor, TargetComponent);

        if (ASequencerPlayer* SP = Cast<ASequencerPlayer>(SelectedActor))
        {
            FString Label = SP->Socket.ToString();
            if (ImGui::InputText("##Socket", GetData(Label), 256))
            {
                SP->Socket = Label;
            }

            if (ImGui::BeginCombo("##Parent", "Parent", ImGuiComboFlags_None))
            {
                for (auto It : TObjectRange<USkeletalMeshComponent>())
                {
                    if (ImGui::Selectable(GetData(It->GetName()), false))
                    {
                        SP->SkeletalMeshComponent = It;
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
    
    if (UAmbientLightComponent* LightComponent = GetTargetComponent<UAmbientLightComponent>(SelectedActor, SelectedComponent))
    {
        RenderForAmbientLightComponent(LightComponent);
    }
    if (UDirectionalLightComponent* LightComponent = GetTargetComponent<UDirectionalLightComponent>(SelectedActor, SelectedComponent))
    {
        RenderForDirectionalLightComponent(LightComponent);
    }
    if (UPointLightComponent* LightComponent = GetTargetComponent<UPointLightComponent>(SelectedActor, SelectedComponent))
    {
        RenderForPointLightComponent(LightComponent);
    }
    if (USpotLightComponent* LightComponent = GetTargetComponent<USpotLightComponent>(SelectedActor, SelectedComponent))
    {
        RenderForSpotLightComponent(LightComponent);
    }
    if (UProjectileMovementComponent* ProjectileComp = GetTargetComponent<UProjectileMovementComponent>(SelectedActor, SelectedComponent))
    {
        RenderForProjectileMovementComponent(ProjectileComp);
    }
    if (UTextComponent* TextComp = GetTargetComponent<UTextComponent>(SelectedActor, SelectedComponent))
    {
        RenderForTextComponent(TextComp);
    }
    if (UStaticMeshComponent* StaticMeshComponent = GetTargetComponent<UStaticMeshComponent>(SelectedActor, SelectedComponent))
    {
        RenderForStaticMesh(StaticMeshComponent);
        RenderForMaterial(StaticMeshComponent);
    }
    if (USkeletalMeshComponent* SkeletalMeshComponent = GetTargetComponent<USkeletalMeshComponent>(SelectedActor, SelectedComponent))
    {
        RenderForSkeletalMesh(SkeletalMeshComponent);
    }
    if (UParticleSystemComponent* ParticleSystemComponent = GetTargetComponent<UParticleSystemComponent>(SelectedActor, SelectedComponent))
    {
        RenderForParticleSystemComponent(ParticleSystemComponent);
    }
    if (UHeightFogComponent* FogComponent = GetTargetComponent<UHeightFogComponent>(SelectedActor, SelectedComponent))
    {
        RenderForExponentialHeightFogComponent(FogComponent);
    }

    if (UCameraComponent* CameraComponent = GetTargetComponent<UCameraComponent>(SelectedActor, SelectedComponent))
    {
        RenderForCameraComponent(CameraComponent);
    }
  
    if (UShapeComponent* ShapeComponent = GetTargetComponent<UShapeComponent>(SelectedActor, SelectedComponent))
    {
        RenderForShapeComponent(ShapeComponent);
    }

    if (USpringArmComponent* SpringArmComponent = GetTargetComponent<USpringArmComponent>(SelectedActor, SelectedComponent))
    {
        RenderForSpringArmComponent(SpringArmComponent);
    }

    if (SelectedActor)
    {
        ImGui::Separator();
        const UClass* Class = SelectedActor->GetClass();

        for (; Class; Class = Class->GetSuperClass())
        {
            const TArray<FProperty*>& Properties = Class->GetProperties();
            if (!Properties.IsEmpty())
            {
                ImGui::SeparatorText(*Class->GetName());
            }

            for (const FProperty* Prop : Properties)
            {
                Prop->DisplayInImGui(SelectedActor);
            }
        }
    }

    if (SelectedComponent)
    {
        ImGui::Separator();
        const UClass* Class = GetTargetComponent<USceneComponent>(SelectedActor, SelectedComponent)->GetClass();

        for (; Class; Class = Class->GetSuperClass())
        {
            const TArray<FProperty*>& Properties = Class->GetProperties();
            if (!Properties.IsEmpty())
            {
                ImGui::SeparatorText(*Class->GetName());
            }

            for (const FProperty* Prop : Properties)
            {
                Prop->DisplayInImGui(SelectedComponent);
            }
        }
    }

    ImGui::End();
}

void PropertyEditorPanel::RGBToHSV(const float R, const float G, const float B, float& H, float& S, float& V)
{
    const float MX = FMath::Max(R, FMath::Max(G, B));
    const float MN = FMath::Min(R, FMath::Min(G, B));
    const float Delta = MX - MN;

    V = MX;

    if (MX == 0.0f) {
        S = 0.0f;
        H = 0.0f;
        return;
    }
    else {
        S = Delta / MX;
    }

    if (Delta < 1e-6) {
        H = 0.0f;
    }
    else {
        if (R >= MX) {
            H = (G - B) / Delta;
        }
        else if (G >= MX) {
            H = 2.0f + (B - R) / Delta;
        }
        else {
            H = 4.0f + (R - G) / Delta;
        }
        H *= 60.0f;
        if (H < 0.0f) {
            H += 360.0f;
        }
    }
}

void PropertyEditorPanel::HSVToRGB(const float H, const float S, const float V, float& R, float& G, float& B)
{
    // h: 0~360, s:0~1, v:0~1
    const float C = V * S;
    const float Hp = H / 60.0f;             // 0~6 구간
    const float X = C * (1.0f - fabsf(fmodf(Hp, 2.0f) - 1.0f));
    const float M = V - C;

    if (Hp < 1.0f) { R = C;  G = X;  B = 0.0f; }
    else if (Hp < 2.0f) { R = X;  G = C;  B = 0.0f; }
    else if (Hp < 3.0f) { R = 0.0f; G = C;  B = X; }
    else if (Hp < 4.0f) { R = 0.0f; G = X;  B = C; }
    else if (Hp < 5.0f) { R = X;  G = 0.0f; B = C; }
    else { R = C;  G = 0.0f; B = X; }

    R += M;  G += M;  B += M;
}


void PropertyEditorPanel::RenderForSceneComponent(USceneComponent* SceneComponent, AEditorPlayer* Player) const
{
    ImGui::SetItemDefaultFocus();
    // TreeNode 배경색을 변경 (기본 상태)
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        FTransform Transform = SceneComponent->GetRelativeTransform();
        FVector Location = Transform.GetTranslation();
        FRotator Rotation = Transform.GetRotation().Rotator();
        FVector Scale = Transform.GetScale3D();

        FImGuiWidget::DrawVec3Control("Location", Location, 0, 85);
        ImGui::Spacing();

        FImGuiWidget::DrawRot3Control("Rotation", Rotation, 0, 85);
        ImGui::Spacing();

        FImGuiWidget::DrawVec3Control("Scale", Scale, 1, 85);
        ImGui::Spacing();

        SceneComponent->SetRelativeTransform(FTransform(Rotation, Location, Scale));

        std::string CoordiButtonLabel;
        if (Player->GetCoordMode() == ECoordMode::CDM_WORLD)
        {
            CoordiButtonLabel = "World";
        }
        else if (Player->GetCoordMode() == ECoordMode::CDM_LOCAL)
        {
            CoordiButtonLabel = "Local";
        }

        if (ImGui::Button(CoordiButtonLabel.c_str(), ImVec2(ImGui::GetWindowContentRegionMax().x * 0.9f, 32)))
        {
            Player->AddCoordMode();
        }
         
        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForCameraComponent(UCameraComponent* InCameraComponent)
{
    
}

void PropertyEditorPanel::RenderForPlayerActor(APlayer* InPlayerActor)
{
    if (ImGui::Button("SetMainPlayer"))
    {
        GEngine->ActiveWorld->SetMainPlayer(InPlayerActor);
    }
}

void PropertyEditorPanel::RenderForActor(AActor* SelectedActor, USceneComponent* TargetComponent) const
{
    if (ImGui::Button("Duplicate"))
    {
        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
        AActor* NewActor = Engine->ActiveWorld->DuplicateActor(Engine->GetSelectedActor());
        Engine->SelectActor(NewActor);
        Engine->DeselectComponent(Engine->GetSelectedComponent());
    }
    
    FString BasePath = FString(L"LuaScripts\\");
    FString LuaDisplayPath;
    
    if (SelectedActor->GetComponentByClass<ULuaScriptComponent>())
    {
        LuaDisplayPath = SelectedActor->GetComponentByClass<ULuaScriptComponent>()->GetDisplayName();
        if (ImGui::Button("Edit Script"))
        {
            // 예: PickedActor에서 스크립트 경로를 받아옴
            if (auto* ScriptComp = SelectedActor->GetComponentByClass<ULuaScriptComponent>())
            {
                std::wstring ws = (BasePath + ScriptComp->GetDisplayName()).ToWideString();
                LuaScriptFileUtils::OpenLuaScriptFile(ws.c_str());
            }
        }
    }
    else
    {
        // Add Lua Script
        if (ImGui::Button("Create Script"))
        {
            // Lua Script Component 생성 및 추가
            ULuaScriptComponent* NewScript = SelectedActor->AddComponent<ULuaScriptComponent>();
            FString LuaFilePath = NewScript->GetScriptPath();
            std::filesystem::path FilePath = std::filesystem::path(GetData(LuaFilePath));
            
            try
            {
                std::filesystem::path Dir = FilePath.parent_path();
                if (!std::filesystem::exists(Dir))
                {
                    std::filesystem::create_directories(Dir);
                }

                std::ifstream luaTemplateFile(TemplateFilePath.ToWideString());

                std::ofstream file(FilePath);
                if (file.is_open())
                {
                    if (luaTemplateFile.is_open())
                    {
                        file << luaTemplateFile.rdbuf();
                    }
                    // 생성 완료
                    file.close();
                }
                else
                {
                    MessageBoxA(nullptr, "Failed to Create Script File for writing: ", "Error", MB_OK | MB_ICONERROR);
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                MessageBoxA(nullptr, "Failed to Create Script File for writing: ", "Error", MB_OK | MB_ICONERROR);
            }
            LuaDisplayPath = NewScript->GetDisplayName();
        }
    }
    ImGui::InputText("Script File", GetData(LuaDisplayPath), IM_ARRAYSIZE(*LuaDisplayPath),
        ImGuiInputTextFlags_ReadOnly);

    if (ImGui::TreeNodeEx("Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        ImGui::Text("Add");
        ImGui::SameLine();

        TArray<UClass*> CompClasses;
        GetChildOfClass(USceneComponent::StaticClass(), CompClasses);

        if (ImGui::BeginCombo("##AddComponent", "Components", ImGuiComboFlags_None))
        {
            for (UClass* Class : CompClasses)
            {
                if (ImGui::Selectable(GetData(Class->GetName()), false))
                {
                    USceneComponent* NewComp = Cast<USceneComponent>(SelectedActor->AddComponent(Class));
                    if (NewComp != nullptr && TargetComponent != nullptr)
                    {
                        NewComp->SetupAttachment(TargetComponent);
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
}

void PropertyEditorPanel::RenderForStaticMesh(UStaticMeshComponent* StaticMeshComp) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Static Mesh", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        ImGui::Text("StaticMesh");
        ImGui::SameLine();

        FString PreviewName = FString("None");
        if (UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh())
        {
            if (FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData())
            {
                PreviewName = RenderData->DisplayName;
            }
        }
        
        const TMap<FName, FAssetInfo> Assets = UAssetManager::Get().GetAssetRegistry();

        if (ImGui::BeginCombo("##StaticMesh", GetData(PreviewName), ImGuiComboFlags_None))
        {
            for (const auto& Asset : Assets)
            {
                if (Asset.Value.AssetType != EAssetType::StaticMesh)
                {
                    continue;
                }
                
                if (ImGui::Selectable(GetData(Asset.Value.AssetName.ToString()), false))
                {
                    FString MeshName = Asset.Value.PackagePath.ToString() + "/" + Asset.Value.AssetName.ToString();
                    UStaticMesh* StaticMesh = FObjManager::GetStaticMesh(MeshName.ToWideString());
                    if (!StaticMesh)
                    {
                        StaticMesh = UAssetManager::Get().GetStaticMesh(MeshName);
                    }

                    if (StaticMesh)
                    {
                        StaticMeshComp->SetStaticMesh(StaticMesh);
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComp) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Skeletal Mesh", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        ImGui::Text("SkeletalMesh");
        ImGui::SameLine();

        FString SelectedSkeletalMeshName = FString("None");
        if (USkeletalMesh* SkeletalMesh = SkeletalMeshComp->GetSkeletalMeshAsset())
        {
            if (const FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetRenderData())
            {
                SelectedSkeletalMeshName = RenderData->DisplayName;
            }
        }
        
        const TMap<FName, FAssetInfo> SkeletalMeshAssets = UAssetManager::Get().GetAssetRegistry();

        if (ImGui::BeginCombo("##SkeletalMesh", GetData(SelectedSkeletalMeshName), ImGuiComboFlags_None))
        {
            for (const auto& Asset : SkeletalMeshAssets)
            {
                if (Asset.Value.AssetType != EAssetType::SkeletalMesh)
                {
                    continue;
                }
                
                if (ImGui::Selectable(GetData(Asset.Value.AssetName.ToString()), false))
                {
                    FString AssetName = Asset.Value.PackagePath.ToString() + "/" + Asset.Value.AssetName.ToString();
                    USkeletalMesh* SkeletalMesh = UAssetManager::Get().GetSkeletalMesh(FName(AssetName));
                    if (SkeletalMesh)
                    {
                        SkeletalMeshComp->SetSkeletalMeshAsset(SkeletalMesh);
                    }
                }
            }
            ImGui::EndCombo();
        }

        // Begin Animation
        ImGui::Text("Animation Mode");
        ImGui::SameLine();
        EAnimationMode CurrentAnimationMode = SkeletalMeshComp->GetAnimationMode();
        FString AnimModeStr = CurrentAnimationMode == EAnimationMode::AnimationBlueprint ? "Animation Instance" : "Animation Asset";
        if (ImGui::BeginCombo("##AnimationMode", GetData(AnimModeStr), ImGuiComboFlags_None))
        {
            if (ImGui::Selectable("Animation Instance", CurrentAnimationMode == EAnimationMode::AnimationBlueprint))
            {
                SkeletalMeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
                CurrentAnimationMode = EAnimationMode::AnimationBlueprint;
                SkeletalMeshComp->SetAnimClass(UClass::FindClass(FName("UMyAnimInstance")));
            }
            if (ImGui::Selectable("Animation Asset", CurrentAnimationMode == EAnimationMode::AnimationSingleNode))
            {
                SkeletalMeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                CurrentAnimationMode = EAnimationMode::AnimationSingleNode;
            }
            ImGui::EndCombo();
        }

        if (CurrentAnimationMode == EAnimationMode::AnimationBlueprint)
        {
            TArray<UClass*> CompClasses;
            GetChildOfClass(UAnimInstance::StaticClass(), CompClasses);
            static int SelectedIndex = 0;

            // AnimInstance 목록 텍스트
            ImGui::Text("AnimInstance List");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##AnimInstance List Combo", *CompClasses[SelectedIndex]->GetName()))
            {
                for (int i = 0; i < CompClasses.Num(); ++i)
                {
                    if (CompClasses[i] == UAnimInstance::StaticClass() || CompClasses[i] == UAnimSingleNodeInstance::StaticClass())
                    {
                        continue;
                    }
                    bool bIsSelected = (SelectedIndex == i);
                    if (ImGui::Selectable(*CompClasses[i]->GetName(), bIsSelected))
                    {
                        SelectedIndex = i;
                    }
                    if (bIsSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (CompClasses.IsValidIndex(SelectedIndex))
            {
                UClass* SelectedClass = CompClasses[SelectedIndex];
                UMyAnimInstance* AnimInstance = Cast<UMyAnimInstance>(SkeletalMeshComp->GetAnimInstance());

                bool bPlaying = AnimInstance->IsPlaying();
                if (ImGui::Checkbox("Playing", &bPlaying))
                {
                    if (bPlaying)
                    {
                        AnimInstance->SetPlaying(true);
                    }
                    else
                    {
                        AnimInstance->SetPlaying(false);
                    }
                }
                if (AnimInstance && AnimInstance->GetClass()->IsChildOf(SelectedClass))
                {                    
                    UAnimStateMachine* AnimStateMachine = AnimInstance->GetStateMachine();
                    if(ImGui::Button("MoveFast"))
                    {
                        AnimStateMachine->MoveFast();
                    }
                    ImGui::SameLine();
                    if(ImGui::Button("MoveSlow"))
                    {
                        AnimStateMachine->MoveSlow();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Dance"))
                    {
                        AnimStateMachine->Dance();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("StopDance"))
                    {
                        AnimStateMachine->StopDance();
                    }
                    
                    AnimInstance->SetAnimState(AnimStateMachine->GetState());
                }
            }
        }
        else
        {
            FString SelectedAnimationName = FString("None");
            if (UAnimationAsset* Animation = SkeletalMeshComp->GetAnimation())
            {
                SelectedAnimationName = Animation->GetName();
            }
        
            const TMap<FName, FAssetInfo> AnimationAssets = UAssetManager::Get().GetAssetRegistry();

            ImGui::Text("Anim To Play");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##Animation", GetData(SelectedAnimationName), ImGuiComboFlags_None))
            {
                if (ImGui::Selectable("None", false))
                {
                    SkeletalMeshComp->SetAnimation(nullptr);
                }
                
                for (const auto& Asset : AnimationAssets)
                {
                    if (Asset.Value.AssetType != EAssetType::Animation)
                    {
                        continue;
                    }
                
                    if (ImGui::Selectable(GetData(Asset.Value.AssetName.ToString()), false))
                    {
                        FString AssetName = Asset.Value.PackagePath.ToString() + "/" + Asset.Value.AssetName.ToString();

                        UAnimationAsset* Animation = UAssetManager::Get().GetAnimation(FName(AssetName));
                        UAnimSequence* AnimSeq = nullptr;
                    
                        if (Animation)
                        {
                            AnimSeq = Cast<UAnimSequence>(Animation);
                        }

                        if (AnimSeq)
                        {
                            const bool bWasLooping = SkeletalMeshComp->IsLooping();
                            const bool bWasPlaying = SkeletalMeshComp->IsPlaying();

                            SkeletalMeshComp->SetAnimation(AnimSeq);
                            
                            SkeletalMeshComp->SetLooping(bWasLooping);
                            if (bWasPlaying)
                            {
                                SkeletalMeshComp->Play(bWasLooping);
                            }
                            else
                            {
                                SkeletalMeshComp->Stop();
                            }
                        }
                    }
                }
                ImGui::EndCombo();
            }

            bool bLooping = SkeletalMeshComp->IsLooping();
            if (ImGui::Checkbox("Looping", &bLooping))
            {
                SkeletalMeshComp->SetLooping(bLooping);
            }

            bool bPlaying = SkeletalMeshComp->IsPlaying();
            if (ImGui::Checkbox("Playing", &bPlaying))
            {
                if (bPlaying)
                {
                    SkeletalMeshComp->Play(bLooping);
                }
                else
                {
                    SkeletalMeshComp->Stop();
                }
            }
        }

        // End Animation

        if (ImGui::Button("Open Viewer"))
        {
            UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
            if (!Engine)
            {
                return;
            }
            if (SkeletalMeshComp->GetSkeletalMeshAsset())
            {
                Engine->StartSkeletalMeshViewer(FName(SkeletalMeshComp->GetSkeletalMeshAsset()->GetRenderData()->ObjectName), SkeletalMeshComp->GetAnimation());
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForAmbientLightComponent(UAmbientLightComponent* AmbientLightComponent) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    if (ImGui::TreeNodeEx("AmbientLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawColorProperty("Light Color",
            [&]() { return AmbientLightComponent->GetLightColor(); },
            [&](FLinearColor c) { AmbientLightComponent->SetLightColor(c); });
        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForDirectionalLightComponent(UDirectionalLightComponent* DirectionalLightComponent) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    if (ImGui::TreeNodeEx("DirectionalLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawColorProperty("Light Color",
            [&]() { return DirectionalLightComponent->GetLightColor(); },
            [&](FLinearColor c) { DirectionalLightComponent->SetLightColor(c); });

        float Intensity = DirectionalLightComponent->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &Intensity, 0.0f, 150.0f, "%.1f"))
        {
            DirectionalLightComponent->SetIntensity(Intensity);
        }

        FVector LightDirection = DirectionalLightComponent->GetDirection();
        FImGuiWidget::DrawVec3Control("Direction", LightDirection, 0, 85);


        // --- Cast Shadows 체크박스 추가 ---
        bool bCastShadows = DirectionalLightComponent->GetCastShadows(); // 현재 상태 가져오기
        if (ImGui::Checkbox("Cast Shadows", &bCastShadows)) // 체크박스 UI 생성 및 상호작용 처리
        {
            DirectionalLightComponent->SetCastShadows(bCastShadows); // 변경된 상태 설정
            // 필요하다면, 상태 변경에 따른 즉각적인 렌더링 업데이트 요청 로직 추가
            // 예: PointlightComponent->MarkRenderStateDirty();
        }
        ImGui::Text("ShadowMap");

        // 분할된 개수만큼 CSM 해당 SRV 출력
        const uint32& NumCascades = FEngineLoop::Renderer.ShadowManager->GetNumCasCades();
        for (uint32 i = 0; i < NumCascades; ++i)
        {
            ImGui::Image(reinterpret_cast<ImTextureID>(FEngineLoop::Renderer.ShadowManager->GetDirectionalShadowCascadeDepthRHI()->ShadowSRVs[i]), ImVec2(200, 200));
            //ImGui::SameLine();
        }
        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForPointLightComponent(UPointLightComponent* PointlightComponent) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    if (ImGui::TreeNodeEx("PointLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawColorProperty("Light Color",
            [&]() { return PointlightComponent->GetLightColor(); },
            [&](FLinearColor c) { PointlightComponent->SetLightColor(c); });

        float Intensity = PointlightComponent->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &Intensity, 0.0f, 160.0f, "%.1f"))
        {
            PointlightComponent->SetIntensity(Intensity);
        }

        float Radius = PointlightComponent->GetRadius();
        if (ImGui::SliderFloat("Radius", &Radius, 0.01f, 200.f, "%.1f"))
        {
            PointlightComponent->SetRadius(Radius);
        }
        // --- Cast Shadows 체크박스 추가 ---
        bool bCastShadows = PointlightComponent->GetCastShadows(); // 현재 상태 가져오기
        if (ImGui::Checkbox("Cast Shadows", &bCastShadows)) // 체크박스 UI 생성 및 상호작용 처리
        {
            PointlightComponent->SetCastShadows(bCastShadows); // 변경된 상태 설정
            // 필요하다면, 상태 변경에 따른 즉각적인 렌더링 업데이트 요청 로직 추가
            // 예: PointlightComponent->MarkRenderStateDirty();
        }

        ImGui::Text("ShadowMap");

        FShadowCubeMapArrayRHI* pointRHI = FEngineLoop::Renderer.ShadowManager->GetPointShadowCubeMapRHI();
        const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
        float imageSize = 128.0f;
        int index =  PointlightComponent->GetPointLightInfo().ShadowMapArrayIndex;
        // CubeMap이므로 6개의 ShadowMap을 그립니다.
        for (int i = 0; i < 6; ++i)
        {
            ID3D11ShaderResourceView* faceSRV = pointRHI->ShadowFaceSRVs[index][i];
            if (faceSRV)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(faceSRV), ImVec2(imageSize, imageSize));
                ImGui::SameLine(); 
                ImGui::Text("%s", faceNames[i]);
            }
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForSpotLightComponent(USpotLightComponent* SpotLightComponent) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    if (ImGui::TreeNodeEx("SpotLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawColorProperty("Light Color",
            [&]() { return SpotLightComponent->GetLightColor(); },
            [&](FLinearColor c) { SpotLightComponent->SetLightColor(c); });

        float Intensity = SpotLightComponent->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &Intensity, 0.0f, 160.0f, "%.1f"))
        {
            SpotLightComponent->SetIntensity(Intensity);
        }

        float Radius = SpotLightComponent->GetRadius();
        if (ImGui::SliderFloat("Radius", &Radius, 0.01f, 200.f, "%.1f"))
        {
            SpotLightComponent->SetRadius(Radius);
        }

        FVector LightDirection = SpotLightComponent->GetDirection();
        FImGuiWidget::DrawVec3Control("Direction", LightDirection, 0, 85);

        float InnerConeAngle = SpotLightComponent->GetInnerDegree();
        float OuterConeAngle = SpotLightComponent->GetOuterDegree();
        if (ImGui::DragFloat("InnerConeAngle", &InnerConeAngle, 0.5f, 0.0f, 80.0f))
        {
            OuterConeAngle = std::max(InnerConeAngle, OuterConeAngle);

            SpotLightComponent->SetInnerDegree(InnerConeAngle);
            SpotLightComponent->SetOuterDegree(OuterConeAngle);
        }

        if (ImGui::DragFloat("OuterConeAngle", &OuterConeAngle, 0.5f, 0.0f, 80.0f))
        {
            InnerConeAngle = std::min(OuterConeAngle, InnerConeAngle);

            SpotLightComponent->SetOuterDegree(OuterConeAngle);
            SpotLightComponent->SetInnerDegree(InnerConeAngle);
        }

        // --- Cast Shadows 체크박스 추가 ---
        bool bCastShadows = SpotLightComponent->GetCastShadows(); // 현재 상태 가져오기
        if (ImGui::Checkbox("Cast Shadows", &bCastShadows)) // 체크박스 UI 생성 및 상호작용 처리
        {
            SpotLightComponent->SetCastShadows(bCastShadows); // 변경된 상태 설정
            // 필요하다면, 상태 변경에 따른 즉각적인 렌더링 업데이트 요청 로직 추가
            // 예: PointlightComponent->MarkRenderStateDirty();
        }

        ImGui::Text("ShadowMap");
        ImGui::Image(reinterpret_cast<ImTextureID>(FEngineLoop::Renderer.ShadowManager->GetSpotShadowDepthRHI()->ShadowSRVs[0]), ImVec2(200, 200));

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForLightCommon(ULightComponentBase* LightComponent) const
{
    const auto Engine = Cast<UEditorEngine>(GEngine);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    // --- "Override Camera" 버튼 추가 ---
    if (ImGui::Button("Override Camera with Light's Perspective"))
    {
        // 1. 라이트의 월드 위치 및 회전 가져오기
        FVector LightLocation = LightComponent->GetComponentLocation();

        FVector Forward = FVector(1.f, 0.f, 0.0f);
        Forward = JungleMath::FVectorRotate(Forward, LightLocation);
        FVector LightForward = Forward;
        FRotator LightRotation = LightComponent->GetComponentRotation();
        FVector LightRotationVector;
        LightRotationVector.X = LightRotation.Roll;
        LightRotationVector.Y = -LightRotation.Pitch;
        LightRotationVector.Z = LightRotation.Yaw;

        // 2. 활성 에디터 뷰포트 클라이언트 가져오기 (!!! 엔진별 구현 필요 !!!)
        std::shared_ptr<FEditorViewportClient> ViewportClient = Engine->GetLevelEditor()->GetActiveViewportClient(); // 위에 정의된 헬퍼 함수 사용 (또는 직접 구현)

        // 3. 뷰포트 클라이언트가 유효하면 카메라 설정
        if (ViewportClient)
        {
            ViewportClient->PerspectiveCamera.SetLocation(LightLocation + LightForward); // 카메라 위치 설정 함수 호출
            ViewportClient->PerspectiveCamera.SetRotation(LightRotationVector); // 카메라 회전 설정 함수 호출

            // 필요시 뷰포트 강제 업데이트/다시 그리기 호출
            // ViewportClient->Invalidate();
        }
        else
        {
            // 뷰포트 클라이언트를 찾을 수 없음 (오류 로그 등)
            // UE_LOG(LogTemp, Warning, TEXT("Active Viewport Client not found."));
        }
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForProjectileMovementComponent(UProjectileMovementComponent* ProjectileComp) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    if (ImGui::TreeNodeEx("Projectile Movement Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        float InitialSpeed = ProjectileComp->GetInitialSpeed();
        if (ImGui::InputFloat("InitialSpeed", &InitialSpeed, 0.f, 10000.0f, "%.1f"))
        {
            ProjectileComp->SetInitialSpeed(InitialSpeed);
        }

        float MaxSpeed = ProjectileComp->GetMaxSpeed();
        if (ImGui::InputFloat("MaxSpeed", &MaxSpeed, 0.f, 10000.0f, "%.1f"))
        {
            ProjectileComp->SetMaxSpeed(MaxSpeed);
        }

        float Gravity = ProjectileComp->GetGravity();
        if (ImGui::InputFloat("Gravity", &Gravity, 0.f, 10000.f, "%.1f"))
        {
            ProjectileComp->SetGravity(Gravity);
        }

        float ProjectileLifetime = ProjectileComp->GetLifetime();
        if (ImGui::InputFloat("Lifetime", &ProjectileLifetime, 0.f, 10000.f, "%.1f"))
        {
            ProjectileComp->SetLifetime(ProjectileLifetime);
        }

        FVector CurrentVelocity = ProjectileComp->GetVelocity();

        float Velocity[3] = { CurrentVelocity.X, CurrentVelocity.Y, CurrentVelocity.Z };

        if (ImGui::InputFloat3("Velocity", Velocity, "%.1f"))
        {
            ProjectileComp->SetVelocity(FVector(Velocity[0], Velocity[1], Velocity[2]));
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForTextComponent(UTextComponent* TextComponent) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Text Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        if (TextComponent)
        {
            TextComponent->SetTexture(L"Assets/Texture/font.png");
            TextComponent->SetRowColumnCount(106, 106);
            FWString wText = TextComponent->GetText();
            int Len = WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string u8Text(Len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, u8Text.data(), Len, nullptr, nullptr);

            static char Buf[256];
            strcpy_s(Buf, u8Text.c_str());

            ImGui::Text("Text: ", Buf);
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
            if (ImGui::InputText("##Text", Buf, 256, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                TextComponent->ClearText();
                int wLen = MultiByteToWideChar(CP_UTF8, 0, Buf, -1, nullptr, 0);
                FWString wNewText(wLen, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, Buf, -1, wNewText.data(), wLen);
                TextComponent->SetText(wNewText.c_str());
            }
            ImGui::PopItemFlag();
        }
        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForParticleSystemComponent(UParticleSystemComponent* ParticleSystemComp) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Particle", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        FName SelectedTemplateName = ParticleSystemComp->Template ? ParticleSystemComp->TemplateName : TEXT("None"); // 현재 선택된 템플릿 이름
        
        if (ImGui::BeginCombo("##Template", *SelectedTemplateName.ToString(), ImGuiComboFlags_None))
        {
            TMap<FName, UParticleSystem*> ParticleTemplates = UAssetManager::Get().GetParticleTemplateMap();
            for (const auto& Asset : ParticleTemplates)
            {
                bool isSelected = (SelectedTemplateName == Asset.Key);
                if (ImGui::Selectable(*Asset.Key.ToString(), isSelected))
                {
                    SelectedTemplateName = Asset.Key;
                    // 추가로, 선택된 파티클 에셋을 변수에 할당하거나 처리
                    ParticleSystemComp->Template = Asset.Value;
                    ParticleSystemComp->TemplateName = Asset.Key;
                    ParticleSystemComp->UpdateComponent();
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    
        if (ImGui::Button("Open Editer"))
        {
            UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
            if (!Engine)
            {
                return;
            }
            if (SelectedTemplateName != TEXT("None"))
            {
                Engine->StartParticleSystemViewer(ParticleSystemComp->TemplateName);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForExponentialHeightFogComponent(UHeightFogComponent* FogComponent) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    if (ImGui::TreeNodeEx("Exponential Height Fog", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        FLinearColor CurrColor = FogComponent->GetFogColor();

        float R = CurrColor.R;
        float G = CurrColor.G;
        float B = CurrColor.B;
        float A = CurrColor.A;
        float H, S, V;
        float LightColor[4] = { R, G, B, A };

        // Fog Color
        if (ImGui::ColorPicker4("##Fog Color", LightColor,
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_Float))
        {

            R = LightColor[0];
            G = LightColor[1];
            B = LightColor[2];
            A = LightColor[3];
            FogComponent->SetFogColor(FLinearColor(R, G, B, A));
        }
        RGBToHSV(R, G, B, H, S, V);
        // RGB/HSV
        bool ChangedRGB = false;
        bool ChangedHSV = false;

        // RGB
        ImGui::PushItemWidth(50.0f);
        if (ImGui::DragFloat("R##R", &R, 0.001f, 0.f, 1.f)) ChangedRGB = true;
        ImGui::SameLine();
        if (ImGui::DragFloat("G##G", &G, 0.001f, 0.f, 1.f)) ChangedRGB = true;
        ImGui::SameLine();
        if (ImGui::DragFloat("B##B", &B, 0.001f, 0.f, 1.f)) ChangedRGB = true;
        ImGui::Spacing();

        // HSV
        if (ImGui::DragFloat("H##H", &H, 0.1f, 0.f, 360)) ChangedHSV = true;
        ImGui::SameLine();
        if (ImGui::DragFloat("S##S", &S, 0.001f, 0.f, 1)) ChangedHSV = true;
        ImGui::SameLine();
        if (ImGui::DragFloat("V##V", &V, 0.001f, 0.f, 1)) ChangedHSV = true;
        ImGui::PopItemWidth();
        ImGui::Spacing();

        if (ChangedRGB && !ChangedHSV)
        {
            // RGB -> HSV
            RGBToHSV(R, G, B, H, S, V);
            FogComponent->SetFogColor(FLinearColor(R, G, B, A));
        }
        else if (ChangedHSV && !ChangedRGB)
        {
            // HSV -> RGB
            HSVToRGB(H, S, V, R, G, B);
            FogComponent->SetFogColor(FLinearColor(R, G, B, A));
        }

        float FogDensity = FogComponent->GetFogDensity();
        if (ImGui::SliderFloat("Density", &FogDensity, 0.00f, 3.0f))
        {
            FogComponent->SetFogDensity(FogDensity);
        }

        float FogDistanceWeight = FogComponent->GetFogDistanceWeight();
        if (ImGui::SliderFloat("Distance Weight", &FogDistanceWeight, 0.00f, 3.0f))
        {
            FogComponent->SetFogDistanceWeight(FogDistanceWeight);
        }

        float FogHeightFallOff = FogComponent->GetFogHeightFalloff();
        if (ImGui::SliderFloat("Height Fall Off", &FogHeightFallOff, 0.001f, 0.15f))
        {
            FogComponent->SetFogHeightFalloff(FogHeightFallOff);
        }

        float FogStartDistance = FogComponent->GetStartDistance();
        if (ImGui::SliderFloat("Start Distance", &FogStartDistance, 0.00f, 50.0f))
        {
            FogComponent->SetStartDistance(FogStartDistance);
        }

        float FogEndtDistance = FogComponent->GetEndDistance();
        if (ImGui::SliderFloat("End Distance", &FogEndtDistance, 0.00f, 50.0f))
        {
            FogComponent->SetEndDistance(FogEndtDistance);
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForShapeComponent(UShapeComponent* ShapeComponent) const
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (USphereComponent* Component = Cast<USphereComponent>(ShapeComponent))
    {
        if (ImGui::TreeNodeEx("Sphere Collision", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
        {
            float Radius = Component->GetRadius();
            ImGui::Text("Radius");
            ImGui::SameLine();
            if (ImGui::DragFloat("##Radius", &Radius, 0.01f, 0.f, 1000.f))
            {
                Component->SetRadius(Radius);
            }
            ImGui::TreePop();
        }
    }

    if (UBoxComponent* Component = Cast<UBoxComponent>(ShapeComponent))
    {
        if (ImGui::TreeNodeEx("Box Collision", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
        {
            FVector Extent = Component->GetBoxExtent();

            float Extents[3] = { Extent.X, Extent.Y, Extent.Z };

            ImGui::Text("Extent");
            ImGui::SameLine();
            if (ImGui::DragFloat3("##Extent", Extents, 0.01f, 0.f, 1000.f))
            {
                Component->SetBoxExtent(FVector(Extents[0], Extents[1], Extents[2]));
            }
            ImGui::TreePop();
        }
    }

    if (UCapsuleComponent* Component = Cast<UCapsuleComponent>(ShapeComponent))
    {
        if (ImGui::TreeNodeEx("Box Collision", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
        {
            float HalfHeight = Component->GetHalfHeight();
            float Radius = Component->GetRadius();

            ImGui::Text("HalfHeight");
            ImGui::SameLine();
            if (ImGui::DragFloat("##HalfHeight", &HalfHeight, 0.01f, 0.f, 1000.f)) {
                Component->SetHalfHeight(HalfHeight);
            }

            ImGui::Text("Radius");
            ImGui::SameLine();
            if (ImGui::DragFloat("##Radius", &Radius, 0.01f, 0.f, 1000.f)) {
                Component->SetRadius(Radius);
            }
            ImGui::TreePop();
        }
    }
    
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForSpringArmComponent(USpringArmComponent* SpringArmComponent) const
{
    if (ImGui::TreeNodeEx("SpringArm", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        // --- TargetOffset (FVector) ---
        float TargetOffsetValues[3] = {
            SpringArmComponent->TargetOffset.X,
            SpringArmComponent->TargetOffset.Y,
            SpringArmComponent->TargetOffset.Z
        };
        if (ImGui::DragFloat3("TargetOffset", TargetOffsetValues, 1.0f, -1000.0f, 1000.0f))
        {
            SpringArmComponent->TargetOffset.X = TargetOffsetValues[0];
            SpringArmComponent->TargetOffset.Y = TargetOffsetValues[1];
            SpringArmComponent->TargetOffset.Z = TargetOffsetValues[2];
        }
        ImGui::Spacing();

        // --- Floats: ArmLength / ProbeSize ---
        ImGui::DragFloat("ArmLength", &SpringArmComponent->TargetArmLength, 1.0f, 0.0f, 1000.0f);
        ImGui::DragFloat("ProbeSize", &SpringArmComponent->ProbeSize, 0.1f, 0.0f, 30.0f);
        ImGui::Spacing();

        // --- Bools (2 per line) ---
        bool DoCollisionTest = SpringArmComponent->bDoCollisionTest;
        if (ImGui::Checkbox("DoCollisionTest", &DoCollisionTest))
            SpringArmComponent->bDoCollisionTest = DoCollisionTest;
        ImGui::SameLine();
        bool UsePawnControlRotation = SpringArmComponent->bUsePawnControlRotation;
        if (ImGui::Checkbox("UsePawnControlRotation", &UsePawnControlRotation))
            SpringArmComponent->bUsePawnControlRotation = UsePawnControlRotation;

		bool UseAbsolRot = SpringArmComponent->IsUsingAbsoluteRotation();
		if (ImGui::Checkbox("UseAbsoluteRot", &UseAbsolRot))
			SpringArmComponent->SetUsingAbsoluteRotation(UseAbsolRot);

        bool InheritPitch = SpringArmComponent->bInheritPitch;
        if (ImGui::Checkbox("InheritPitch", &InheritPitch))
            SpringArmComponent->bInheritPitch = InheritPitch;
        ImGui::SameLine();
        bool InheritYaw = SpringArmComponent->bInheritYaw;
        if (ImGui::Checkbox("InheritYaw", &InheritYaw))
            SpringArmComponent->bInheritYaw = InheritYaw;

        bool InheritRoll = SpringArmComponent->bInheritRoll;
        if (ImGui::Checkbox("InheritRoll", &InheritRoll))
            SpringArmComponent->bInheritRoll = InheritRoll;
        ImGui::SameLine();
        bool EnableCameraLag = SpringArmComponent->bEnableCameraLag;
        if (ImGui::Checkbox("EnableCameraLag", &EnableCameraLag))
            SpringArmComponent->bEnableCameraLag = EnableCameraLag;

        bool EnableCameraRotationLag = SpringArmComponent->bEnableCameraRotationLag;
        if (ImGui::Checkbox("EnableCameraRotationLag", &EnableCameraRotationLag))
            SpringArmComponent->bEnableCameraRotationLag = EnableCameraRotationLag;
        ImGui::SameLine();
        bool UseCameraLagSubstepping = SpringArmComponent->bUseCameraLagSubstepping;
        if (ImGui::Checkbox("UseCameraLagSubstepping", &UseCameraLagSubstepping))
            SpringArmComponent->bUseCameraLagSubstepping = UseCameraLagSubstepping;
        ImGui::Spacing();

        // --- Lag speeds / limits ---
        ImGui::DragFloat("LocSpeed", &SpringArmComponent->CameraLagSpeed, 0.1f, 0.0f, 100.0f);
        
        ImGui::DragFloat("RotSpeed", &SpringArmComponent->CameraRotationLagSpeed, 0.1f, 0.0f, 100.0f);
        //ImGui::NewLine();
        ImGui::DragFloat("LagMxStep", &SpringArmComponent->CameraLagMaxTimeStep, 0.005f, 0.0f, 1.0f);
        ImGui::SameLine();
        ImGui::DragFloat("LogMDist", &SpringArmComponent->CameraLagMaxDistance, 1.0f, 0.0f, 1000.0f);

        ImGui::TreePop();
    }
}

// 텍스처 관련 헬퍼 함수 구현
void PropertyEditorPanel::ScanTextureFiles()
{
    if (bTexturesScanned)
        return;

    AvailableTextures.Empty();

    TMap<FWString, std::shared_ptr<FTexture>> TextureMap = FEngineLoop::ResourceManager.GetTextureMap();

    for (const auto& TexturePair : TextureMap)
    {
        const FWString& TextureName = TexturePair.Key;
        const std::shared_ptr<FTexture>& Texture = TexturePair.Value;
        // 텍스처가 유효한지 확인
        if (Texture)
        {
            AvailableTextures.Add(TextureName);
        }
    }
    
    // 텍스처 이름으로 정렬
    AvailableTextures.Sort([](const FWString& A, const FWString& B) {
        // 파일 이름만 추출
        size_t lastSlashA = A.find_last_of(L"/\\");
        size_t lastSlashB = B.find_last_of(L"/\\");
        
        FWString FileNameA = (lastSlashA != std::wstring::npos) ? A.substr(lastSlashA + 1) : A;
        FWString FileNameB = (lastSlashB != std::wstring::npos) ? B.substr(lastSlashB + 1) : B;
        
        // 알파벳순 비교
        return FileNameA < FileNameB;
    });

    bTexturesScanned = true;
}

TArray<FWString> PropertyEditorPanel::GetAvailableTextures() const
{
    return AvailableTextures;
}

const char* PropertyEditorPanel::GetTextureSlotName(EMaterialTextureSlots Slot) const
{
    switch (Slot)
    {
    case EMaterialTextureSlots::MTS_Diffuse:   return "Diffuse";
    case EMaterialTextureSlots::MTS_Specular:  return "Specular";
    case EMaterialTextureSlots::MTS_Normal:    return "Normal";
    case EMaterialTextureSlots::MTS_Emissive:  return "Emissive";
    case EMaterialTextureSlots::MTS_Alpha:     return "Alpha";
    case EMaterialTextureSlots::MTS_Ambient:   return "Ambient";
    case EMaterialTextureSlots::MTS_Shininess: return "Shininess";
    case EMaterialTextureSlots::MTS_Metallic:  return "Metallic";
    case EMaterialTextureSlots::MTS_Roughness: return "Roughness";
    default: return "Unknown";
    }
}

void PropertyEditorPanel::RenderForMaterial(UStaticMeshComponent* StaticMeshComp)
{
    if (StaticMeshComp->GetStaticMesh() == nullptr)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        for (uint32 i = 0; i < StaticMeshComp->GetNumMaterials(); ++i)
        {
            if (ImGui::Selectable(GetData(StaticMeshComp->GetMaterialSlotNames()[i].ToString()), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    std::cout << GetData(StaticMeshComp->GetMaterialSlotNames()[i].ToString()) << std::endl;
                    SelectedMaterialIndex = i;
                    SelectedStaticMeshComp = StaticMeshComp;
                }
            }
        }

        if (ImGui::Button("    +    ")) {
            IsCreateMaterial = true;
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("SubMeshes", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        const auto Subsets = StaticMeshComp->GetStaticMesh()->GetRenderData()->MaterialSubsets;
        for (uint32 i = 0; i < Subsets.Num(); ++i)
        {
            std::string temp = "subset " + std::to_string(i);
            if (ImGui::Selectable(temp.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    StaticMeshComp->SetselectedSubMeshIndex(i);
                    SelectedStaticMeshComp = StaticMeshComp;
                }
            }
        }
        const std::string Temp = "clear subset";
        if (ImGui::Selectable(Temp.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(0))
                StaticMeshComp->SetselectedSubMeshIndex(-1);
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();

    if (SelectedMaterialIndex != -1)
    {
        RenderMaterialView(SelectedStaticMeshComp->GetMaterial(SelectedMaterialIndex));
    }
    if (IsCreateMaterial) {
        RenderCreateMaterialView();
    }
}

void PropertyEditorPanel::RenderMaterialView(UMaterial* Material)
{
    // 텍스처 스캔
    if (!bTexturesScanned)
    {
        ScanTextureFiles();
    }

    ImGui::SetNextWindowSize(ImVec2(380, 600), ImGuiCond_Once);
    ImGui::Begin("Material Viewer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

    static ImGuiSelectableFlags BaseFlag = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None | ImGuiColorEditFlags_NoAlpha;
    
    // 검색 필터 추가
    static char filterBuffer[128] = "";
    ImGui::InputText("Filter Textures", filterBuffer, IM_ARRAYSIZE(filterBuffer));
    std::string filter = filterBuffer;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    const FVector MatDiffuseColor = Material->GetMaterialInfo().DiffuseColor;
    const FVector MatSpecularColor = Material->GetMaterialInfo().SpecularColor;
    const FVector MatAmbientColor = Material->GetMaterialInfo().AmbientColor;
    const FVector MatEmissiveColor = Material->GetMaterialInfo().EmissiveColor;

    const float DiffuseR = MatDiffuseColor.X;
    const float DiffuseG = MatDiffuseColor.Y;
    const float DiffuseB = MatDiffuseColor.Z;
    constexpr float DiffuseA = 1.0f;
    float DiffuseColorPick[4] = { DiffuseR, DiffuseG, DiffuseB, DiffuseA };

    ImGui::Text("Material Name |");
    ImGui::SameLine();
    ImGui::Text(*Material->GetMaterialInfo().MaterialName);
    ImGui::Separator();

    ImGui::Text("  Diffuse Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Diffuse##Color", reinterpret_cast<float*>(&DiffuseColorPick), BaseFlag))
    {
        const FVector NewColor = { DiffuseColorPick[0], DiffuseColorPick[1], DiffuseColorPick[2] };
        Material->SetDiffuse(NewColor);
    }

    const float SpecularR = MatSpecularColor.X;
    const float SpecularG = MatSpecularColor.Y;
    const float SpecularB = MatSpecularColor.Z;
    constexpr float SpecularA = 1.0f;
    float SpecularColorPick[4] = { SpecularR, SpecularG, SpecularB, SpecularA };

    ImGui::Text("Specular Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Specular##Color", reinterpret_cast<float*>(&SpecularColorPick), BaseFlag))
    {
        const FVector NewColor = { SpecularColorPick[0], SpecularColorPick[1], SpecularColorPick[2] };
        Material->SetSpecular(NewColor);
    }

    const float AmbientR = MatAmbientColor.X;
    const float AmbientG = MatAmbientColor.Y;
    const float AmbientB = MatAmbientColor.Z;
    constexpr float AmbientA = 1.0f;
    float AmbientColorPick[4] = { AmbientR, AmbientG, AmbientB, AmbientA };

    ImGui::Text("Ambient Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Ambient##Color", reinterpret_cast<float*>(&AmbientColorPick), BaseFlag))
    {
        const FVector NewColor = { AmbientColorPick[0], AmbientColorPick[1], AmbientColorPick[2] };
        Material->SetAmbient(NewColor);
    }

    const float EmissiveR = MatEmissiveColor.X;
    const float EmissiveG = MatEmissiveColor.Y;
    const float EmissiveB = MatEmissiveColor.Z;
    constexpr float EmissiveA = 1.0f;
    float EmissiveColorPick[4] = { EmissiveR, EmissiveG, EmissiveB, EmissiveA };

    ImGui::Text("Emissive Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Emissive##Color", reinterpret_cast<float*>(&EmissiveColorPick), BaseFlag))
    {
        const FVector NewColor = { EmissiveColorPick[0], EmissiveColorPick[1], EmissiveColorPick[2] };
        Material->SetEmissive(NewColor);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 텍스처 섹션 헤더
    ImGui::Separator();
    ImGui::Text("Textures");
    ImGui::Spacing();
    
    if (AvailableTextures.Num() == 0)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No textures found. Check Contents/Texture/ directory.");
    }
    else
    {
        // 재질에 있는 기존 텍스처 정보 가져오기
        FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();
        TArray<FTextureInfo>& TextureInfos = MaterialInfo.TextureInfos;

        // 각 텍스처 유형에 대한 드롭다운 메뉴
        const struct TextureSlotUI
        {
            EMaterialTextureSlots Slot;
            const char* Name;
            ImVec4 LabelColor;  // 텍스처 유형별 색상
        } TextureSlots[] = {
            { EMaterialTextureSlots::MTS_Diffuse,   "Diffuse",   ImVec4(0.9f, 0.9f, 0.9f, 1.0f) },
            { EMaterialTextureSlots::MTS_Specular,    "Specular",    ImVec4(0.0f, 0.5f, 1.0f, 1.0f) },
            { EMaterialTextureSlots::MTS_Normal,  "Normal",  ImVec4(1.0f, 1.0f, 0.5f, 1.0f) },
            { EMaterialTextureSlots::MTS_Emissive,  "Emissive",  ImVec4(0.7f, 0.7f, 0.7f, 1.0f) },
            { EMaterialTextureSlots::MTS_Alpha, "Alpha", ImVec4(0.5f, 0.5f, 0.5f, 1.0f) },
            { EMaterialTextureSlots::MTS_Ambient,  "Ambient",  ImVec4(1.0f, 0.7f, 0.3f, 1.0f) },
            { EMaterialTextureSlots::MTS_Shininess,   "Shininess",   ImVec4(0.5f, 0.8f, 1.0f, 1.0f) },
            { EMaterialTextureSlots::MTS_Metallic,     "Metallic",     ImVec4(1.0f, 1.0f, 1.0f, 1.0f) },
            { EMaterialTextureSlots::MTS_Roughness, "Roughness", ImVec4(0.7f, 0.7f, 0.5f, 1.0f) }
        };

        for (int j = 0; j < TextureInfos.Num(); ++j)
        {
            const int SlotIdx = static_cast<int>(TextureSlots[j].Slot);
            const uint32 SlotFlag = 1 << SlotIdx;
            
            // 현재 슬롯에 할당된 텍스처 찾기
            FString CurrentTexturePath = "None";
            int TextureIndex = j;
            if ((MaterialInfo.TextureFlag & SlotFlag) != 0)
            {
                    CurrentTexturePath = TextureInfos[j].TexturePath;
            }
            // 슬롯 이름 표시 (색상 적용)
            ImGui::TextColored(TextureSlots[j].LabelColor, "%s:", TextureSlots[j].Name);
            ImGui::SameLine();

            // 드롭다운 메뉴 ID 생성
            char comboID[64];
            sprintf_s(comboID, "##Texture_%s", TextureSlots[j].Name);

            // 드롭다운 메뉴 표시
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10.0f);
            if (ImGui::BeginCombo(comboID, *CurrentTexturePath))
            {
                // "None" 옵션
                bool isNoneSelected = (CurrentTexturePath == "None");
                if (ImGui::Selectable("None", isNoneSelected))
                {
                    MaterialInfo.TextureFlag &= ~SlotFlag;  // 해당 비트 지우기
                    
                    if (TextureIndex >= 0 && TextureIndex < TextureInfos.Num())
                    {
                        TextureInfos.RemoveAt(TextureIndex);
                    }
                }

                // 필터 추가
                static char filterBuffer[256] = "";
                ImGui::InputText("Filter", filterBuffer, IM_ARRAYSIZE(filterBuffer));
                std::string filter = filterBuffer;
                std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

                // 사용 가능한 모든 텍스처 표시 (필터링 적용)
                ImGui::BeginChild("TextureList", ImVec2(0, 200), true);
                for (const FWString& TexturePath : AvailableTextures)
                {
                    // 경로에서 파일 이름만 추출
                    FWString FileName = TexturePath;
                    size_t lastSlash = FileName.find_last_of(L"/\\");
                    if (lastSlash != std::wstring::npos)
                    {
                        FileName = FileName.substr(lastSlash + 1);
                    }
                    
                    FString FileNameStr(FileName.c_str());
                    
                    // 필터링 적용
                    if (!filter.empty())
                    {
                        std::string fileNameLower = std::string(FileNameStr);
                        std::transform(fileNameLower.begin(), fileNameLower.end(), fileNameLower.begin(), ::tolower);
                        
                        if (fileNameLower.find(filter) == std::string::npos)
                        {
                            continue;  // 필터와 일치하지 않으면 건너뛰기
                        }
                    }
                    
                    bool isSelected = (FileNameStr == CurrentTexturePath);
                    
                    if (ImGui::Selectable(*FileNameStr, isSelected))
                    {
                        FTextureInfo NewTextureInfo;
                        NewTextureInfo.TextureName = FileNameStr;
                        NewTextureInfo.TexturePath = TexturePath;
                        NewTextureInfo.bIsSRGB = true;  // 기본값
                        
                        if (TextureIndex >= 0 && TextureIndex < TextureInfos.Num())
                        {
                            TextureInfos[j] = NewTextureInfo;
                            MaterialInfo.TextureFlag |= SlotFlag;  // 해당 비트 설정
                        }
                        
                    }
                    
                    if (isSelected)
                    {
                        ImGui::SameLine();
                        ImGui::Text("[Selected]");
                        ImGui::SameLine();
                       
                    }
                }
                ImGui::EndChild();
                
                ImGui::EndCombo();
            }

            // SRGB 체크박스 추가 (텍스처가 선택된 경우만)
            if (CurrentTexturePath != "None" && TextureIndex >= 0 && TextureIndex < TextureInfos.Num())
            {
                ImGui::SameLine();
                bool bIsSRGB = TextureInfos[TextureIndex].bIsSRGB;
                if (ImGui::Checkbox(("sRGB##" + std::string(TextureSlots[j].Name)).c_str(), &bIsSRGB))
                {
                    TextureInfos[TextureIndex].bIsSRGB = bIsSRGB;
                }
                
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("sRGB color space (checked) or linear color space (unchecked)");
                }
            }
        }
    }
   

    ImGui::Spacing();
    // @note 머티리얼 뷰어에 부합하지 않는 기능임. 따로 분리할 것.
    //ImGui::Separator();

    //ImGui::Text("Choose Material");
    //ImGui::Spacing();

    //ImGui::Text("Material Slot Name |");
    //ImGui::SameLine();
    //ImGui::Text(GetData(SelectedStaticMeshComp->GetMaterialSlotNames()[SelectedMaterialIndex].ToString()));

    //ImGui::Text("Override Material |");
    //ImGui::SameLine();
    //ImGui::SetNextItemWidth(160);
    //// 메테리얼 이름 목록을 const char* 배열로 변환
    //std::vector<const char*> MaterialChars;
    //for (const auto& Material : FObjManager::GetMaterials())
    //{
    //    MaterialChars.push_back(*Material.Value->GetMaterialInfo().MaterialName);
    //}

    ////// 드롭다운 표시 (currentMaterialIndex가 범위를 벗어나지 않도록 확인)
    ////if (currentMaterialIndex >= FManagerGetMaterialNum())
    ////    currentMaterialIndex = 0;

    //// 드롭다운 너비 제한 설정
    //ImGui::SetNextItemWidth(200.0f);
    //
    //// ImGui::Combo 사용 시 고유한 ID 사용
    //if (ImGui::Combo("##MaterialDropdown", &CurMaterialIndex, MaterialChars.data(), FObjManager::GetMaterialNum())) {
    //    UMaterial* Material = FObjManager::GetMaterial(MaterialChars[CurMaterialIndex]);
    //    SelectedStaticMeshComp->SetMaterial(SelectedMaterialIndex, Material);
    //}

    if (ImGui::Button("Close"))
    {
        SelectedMaterialIndex = -1;
        SelectedStaticMeshComp = nullptr;
        bShowMaterialView = false;
    }

    ImGui::End();
}

void PropertyEditorPanel::RenderCreateMaterialView()
{
    // 텍스처 스캔
    if (!bTexturesScanned)
        ScanTextureFiles();

    ImGui::SetNextWindowSize(ImVec2(300, 700), ImGuiCond_Once);
    ImGui::Begin("Create Material Viewer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

    static ImGuiSelectableFlags BaseFlag = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None | ImGuiColorEditFlags_NoAlpha;

    ImGui::Text("New Name");
    ImGui::SameLine();
    static char MaterialName[256] = "New Material";
    // 기본 텍스트 입력 필드
    ImGui::SetNextItemWidth(128);


    TArray<FTextureInfo>& TextureInfos = tempMaterialInfo.TextureInfos;
    TextureInfos.SetNum(static_cast<uint32>(EMaterialTextureSlots::MTS_MAX));

    if (ImGui::InputText("##NewName", MaterialName, IM_ARRAYSIZE(MaterialName))) {
        tempMaterialInfo.MaterialName = MaterialName;
    }

    const FVector MatDiffuseColor = tempMaterialInfo.DiffuseColor;
    const FVector MatSpecularColor = tempMaterialInfo.SpecularColor;
    const FVector MatAmbientColor = tempMaterialInfo.AmbientColor;
    const FVector MatEmissiveColor = tempMaterialInfo.EmissiveColor;

    const float DiffuseR = MatDiffuseColor.X;
    const float DiffuseG = MatDiffuseColor.Y;
    const float DiffuseB = MatDiffuseColor.Z;
    constexpr float DiffuseA = 1.0f;
    float DiffuseColorPick[4] = { DiffuseR, DiffuseG, DiffuseB, DiffuseA };

    ImGui::Text("Set Property");
    ImGui::Indent();

    ImGui::Text("  Diffuse Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Diffuse##Color", reinterpret_cast<float*>(&DiffuseColorPick), BaseFlag))
    {
        const FVector NewColor = { DiffuseColorPick[0], DiffuseColorPick[1], DiffuseColorPick[2] };
        tempMaterialInfo.DiffuseColor = NewColor;
    }

    const float SpecularR = MatSpecularColor.X;
    const float SpecularG = MatSpecularColor.Y;
    const float SpecularB = MatSpecularColor.Z;
    constexpr float SpecularA = 1.0f;
    float SpecularColorPick[4] = { SpecularR, SpecularG, SpecularB, SpecularA };

    ImGui::Text("Specular Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Specular##Color", reinterpret_cast<float*>(&SpecularColorPick), BaseFlag))
    {
        const FVector NewColor = { SpecularColorPick[0], SpecularColorPick[1], SpecularColorPick[2] };
        tempMaterialInfo.SpecularColor = NewColor;
    }

    const float AmbientR = MatAmbientColor.X;
    const float AmbientG = MatAmbientColor.Y;
    const float AmbientB = MatAmbientColor.Z;
    constexpr float AmbientA = 1.0f;
    float AmbientColorPick[4] = { AmbientR, AmbientG, AmbientB, AmbientA };

    ImGui::Text("Ambient Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Ambient##Color", reinterpret_cast<float*>(&AmbientColorPick), BaseFlag))
    {
        const FVector NewColor = { AmbientColorPick[0], AmbientColorPick[1], AmbientColorPick[2] };
        tempMaterialInfo.AmbientColor = NewColor;
    }

    const float EmissiveR = MatEmissiveColor.X;
    const float EmissiveG = MatEmissiveColor.Y;
    const float EmissiveB = MatEmissiveColor.Z;
    constexpr float EmissiveA = 1.0f;
    float EmissiveColorPick[4] = { EmissiveR, EmissiveG, EmissiveB, EmissiveA };

    ImGui::Text("Emissive Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Emissive##Color", reinterpret_cast<float*>(&EmissiveColorPick), BaseFlag))
    {
        const FVector NewColor = { EmissiveColorPick[0], EmissiveColorPick[1], EmissiveColorPick[2] };
        tempMaterialInfo.EmissiveColor = NewColor;
    }
    ImGui::Unindent();

    // 텍스처 섹션
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::Text("Textures");
    ImGui::Spacing();

    // 텍스처 필터링을 위한 입력 필드
    static char createFilterBuffer[128] = "";
    ImGui::InputText("Filter Textures", createFilterBuffer, IM_ARRAYSIZE(createFilterBuffer));
    std::string createFilter = createFilterBuffer;
    std::transform(createFilter.begin(), createFilter.end(), createFilter.begin(), ::tolower);
    
    if (AvailableTextures.Num() == 0)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No textures found. Check Contents/Texture/ directory.");
    }
    else
    {
        // 각 텍스처 유형에 대한 드롭다운 메뉴
        const struct TextureSlotUI
        {
            EMaterialTextureSlots Slot;
            const char* Name;
            ImVec4 LabelColor;  // 텍스처 유형별 색상
        } TextureSlots[] = {
            { EMaterialTextureSlots::MTS_Diffuse,   "Diffuse",   ImVec4(0.9f, 0.9f, 0.9f, 1.0f) },
            { EMaterialTextureSlots::MTS_Specular,    "Specular",    ImVec4(0.0f, 0.5f, 1.0f, 1.0f) },
            { EMaterialTextureSlots::MTS_Normal,  "Normal",  ImVec4(1.0f, 1.0f, 0.5f, 1.0f) },
            { EMaterialTextureSlots::MTS_Emissive,  "Emissive",  ImVec4(0.7f, 0.7f, 0.7f, 1.0f) },
            { EMaterialTextureSlots::MTS_Alpha, "Alpha", ImVec4(0.5f, 0.5f, 0.5f, 1.0f) },
            { EMaterialTextureSlots::MTS_Ambient,  "Ambient",  ImVec4(1.0f, 0.7f, 0.3f, 1.0f) },
            { EMaterialTextureSlots::MTS_Shininess,   "Shininess",   ImVec4(0.5f, 0.8f, 1.0f, 1.0f) },
            { EMaterialTextureSlots::MTS_Metallic,     "Metallic",     ImVec4(1.0f, 1.0f, 1.0f, 1.0f) },
            { EMaterialTextureSlots::MTS_Roughness, "Roughness", ImVec4(0.7f, 0.7f, 0.5f, 1.0f) }
        };

        for (int j = 0; j < TextureInfos.Num(); ++j)
        {
            const int SlotIdx = static_cast<int>(TextureSlots[j].Slot);
            const uint32 SlotFlag = 1 << SlotIdx;

            // 현재 슬롯에 할당된 텍스처 찾기
            FString CurrentTexturePath = "None";
            int TextureIndex = j;
            if ((tempMaterialInfo.TextureFlag & SlotFlag) != 0)
            {
                CurrentTexturePath = TextureInfos[j].TexturePath;
            }
            // 슬롯 이름 표시 (색상 적용)
            ImGui::TextColored(TextureSlots[j].LabelColor, "%s:", TextureSlots[j].Name);
            ImGui::SameLine();

            // 드롭다운 메뉴 ID 생성
            char comboID[64];
            sprintf_s(comboID, "##Texture_%s", TextureSlots[j].Name);

            // 드롭다운 메뉴 표시
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10.0f);
            if (ImGui::BeginCombo(comboID, *CurrentTexturePath))
            {
                // "None" 옵션
                bool isNoneSelected = (CurrentTexturePath == "None");
                if (ImGui::Selectable("None", isNoneSelected))
                {
                    tempMaterialInfo.TextureFlag &= ~SlotFlag;  // 해당 비트 지우기

                    if (TextureIndex >= 0 && TextureIndex < TextureInfos.Num())
                    {
                        TextureInfos.RemoveAt(TextureIndex);
                    }
                }

                // 필터 추가
                static char filterBuffer[256] = "";
                ImGui::InputText("Filter", filterBuffer, IM_ARRAYSIZE(filterBuffer));
                std::string filter = filterBuffer;
                std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

                // 사용 가능한 모든 텍스처 표시 (필터링 적용)
                ImGui::BeginChild("TextureList", ImVec2(0, 200), true);
                for (const FWString& TexturePath : AvailableTextures)
                {
                    // 경로에서 파일 이름만 추출
                    FWString FileName = TexturePath;
                    size_t lastSlash = FileName.find_last_of(L"/\\");
                    if (lastSlash != std::wstring::npos)
                    {
                        FileName = FileName.substr(lastSlash + 1);
                    }

                    FString FileNameStr(FileName.c_str());

                    // 필터링 적용
                    if (!filter.empty())
                    {
                        std::string fileNameLower = std::string(FileNameStr);
                        std::transform(fileNameLower.begin(), fileNameLower.end(), fileNameLower.begin(), ::tolower);

                        if (fileNameLower.find(filter) == std::string::npos)
                        {
                            continue;  // 필터와 일치하지 않으면 건너뛰기
                        }
                    }

                    bool isSelected = (FileNameStr == CurrentTexturePath);

                    if (ImGui::Selectable(*FileNameStr, isSelected))
                    {
                        FTextureInfo NewTextureInfo;
                        NewTextureInfo.TextureName = FileNameStr;
                        NewTextureInfo.TexturePath = TexturePath;
                        NewTextureInfo.bIsSRGB = true;  // 기본값

                        if (TextureIndex >= 0 && TextureIndex < TextureInfos.Num())
                        {
                            TextureInfos[j] = NewTextureInfo;
                            tempMaterialInfo.TextureFlag |= SlotFlag;  // 해당 비트 설정
                        }

                    }

                    if (isSelected)
                    {
                        ImGui::SameLine();
                        ImGui::Text("[Selected]");
                        ImGui::SameLine();

                    }
                }
                ImGui::EndChild();

                ImGui::EndCombo();
            }

            // SRGB 체크박스 추가 (텍스처가 선택된 경우만)
            if (CurrentTexturePath != "None" && TextureIndex >= 0 && TextureIndex < TextureInfos.Num())
            {
                ImGui::SameLine();
                bool bIsSRGB = TextureInfos[TextureIndex].bIsSRGB;
                if (ImGui::Checkbox(("sRGB##" + std::string(TextureSlots[j].Name)).c_str(), &bIsSRGB))
                {
                    TextureInfos[TextureIndex].bIsSRGB = bIsSRGB;
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("sRGB color space (checked) or linear color space (unchecked)");
                }
            }
        }
    }

    ImGui::NewLine();
    if (ImGui::Button("Create Material")) {
        FObjManager::CreateMaterial(tempMaterialInfo);
    }

    ImGui::NewLine();
    if (ImGui::Button("Close"))
    {
        IsCreateMaterial = false;
    }

    ImGui::End();
}

void PropertyEditorPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = static_cast<float>(ClientRect.right - ClientRect.left);
    Height = static_cast<float>(ClientRect.bottom - ClientRect.top);
}

void PropertyEditorPanel::RenderForParticleSystem(UParticleSystem* ParticleSystem)
{
    if (!ParticleSystem)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.3f, 1.0f));
    if (ImGui::TreeNodeEx("Particle System", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 이름 표시
        ImGui::Text("Name: %s", GetData(ParticleSystem->GetName()));
        
        // 업데이트 모드 설정
        const char* UpdateModeItems[] = { "RealTime", "FixedTime" };
        int UpdateModeIndex = static_cast<int>(ParticleSystem->SystemUpdateMode);
        if (ImGui::Combo("Update Mode", &UpdateModeIndex, UpdateModeItems, IM_ARRAYSIZE(UpdateModeItems)))
        {
            ParticleSystem->SystemUpdateMode = static_cast<EParticleSystemUpdateMode>(UpdateModeIndex);
        }

        // Fixed Time 모드일 경우 FPS 설정
        if (ParticleSystem->SystemUpdateMode == EPSUM_FixedTime)
        {
            float UpdateTimeFPS = ParticleSystem->UpdateTime_FPS;
            if (ImGui::DragFloat("Update FPS", &UpdateTimeFPS, 1.0f, 10.0f, 120.0f))
            {
                ParticleSystem->UpdateTime_FPS = UpdateTimeFPS;
                // 델타 타임 자동 계산 (초당 프레임 수의 역수)
                ParticleSystem->UpdateTime_Delta = 1.0f / UpdateTimeFPS;
            }
        }

        // 웜업 타임 설정 (주의 텍스트와 함께)
        float WarmupTime = ParticleSystem->WarmupTime;
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Warning: High values may cause hitches");
        if (ImGui::DragFloat("Warmup Time", &WarmupTime, 0.1f, 0.0f, 10.0f))
        {
            ParticleSystem->WarmupTime = WarmupTime;
        }

        // 웜업 틱 레이트 설정
        float WarmupTickRate = ParticleSystem->WarmupTickRate;
        if (ImGui::DragFloat("Warmup Tick Rate", &WarmupTickRate, 0.001f, 0.001f, 0.1f))
        {
            ParticleSystem->WarmupTickRate = WarmupTickRate;
        }

        // LOD 방식 설정
        const char* LODMethodItems[] = { "Automatic", "DirectSet", "ActivateAutomatic" };
        int LODMethodIndex = static_cast<int>(ParticleSystem->LODMethod);
        if (ImGui::Combo("LOD Method", &LODMethodIndex, LODMethodItems, IM_ARRAYSIZE(LODMethodItems)))
        {
            ParticleSystem->LODMethod = static_cast<ParticleSystemLODMethod>(LODMethodIndex);
        }

        // LOD 거리 체크 시간 설정
        float LODDistanceCheckTime = ParticleSystem->LODDistanceCheckTime;
        if (ImGui::DragFloat("LOD Check Time", &LODDistanceCheckTime, 0.1f, 0.1f, 10.0f))
        {
            ParticleSystem->LODDistanceCheckTime = LODDistanceCheckTime;
        }

        // 카메라 방향 설정
        bool bOrientToCamera = ParticleSystem->bOrientZAxisTowardCamera != 0;
        if (ImGui::Checkbox("Orient Toward Camera", &bOrientToCamera))
        {
            ParticleSystem->bOrientZAxisTowardCamera = bOrientToCamera ? 1 : 0;
        }

        // 자동 비활성화 설정
        bool bAutoDeactivate = ParticleSystem->bAutoDeactivate != 0;
        if (ImGui::Checkbox("Auto Deactivate", &bAutoDeactivate))
        {
            ParticleSystem->bAutoDeactivate = bAutoDeactivate ? 1 : 0;
        }

        // 지연 시간 설정
        float Delay = ParticleSystem->Delay;
        if (ImGui::DragFloat("Spawn Delay", &Delay, 0.1f, 0.0f, 10.0f))
        {
            ParticleSystem->SetDelay(Delay);
        }

        // 지연 범위 사용 여부
        bool bUseDelayRange = ParticleSystem->bUseDelayRange != 0;
        if (ImGui::Checkbox("Use Delay Range", &bUseDelayRange))
        {
            ParticleSystem->bUseDelayRange = bUseDelayRange ? 1 : 0;
        }

        // 지연 범위를 사용하는 경우 최소값 설정
        if (bUseDelayRange)
        {
            float DelayLow = ParticleSystem->DelayLow;
            if (ImGui::DragFloat("Delay Low", &DelayLow, 0.1f, 0.0f, Delay))
            {
                ParticleSystem->DelayLow = DelayLow;
            }
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForParticleEmitter(UParticleEmitter* Emitter)
{
    if (!Emitter)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.3f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Particle Emitter", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 에미터 이름 표시 및 편집
        FString EmitterNameStr = Emitter->EmitterName.ToString();
        char EmitterNameBuffer[256];
        strcpy_s(EmitterNameBuffer, GetData(EmitterNameStr));

        ImGui::Text("Emitter Name: ");
        ImGui::SameLine();
        if (ImGui::InputText("##EmitterName", EmitterNameBuffer, IM_ARRAYSIZE(EmitterNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            FName NewEmitterName = FName(EmitterNameBuffer);
            Emitter->SetEmitterName(NewEmitterName);
        }

        // 에미터 활성화 상태 편집
        bool bEmitterEnabled = Emitter->bIsSoloing != 0;
        if (ImGui::Checkbox("Enabled", &bEmitterEnabled))
        {
            Emitter->bIsSoloing = bEmitterEnabled ? 1 : 0;
        }

        // 축 잠금 플래그 설정
        const char* AxisLockItems[] = { "None", "X", "Y", "Z", "XY", "XZ", "YZ", "XYZ" };
        int AxisLockIndex = static_cast<int>(Emitter->LockAxisFlags);
        if (ImGui::Combo("Lock Axis", &AxisLockIndex, AxisLockItems, IM_ARRAYSIZE(AxisLockItems)))
        {
            Emitter->LockAxisFlags = static_cast<EParticleAxisLock>(AxisLockIndex);
            Emitter->bAxisLockEnabled = (Emitter->LockAxisFlags != EParticleAxisLock::EPAL_NONE);
        }

        // 초기 할당 개수 설정
        int32 InitialAllocationCount = Emitter->InitialAllocationCount;
        if (ImGui::DragInt("Initial Allocation", &InitialAllocationCount, 1.0f, 0, 10000))
        {
            Emitter->InitialAllocationCount = InitialAllocationCount;
        }

        // LOD 설정
        ImGui::Separator();
        ImGui::Text("LOD Settings");

        // 현재 LOD 레벨 수 표시
        const int32 LODCount = Emitter->LODLevels.Num();
        ImGui::Text("LOD Levels: %d", LODCount);

        // 최적화 설정
        ImGui::Separator();
        ImGui::Text("Performance Settings");
        
        // 품질 레벨 스폰 속도 스케일 설정
        float QualityLevelSpawnRateScale = Emitter->QualityLevelSpawnRateScale;
        if (ImGui::DragFloat("Quality Scale", &QualityLevelSpawnRateScale, 0.01f, 0.0f, 1.0f))
        {
            Emitter->QualityLevelSpawnRateScale = QualityLevelSpawnRateScale;
        }

        // 현재 활성 파티클 수 표시
        ImGui::Text("Active Particles: %d", Emitter->PeakActiveParticles);

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::RenderForParticleModule(UParticleModule* Module)
{
    if (!Module)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Particle Module", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 모듈 이름 및 타입 표시
        ImGui::Text("Module Type: %s", GetData(Module->GetClass()->GetName()));

        // 모듈 활성화 상태 설정
        bool bEnabled = Module->bEnabled != 0;
        if (ImGui::Checkbox("Enabled", &bEnabled))
        {
            Module->bEnabled = bEnabled ? 1 : 0;
        }

        // 모듈 속성에 따른 추가 UI 표시
        switch (Module->GetModuleType())
        {
        case EPMT_Spawn:
            ImGui::Text("Spawn Module");
            break;
        case EPMT_Required:
            ImGui::Text("Required Module");
            // UParticleModuleRequired의 Material 속성에 대한 특별 처리
            if (UParticleModuleRequired* RequiredModule = Cast<UParticleModuleRequired>(Module))
            {
                ImGui::Separator();
                ImGui::Text("Material Settings");
                
                UMaterial* CurrentMaterial = RequiredModule->Material;
                // 머티리얼 미리보기 (100x100 크기로 확장)
                if (CurrentMaterial)
                {
                    // 텍스처와 디퓨즈 색상 모두 처리
                    bool bHasTexturePreview = false;
                    
                    // 머티리얼에서 텍스처 정보 가져오기
                    const FMaterialInfo& MaterialInfo = CurrentMaterial->GetMaterialInfo();
                    if (!MaterialInfo.TextureInfos.IsEmpty())
                    {
                        // 첫 번째 텍스처(보통 Diffuse 텍스처) 경로 확인
                        const FTextureInfo& TextureInfo = MaterialInfo.TextureInfos[0];
                        if (!TextureInfo.TexturePath.empty())
                        {
                            // 텍스처 이미지를 렌더링
                            ID3D11ShaderResourceView* TextureSRV = FEngineLoop::ResourceManager.GetTexture(TextureInfo.TexturePath)->TextureSRV;
                            if (TextureSRV)
                            {
                                ImGui::Image(reinterpret_cast<ImTextureID>(TextureSRV), ImVec2(100, 100));
                                bHasTexturePreview = true;
                            }
                        }
                    }
                    
                    // 텍스처가 없는 경우 디퓨즈 색상으로 미리보기 표시
                    if (!bHasTexturePreview)
                    {
                        // 색상 기반 미리보기 표시
                        FLinearColor PreviewColor(0.8f, 0.8f, 0.8f, 1.0f);
                        if (MaterialInfo.DiffuseColor.X > 0 ||
                            MaterialInfo.DiffuseColor.Y > 0 ||
                            MaterialInfo.DiffuseColor.Z > 0)
                        {
                            PreviewColor = FLinearColor(
                                MaterialInfo.DiffuseColor.X,
                                MaterialInfo.DiffuseColor.Y,
                                MaterialInfo.DiffuseColor.Z,
                                1.0f
                            );
                        }
                        
                        ImGui::ColorButton("##MaterialPreview",
                            ImVec4(PreviewColor.R, PreviewColor.G, PreviewColor.B, PreviewColor.A),
                            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                            ImVec2(100, 100));
                    }

                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        bShowMaterialView = true;
                        // @note RenderMaterialView의 Close에서 false로 변경해줌
                    }

                    if (bShowMaterialView)
                    {
                        RenderMaterialView(CurrentMaterial);
                    }
                }
                else
                {
                    ImGui::Dummy(ImVec2(100, 100));
                }
                ImGui::SameLine();
                {
                    FString PreviewName = FString("None");
                    if (UMaterial* ModuleMaterial = RequiredModule->Material)
                    {
                        if (FMaterialInfo* MaterialInfo = &ModuleMaterial->GetMaterialInfo())
                        {
                            PreviewName = MaterialInfo->MaterialName;
                        }
                    }

                    const auto MaterialMap = UAssetManager::Get().GetMaterialMap();

                    if (ImGui::BeginCombo("##Material", GetData(PreviewName), ImGuiComboFlags_None))
                    {
                        for (const auto& MaterialElement : MaterialMap)
                        {
                            {
                                // 머티리얼 애셋 준비
                                UMaterial* Material = MaterialElement.Value;

                                // 작은 색상 미리보기 사각형 표시
                                bool bHasTexturePreview = false;
                                const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();
                                {
                                    // 텍스처가 있는지 확인
                                    if (!MaterialInfo.TextureInfos.IsEmpty() && MaterialInfo.TextureInfos[0].TexturePath.length() > 0)
                                    {
                                        ID3D11ShaderResourceView* TextureSRV = FEngineLoop::ResourceManager.GetTexture(MaterialInfo.TextureInfos[0].TexturePath)->TextureSRV;
                                        if (TextureSRV)
                                        {
                                            ImGui::Image(reinterpret_cast<ImTextureID>(TextureSRV), ImVec2(20, 20));
                                            bHasTexturePreview = true;
                                        }
                                    }

                                    // 텍스처가 없으면 색상 버튼 표시
                                    if (!bHasTexturePreview)
                                    {
                                        FLinearColor MaterialColor(0.8f, 0.8f, 0.8f, 1.0f);
                                        if (MaterialInfo.DiffuseColor.X > 0 ||
                                            MaterialInfo.DiffuseColor.Y > 0 ||
                                            MaterialInfo.DiffuseColor.Z > 0)
                                        {
                                            MaterialColor = FLinearColor(
                                                MaterialInfo.DiffuseColor.X,
                                                MaterialInfo.DiffuseColor.Y,
                                                MaterialInfo.DiffuseColor.Z,
                                                1.0f
                                            );
                                        }

                                        ImGui::ColorButton("##MaterialItemPreview",
                                            ImVec4(MaterialColor.R, MaterialColor.G, MaterialColor.B, MaterialColor.A),
                                            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                                            ImVec2(20, 20)
                                        );
                                    }
                                }
                                ImGui::SameLine();
                                if (ImGui::Selectable(GetData(MaterialElement.Key.ToString()), false))
                                {
                                    if (Material)
                                    {
                                        RequiredModule->Material = Material;
                                    }
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
            }
            break;
        case EPMT_TypeData:
            ImGui::Text("Type Data Module");
            // UParticleModuleTypeDataMesh의 Mesh 속성에 대한 특별 처리
            if (UParticleModuleTypeDataMesh* TypeDataMeshModule = Cast<UParticleModuleTypeDataMesh>(Module))
            {
                ImGui::Separator();
                ImGui::Text("Mesh");
                ImGui::SameLine();
                {
                    FString PreviewName = FString("None");
                    if (UStaticMesh* StaticMesh = TypeDataMeshModule->Mesh)
                    {
                        if (FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData())
                        {
                            PreviewName = RenderData->DisplayName;
                        }
                    }

                    const TMap<FName, FAssetInfo> Assets = UAssetManager::Get().GetAssetRegistry();

                    if (ImGui::BeginCombo("##StaticMesh", GetData(PreviewName), ImGuiComboFlags_None))
                    {
                        for (const auto& Asset : Assets)
                        {
                            if (Asset.Value.AssetType != EAssetType::StaticMesh)
                            {
                                continue;
                            }

                            if (ImGui::Selectable(GetData(Asset.Value.AssetName.ToString()), false))
                            {
                                FString MeshName = Asset.Value.PackagePath.ToString() + "/" + Asset.Value.AssetName.ToString();
                                UStaticMesh* StaticMesh = FObjManager::GetStaticMesh(MeshName.ToWideString());
                                if (!StaticMesh)
                                {
                                    StaticMesh = UAssetManager::Get().GetStaticMesh(MeshName);
                                }

                                if (StaticMesh)
                                {
                                    TypeDataMeshModule->Mesh = StaticMesh;
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
            }
            break;
        case EPMT_Beam:
            ImGui::Text("Beam Module");
            break;
        case EPMT_Trail:
            ImGui::Text("Trail Module");
            break;
        case EPMT_Light:
            ImGui::Text("Light Module");
            break;
        case EPMT_SubUV:
            ImGui::Text("SubUV Module");
            break;
        case EPMT_Event:
            ImGui::Text("Event Module");
            break;
        default:
            ImGui::Text("General Module");
            break;
        }

        ImGui::Separator();
        UClass* Class = nullptr;
        Class = Module->GetClass();
        for (; Class; Class = Class->GetSuperClass())
        {
            const TArray<FProperty*>& Properties = Class->GetProperties();
            if (!Properties.IsEmpty())
            {
                ImGui::SeparatorText(*Class->GetName());
            }

            for (const FProperty* Prop : Properties)
            {
                // UParticleModuleRequired의 Material 속성에 대한 ImGui 렌더링을 건너뜁니다.
                // 이미 위에서 특별 처리했기 때문입니다.
                if (Module->GetModuleType() == EPMT_Required &&
                    strcmp(Prop->Name, "Material") == 0)
                {
                    continue;
                }
                
                Prop->DisplayInImGui(Module);
            }
        }
        ImGui::Separator();

        // LOD 표시
        ImGui::Text("LOD Validity: 0x%02X", Module->LODValidity);

        // 모듈 속성
        if (Module->bSpawnModule)
        {
            ImGui::Text("This module modifies particles during spawning");
        }
        if (Module->bUpdateModule)
        {
            ImGui::Text("This module modifies particles during update");
        }
        if (Module->bFinalUpdateModule)
        {
            ImGui::Text("This module modifies particles during final update");
        }
        if (Module->bUpdateForGPUEmitter)
        {
            ImGui::Text("This module is used for GPU particles");
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}

void PropertyEditorPanel::OnParticleSelectionChanged(const FSelectedObject& Selection)
{
    // 선택 정보 저장
    CurrentParticleSelection = Selection;
}

void PropertyEditorPanel::SetParticleSystemComponent(UParticleSystemComponent* InParticleSystemComponent)
{
    CurrentParticleSystemComponent = InParticleSystemComponent;
}
