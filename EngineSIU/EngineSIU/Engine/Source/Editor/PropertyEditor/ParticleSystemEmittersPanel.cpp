#include "ParticleSystemEmittersPanel.h"

#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleSystem.h"

void ParticleSystemEmittersPanel::Render()
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (!EditorEngine)
    {
        return;
    }

    UParticleSystem* ParticleSystem = ParticleSystemComponent->Template;
    if (!ParticleSystem)
    {
        return;
    }

    {
        /* Pre Setup */
        const ImGuiIO& IO = ImGui::GetIO();
        ImFont* IconFont = IO.Fonts->Fonts[FEATHER_FONT];

        const float PanelWidth = (Width) * 0.7f;
        const float PanelHeight = (Height) - 5.f;

        const float PanelPosX = (Width) * 0.3f;
        const float PanelPosY = 5.f;

        /* Panel Position, Size */
        ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

        constexpr ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
        if (EditorEngine->ActiveWorld)
        {
            if (EditorEngine->ActiveWorld->WorldType == EWorldType::EditorPreview)
            {
                ImGui::Begin("Emitters", nullptr, PanelFlags);
                {
                    ImGui::Text("Particle System Editor Panel");
                    // Add your particle system editor UI elements here
                    // ...
                    RenderEmitters(ParticleSystem);
                }
                ImGui::End();
            }
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
void ParticleSystemEmittersPanel::RenderEmitters(UParticleSystem* ParticleSystem)
{
    if (!ParticleSystem)
    {
        return;
    }

    {
        // 상단 컨트롤 바
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
        {
            // 버튼 높이 지정 (30으로 설정)
            float ButtonHeight = 30.0f;
            {
                // 파티클 시스템 시뮬레이션 버튼
                if (ImGui::Button("Simulate", ImVec2(120, ButtonHeight)))
                {
                    // 시뮬레이션 로직 구현
                }
            }
            ImGui::SameLine();
            {
                // 새 에미터 추가 버튼
                if (ImGui::Button("Add Emitter", ImVec2(120, ButtonHeight)))
                {
                    // 새 에미터 추가 로직 구현
                }
            }
            ImGui::SameLine();
            {
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 130);
                if (ImGui::Button("Exit Viewer", ImVec2(120, ButtonHeight)))
                {
                    UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
                    EdEngine->EndParticleSystemViewer();
                }
            }
        }
        ImGui::PopStyleVar();
    }
    ImGui::Separator();
    {
        // 스타일 설정
        constexpr float EmitterHeaderHeight = 70.0f;
        constexpr ImVec4 HeaderActiveColor(0.2f, 0.4f, 0.8f, 0.9f);
        constexpr ImVec4 HeaderInactiveColor(0.2f, 0.2f, 0.2f, 0.7f);
        constexpr ImVec4 ModuleActiveColor(0.0f, 0.5f, 0.0f, 0.7f);

        // 에미터 목록을 가로로 정렬
        ImGui::BeginChild("EmittersScrollRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            // 각 에미터를 가로로 배치
            for (int32 EmitterIndex = 0; EmitterIndex < ParticleSystem->Emitters.Num(); ++EmitterIndex)
            {
                UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
                if (!Emitter)
                {
                    continue;
                }
                
                // 에미터 패널 크기 설정
                ImGui::BeginGroup();
                {
                    ImVec2 CursorPos = ImGui::GetCursorScreenPos();
                    ImVec2 AvailableRegion = ImVec2(300.0f, EmitterHeaderHeight); // 에미터 헤더 영역
                    
                    // 에미터 헤더 배경 그리기 (활성화 상태에 따라 색상 변경)
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        CursorPos,
                        ImVec2(CursorPos.x + AvailableRegion.x, CursorPos.y + AvailableRegion.y),
                        Emitter->bIsSoloing ? ImGui::ColorConvertFloat4ToU32(HeaderActiveColor) : ImGui::ColorConvertFloat4ToU32(HeaderInactiveColor)
                    );
                    
                    // 에미터 헤더 내용
                    ImGui::BeginChild(("EmitterHeader" + std::to_string(EmitterIndex)).c_str(), AvailableRegion, false);
                    {
                        // 에미터 이름
                        ImGui::Text("%s", GetData(Emitter->EmitterName.ToString()));
                        
                        {
                            // 에미터 활성화 체크박스
                            bool bEmitterEnabled = Emitter->bIsSoloing;
                            if (ImGui::Checkbox(("##EmitterEnabled" + std::to_string(EmitterIndex)).c_str(), &bEmitterEnabled))
                            {
                                Emitter->bIsSoloing = bEmitterEnabled;
                            }
                        }
                        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("000").x - 40.f - ImGui::GetStyle().WindowPadding.x);
                        {
                            // 파티클 카운트 표시
                            ImGui::Text("%d", Emitter->PeakActiveParticles);
                        }
                        // @todo Load Material's Thumbnail
                        ImGui::SameLine(AvailableRegion.x - 50.0f);
                        {
                            // 썸네일 위치
                            ImGui::Dummy(ImVec2(40.0f, 40.0f));
                            ImGui::GetWindowDrawList()->AddRect(
                                ImVec2(CursorPos.x + AvailableRegion.x - 50.0f, CursorPos.y + 10.0f),
                                ImVec2(CursorPos.x + AvailableRegion.x - 10.0f, CursorPos.y + 50.0f),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.5f))
                            );
                        }
                    }
                    ImGui::EndChild();
                    {
                        // 모듈 리스트 렌더
                        RenderModules(Emitter, EmitterIndex);
                    }
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(10.0f, 0)); // 에미터 간 간격
            }
        }
        ImGui::EndChild();
    }
}
    
    void ParticleSystemEmittersPanel::RenderModules(UParticleEmitter* Emitter, int32 EmitterIndex)
    {
        if (!Emitter || Emitter->LODLevels.Num() == 0)
        {
            return;
    }

    // 현재 LOD 레벨 (단순화를 위해 LOD 0만 표시)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        return;
    }

    // 모듈 목록 영역
    ImGui::BeginChild(("ModulesRegion" + std::to_string(EmitterIndex)).c_str(), ImVec2(300.0f, 0.f), true);
    ImGui::Text("Modules:");
    ImGui::Separator();

    // 각 모듈 유형별 렌더링
    RenderModuleCategory(LODLevel->Modules, EmitterIndex);

    ImGui::EndChild();
}

