#include "ParticleSystemEmittersPanel.h"

void ParticleSystemEmittersPanel::Render()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }

    {
        /* Pre Setup */
        const float PanelWidth = (Width) * 0.7f;
        const float PanelHeight = (Height) - 5.f;

        const float PanelPosX = (Width) * 0.3f;
        const float PanelPosY = 5.f;

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

    {
        float ExitPanelWidth = (Width) * 0.2f - 6.0f;
        float ExitPanelHeight = 30.0f;

        float ExitPanelPosX = Width - ExitPanelWidth;
        float ExitPanelPosY = Height - ExitPanelHeight - 10;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::SetNextWindowSize(ImVec2(ExitPanelWidth, ExitPanelHeight), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ExitPanelPosX, ExitPanelPosY), ImGuiCond_Always);

        constexpr ImGuiWindowFlags ExitPanelFlags =
            ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("Exit Viewer", nullptr, ExitPanelFlags);
        if (ImGui::Button("Exit Viewer", ImVec2(ExitPanelWidth, ExitPanelHeight)))
        {
            UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
            EdEngine->EndParticleSystemViewer();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

void ParticleSystemEmittersPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = static_cast<float>(ClientRect.right - ClientRect.left);
    Height = static_cast<float>(ClientRect.bottom - ClientRect.top);
}
