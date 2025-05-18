#include "ParticleSystemEmittersPanel.h"

void ParticleSystemEmittersPanel::Render()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }

    /* Pre Setup */
    const float PanelWidth = (Width) * 0.7f;
    const float PanelHeight = (Height);

    const float PanelPosX = (Width) * 0.3f;
    const float PanelPosY = 0.f;

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    constexpr ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    if (Engine->ActiveWorld)
    {
        if (Engine->ActiveWorld->WorldType == EWorldType::EditorPreview)
        {
            ImGui::Begin("Emitters", nullptr, PanelFlags);
            ImGui::Text("Particle System Editor Panel");
            // Add your particle system editor UI elements here
            // ...
            ImGui::End();
        }
    }
}

void ParticleSystemEmittersPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = static_cast<float>(ClientRect.right - ClientRect.left);
    Height = static_cast<float>(ClientRect.bottom - ClientRect.top);
}
