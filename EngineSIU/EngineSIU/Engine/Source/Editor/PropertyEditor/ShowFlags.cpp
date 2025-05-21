#include "PropertyEditor/ShowFlags.h"

ShowFlags& ShowFlags::GetInstance()
{
    static ShowFlags Instance;
    return Instance;
}

void ShowFlags::Draw(const std::shared_ptr<FEditorViewportClient>& ActiveViewport)
{
    if (ImGui::Button("Show", ImVec2(60, 32)))
    {
        ImGui::OpenPopup("ShowFlags");
    }

    const char* Items[] = { "AABB", "Primitives", "BillBoardText", "UUID", "Fog", "LightWireframe", "LightWireframeSelectedOnly", "Shadow", "Collision", "CollisionSelectedOnly", "SkeletalMesh", "Particle"};
    const uint64 CurFlag = ActiveViewport->GetShowFlag();

    if (ImGui::BeginPopup("ShowFlags"))
    {
        bool Selected[IM_ARRAYSIZE(Items)] = 
        {
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_AABB),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_Primitives),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_BillboardText),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_UUIDText),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_Fog),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_LightWireframe),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_LightWireframeSelectedOnly),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_Shadow),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_Collision),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_CollisionSelectedOnly),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_SkeletalMesh),
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_Particle),
        }; // 각 항목의 체크 상태 저장

        for (int i = 0; i < IM_ARRAYSIZE(Items); i++)
        {
            ImGui::Checkbox(Items[i], &Selected[i]);
        }

        ActiveViewport->SetShowFlag(ConvertSelectionToFlags(Selected));
        ImGui::EndPopup();
    }
}

uint64 ShowFlags::ConvertSelectionToFlags(const bool Selected[])
{
    uint64 Flags = EEngineShowFlags::None;

    if (Selected[0])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_AABB);
    }
    if (Selected[1])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_Primitives);
    }
    if (Selected[2])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
    }
    if (Selected[3])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_UUIDText);
    }
    if (Selected[4])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_Fog);
    }
    if (Selected[5])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_LightWireframe);
    }
    if (Selected[6])
    {
        // Need SF_LightWireframe to Draw
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_LightWireframe);
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_LightWireframeSelectedOnly);
    }
    if (Selected[7])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_Shadow);
    }
    
    if (Selected[8])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_Collision);
    }
    
    if (Selected[9])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_Collision);
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_CollisionSelectedOnly);
    }

    if (Selected[10])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_SkeletalMesh);
    }
    if (Selected[11])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_Particle);
    }

    return Flags;
}

void ShowFlags::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}
