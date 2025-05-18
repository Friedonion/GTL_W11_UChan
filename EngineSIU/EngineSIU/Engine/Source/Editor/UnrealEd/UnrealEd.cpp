#include "UnrealEd.h"
#include "EditorPanel.h"

#include "PropertyEditor/ControlEditorPanel.h"
#include "PropertyEditor/OutlinerEditorPanel.h"
#include "PropertyEditor/PropertyEditorPanel.h"
#include "PropertyEditor/SkeletalMeshViewerPanel.h"
#include "PropertyEditor/ParticleSystemEmittersPanel.h"
#include "World/World.h"
void UnrealEd::Initialize()
{
    /* Main Editor */
    auto ControlPanel = std::make_shared<ControlEditorPanel>();
    ControlPanel->SetSupportedWorldTypes(EWorldTypeBitFlag::Editor | EWorldTypeBitFlag::PIE | EWorldTypeBitFlag::EditorPreview);
    Panels["ControlPanel"] = ControlPanel;
    
    auto OutlinerPanel = std::make_shared<OutlinerEditorPanel>();
    OutlinerPanel->SetSupportedWorldTypes(EWorldTypeBitFlag::Editor | EWorldTypeBitFlag::PIE);
    Panels["OutlinerPanel"] = OutlinerPanel;
    
    auto PropertyPanel = std::make_shared<PropertyEditorPanel>();
    PropertyPanel->SetSupportedWorldTypes(EWorldTypeBitFlag::Editor | EWorldTypeBitFlag::PIE);
    Panels["PropertyPanel"] = PropertyPanel;

    /* SkeletalMesh */
    auto SkeletalMeshPropertyPanel = std::make_shared<PropertyEditorPanel>();
    SkeletalMeshPropertyPanel->SetSupportedWorldTypes(EWorldTypeBitFlag::EditorPreview);
    SkeletalMeshPropertyPanel->SetPreviewType(EPreviewTypeBitFlag::SkeletalMesh);
    Panels["SkeletalMeshPropertyPanel"] = SkeletalMeshPropertyPanel;

    /* AnimSequence */
    // TODO : SkeletalViewer 전용 UI 분리
    auto BoneHierarchyAnimPanel = std::make_shared<SkeletalMeshViewerPanel>();
    BoneHierarchyAnimPanel->SetSupportedWorldTypes(EWorldTypeBitFlag::EditorPreview);
    BoneHierarchyAnimPanel->SetPreviewType(EPreviewTypeBitFlag::AnimSequence);
    Panels["BoneHierarchyPanel"] = BoneHierarchyAnimPanel;

    /* Particle System */
    auto ParticleDetailPanel = std::make_shared<PropertyEditorPanel>();
    ParticleDetailPanel->SetSupportedWorldTypes(EWorldTypeBitFlag::EditorPreview);
    ParticleDetailPanel->SetPreviewType(EPreviewTypeBitFlag::ParticleSystem);
    Panels["ParticleDetailPanel"] = ParticleDetailPanel;

    auto ParticleEmittersPanel = std::make_shared<ParticleSystemEmittersPanel>();
    ParticleEmittersPanel->SetSupportedWorldTypes(EWorldTypeBitFlag::EditorPreview);
    ParticleEmittersPanel->SetPreviewType(EPreviewTypeBitFlag::ParticleSystem);
    Panels["ParticleSystemEmittersPanel"] = ParticleEmittersPanel;
}

void UnrealEd::Render() const
{
    EWorldTypeBitFlag currentMask;
    switch (GEngine->ActiveWorld->WorldType)
    {
    case EWorldType::Game:
        currentMask = EWorldTypeBitFlag::Game;
        break;
    case EWorldType::Editor:
        currentMask = EWorldTypeBitFlag::Editor;
        break;
    case EWorldType::PIE:
        currentMask = EWorldTypeBitFlag::PIE;
        break;
    case EWorldType::EditorPreview:
        currentMask = EWorldTypeBitFlag::EditorPreview;
        break;
    case EWorldType::GamePreview:
        currentMask = EWorldTypeBitFlag::GamePreview;
        break;
    case EWorldType::GameRPC:
        currentMask = EWorldTypeBitFlag::GameRPC;
        break;
    case EWorldType::Inactive:
        currentMask = EWorldTypeBitFlag::Inactive;
        break;
    case EWorldType::None:
    default:
        currentMask = EWorldTypeBitFlag::None;
        break;
    }
    for (const auto& Panel : Panels)
    {
        if (HasFlag(Panel.Value->GetSupportedWorldTypes(), currentMask))
        {
            if (currentMask == EWorldTypeBitFlag::EditorPreview)
            {
                // @todo PreviewType을 알 수 있는 방법 추가하기
                if (HasFlag(Panel.Value->GetPreviewType(), EPreviewTypeBitFlag::AnimSequence))
                {
                    Panel.Value->Render();
                }
            }
            else
            {
                Panel.Value->Render();
            }
        }
    }
}

void UnrealEd::AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel)
{
    Panels[PanelId] = EditorPanel;
}

void UnrealEd::OnResize(HWND hWnd) const
{
    for (auto& Panel : Panels)
    {
        Panel.Value->OnResize(hWnd);
    }
}

std::shared_ptr<UEditorPanel> UnrealEd::GetEditorPanel(const FString& PanelId)
{
    return Panels[PanelId];
}