void ParticleSystemEmittersPanel::RenderModuleCategory(const TArray<UParticleModule*>& Modules, int32 EmitterIndex)
{
    if (Modules.Num() == 0)
    {
        return;
    }

    // 각 모듈 표시
    for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ++ModuleIndex)
    {
        UParticleModule* Module = Modules[ModuleIndex];
        if (!Module)
        {
            continue;
        }

        {
            // 모듈 이름
            ImGui::Text("%s", *Module->GetClass()->GetName());
        }
        ImGui::SameLine(ImGui::GetWindowWidth() - 60.0f - ImGui::GetStyle().WindowPadding.x);
        {
            // 모듈 식별자
            const std::string ModuleID = "##Module" + std::to_string(EmitterIndex) + "_" + std::to_string(ModuleIndex);

            // 모듈 활성화 체크박스
            bool bModuleEnabled = Module->bEnabled;
            if (ImGui::Checkbox((ModuleID + "Enabled").c_str(), &bModuleEnabled))
            {
                Module->bEnabled = bModuleEnabled;
            }
        }
        ImGui::SameLine();
        {
            // 모듈 편집 버튼 (그래프 아이콘)
            const ImGuiIO& IO = ImGui::GetIO();
            ImFont* IconFont = IO.Fonts->Fonts[FEATHER_FONT];
            ImGui::PushFont(IconFont);
            {
                if (ImGui::Button("##Edit", ImVec2(25, 25)))
                {
                    // @todo Curve Editor 구현 시에 추가 바람
                    // 모듈 편집 로직 구현
                }
            }
            ImGui::PopFont();
        }
    }

    ImGui::Separator();
}
