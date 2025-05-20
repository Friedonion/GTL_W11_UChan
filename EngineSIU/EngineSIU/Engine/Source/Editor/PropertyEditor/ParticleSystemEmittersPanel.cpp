#include "ParticleSystemEmittersPanel.h"

#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleSystem.h"

// 선택 상태 변경 처리 함수
bool ParticleSystemEmittersPanel::HandleEmitterSelection(UParticleEmitter* Emitter, int32 EmitterIndex)
{
    // 이미 선택된 에미터인 경우 선택 해제
    if (Selection.SelectedEmitter == Emitter && Selection.EmitterIndex == EmitterIndex)
    {
        Selection.Reset();
        OnSelectionChanged();
        return true;
    }
    
    // 다른 에미터를 선택하는 경우
    Selection.SelectedEmitter = Emitter;
    Selection.EmitterIndex = EmitterIndex;
    Selection.SelectedModule = nullptr;
    Selection.ModuleIndex = INDEX_NONE;
    OnSelectionChanged();
    return true;
}

bool ParticleSystemEmittersPanel::HandleModuleSelection(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex)
{
    // 이미 선택된 모듈인 경우 선택 해제
    if (Selection.SelectedModule == Module && Selection.ModuleIndex == ModuleIndex &&
        Selection.EmitterIndex == EmitterIndex)
    {
        Selection.SelectedModule = nullptr;
        Selection.ModuleIndex = INDEX_NONE;
        // 모듈 선택 해제 시 에미터 선택 상태는 유지
        OnSelectionChanged();
        return true;
    }
    
    // 새로운 모듈 선택
    UParticleSystemComponent* PSC = ParticleSystemComponent;
    if (PSC && PSC->Template && EmitterIndex < PSC->Template->Emitters.Num())
    {
        // 모듈 선택 시 해당 에미터도 함께 선택
        Selection.SelectedEmitter = PSC->Template->Emitters[EmitterIndex];
        Selection.EmitterIndex = EmitterIndex;
        Selection.SelectedModule = Module;
        Selection.ModuleIndex = ModuleIndex;
        
        std::cout << "Module selected - Associated Emitter: " << GetData(Selection.SelectedEmitter->EmitterName.ToString()) << std::endl;
        std::cout << "Selected Module: " << *Selection.SelectedModule->GetClass()->GetName() << std::endl;
        
        OnSelectionChanged();
        return true;
    }
    
    return false;
}

void ParticleSystemEmittersPanel::OnSelectionChanged()
{
    // 콜백이 설정되어 있으면 호출
    if (SelectionChangedCallback)
    {
        SelectionChangedCallback(Selection, ParticleSystemComponent);
    }
    
    //if (Selection.SelectedEmitter)
    //{
    //    UE_LOG(ELogLevel::Display, TEXT("Selection Changed - Emitter: %s"), GetData(Selection.SelectedEmitter->EmitterName.ToString()));
    //    
    //    // 모듈이 선택된 경우 모듈-에미터 관계 출력
    //    if (Selection.SelectedModule)
    //    {
    //        UE_LOG(ELogLevel::Display, TEXT("  └ Selected Module: %s"), *Selection.SelectedModule->GetClass()->GetName());
    //        UE_LOG(ELogLevel::Display, TEXT("     Module belongs to Emitter: %s"), GetData(Selection.SelectedEmitter->EmitterName.ToString()));
    //    }
    //}
    //else if (Selection.SelectedModule) // 에미터 없이 모듈만 선택된 예외 케이스 (발생해서는 안 됨)
    //{
    //    UE_LOG(ELogLevel::Display, TEXT("WARNING: Module selected without associated emitter!"));
    //    UE_LOG(ELogLevel::Display, TEXT("Selected Module: %s"), *Selection.SelectedModule->GetClass()->GetName());
    //}
}

void ParticleSystemEmittersPanel::Render()
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (!EditorEngine)
    {
        return;
    }

    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem)
    {
        ImGui::Text("No Particle System selected");
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

void ParticleSystemEmittersPanel::SetParticleSystemComponent(UParticleSystemComponent* InParticleSystemComponent)
{
    ParticleSystemComponent = InParticleSystemComponent;
    
    // 컴포넌트가 변경되면 선택 상태 초기화
    Selection.Reset();
    Selection.ParticleSystem = InParticleSystemComponent->Template;
    OnSelectionChanged();
}

UParticleEmitter* ParticleSystemEmittersPanel::FindEmitterForModule(UParticleModule* Module, int32& OutEmitterIndex)
{
    // 기본값 초기화
    OutEmitterIndex = INDEX_NONE;
    
    if (!Module || !ParticleSystemComponent || !ParticleSystemComponent->Template)
    {
        return nullptr;
    }
    
    UParticleSystem* ParticleSystem = ParticleSystemComponent->Template;
    
    // 모든 에미터 검사
    for (int32 EmitterIndex = 0; EmitterIndex < ParticleSystem->Emitters.Num(); ++EmitterIndex)
    {
        UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
        if (!Emitter || Emitter->LODLevels.Num() == 0)
        {
            continue;
        }
        
        // 현재는 LOD 0만 검사 (필요시 모든 LOD 레벨 검사로 확장 가능)
        UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
        if (!LODLevel)
        {
            continue;
        }
        
        // 에미터의 모든 모듈 검사
        for (UParticleModule* EmitterModule : LODLevel->Modules)
        {
            if (EmitterModule == Module)
            {
                // 모듈을 찾았으면 해당 에미터와 인덱스 반환
                OutEmitterIndex = EmitterIndex;
                return Emitter;
            }
        }
    }
    
    // 모듈을 찾지 못한 경우
    return nullptr;
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
                
                // 현재 에미터의 선택 상태 확인
                bool bIsEmitterSelected = (Selection.SelectedEmitter == Emitter && Selection.EmitterIndex == EmitterIndex);
                
                // 현재 에미터에 속한 모듈이 선택되었는지 확인
                bool bHasSelectedModule = (Selection.SelectedModule != nullptr && Selection.EmitterIndex == EmitterIndex);
                
                // 에미터 패널 크기 설정
                ImGui::BeginGroup();
                {
                    ImVec2 CursorPos = ImGui::GetCursorScreenPos();
                    ImVec2 AvailableRegion = ImVec2(300.0f, EmitterHeaderHeight); // 에미터 헤더 영역
                    
                    // 에미터 헤더 배경 그리기 (선택/활성화 상태에 따라 색상 변경)
                    ImVec4 HeaderColor;
                    if (bIsEmitterSelected)
                    {
                        // 에미터가 직접 선택된 경우 - 푸른색
                        HeaderColor = ImVec4(0.3f, 0.5f, 0.9f, 1.0f);
                    }
                    else if (bHasSelectedModule)
                    {
                        // 에미터는 직접 선택되지 않았지만 해당 에미터의 모듈이 선택된 경우 - 녹청색
                        HeaderColor = ImVec4(0.2f, 0.6f, 0.7f, 0.9f);
                    }
                    else
                    {
                        // 선택되지 않은 에미터는 활성화 상태에 따라 색상 다르게
                        HeaderColor = Emitter->bIsSoloing ? HeaderActiveColor : HeaderInactiveColor;
                    }
                    
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        CursorPos,
                        ImVec2(CursorPos.x + AvailableRegion.x, CursorPos.y + AvailableRegion.y),
                        ImGui::ColorConvertFloat4ToU32(HeaderColor)
                    );
                    
                    // 선택된 에미터나 모듈이 있는 에미터는 테두리 표시
                    if (bIsEmitterSelected || bHasSelectedModule)
                    {
                        ImVec4 BorderColor = bIsEmitterSelected ?
                            ImVec4(1.0f, 1.0f, 0.0f, 1.0f) : // 에미터 직접 선택 - 노란색 테두리
                            ImVec4(0.0f, 1.0f, 0.5f, 1.0f);  // 모듈 선택 - 녹색 테두리
                            
                        ImGui::GetWindowDrawList()->AddRect(
                            CursorPos,
                            ImVec2(CursorPos.x + AvailableRegion.x, CursorPos.y + AvailableRegion.y),
                            ImGui::ColorConvertFloat4ToU32(BorderColor),
                            0.0f, 0, 2.0f // 라운딩 없이, 두꺼운 선
                        );
                    }
                    
                    // 에미터 헤더 내용
                    ImGui::BeginChild(("EmitterHeader" + std::to_string(EmitterIndex)).c_str(), AvailableRegion, false);
                    {
                        // 에미터 이름
                        ImGui::Text("%s", GetData(Emitter->EmitterName.ToString()));
                        
                        // 클릭 영역 확인 (헤더 영역 전체)
                        if (ImGui::IsItemClicked() || ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(0))
                        {
                            HandleEmitterSelection(Emitter, EmitterIndex);
                        }
                        
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

        // 현재 모듈이 선택된 상태인지 확인
        bool bIsModuleSelected = (Selection.SelectedModule == Module &&
                                  Selection.EmitterIndex == EmitterIndex &&
                                  Selection.ModuleIndex == ModuleIndex);

        // 선택된 모듈의 배경색 변경
        // 모듈이 선택된 상태에 따라 스타일 조정
        if (bIsModuleSelected)
        {
            // 더 밝고 눈에 띄는 녹색 계열 색상으로 변경
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.9f, 0.4f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        
        // 전체 행을 버튼처럼 보이게 만들기 위한 배경
        float RowHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
        ImVec2 CursorPos = ImGui::GetCursorScreenPos();
        ImVec2 RowEnd = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), CursorPos.y + RowHeight);
        
        // 모듈 행 배경 그리기
        if (bIsModuleSelected)
        {
            // 선택된 모듈은 더 뚜렷한 배경색과 테두리
            ImGui::GetWindowDrawList()->AddRectFilled(
                CursorPos,
                RowEnd,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.7f, 0.2f, 0.5f)) // 불투명도 증가
            );
            
            // 선택된 모듈에 테두리 추가
            ImGui::GetWindowDrawList()->AddRect(
                CursorPos,
                RowEnd,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.5f, 0.8f)), // 청녹색 테두리
                0.0f, 0, 1.5f // 테두리 두께
            );
        }
        else if (ImGui::IsMouseHoveringRect(CursorPos, RowEnd))
        {
            // 마우스 오버 효과
            ImGui::GetWindowDrawList()->AddRectFilled(
                CursorPos,
                RowEnd,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 0.3f))
            );
        }

        {
            // 모듈 이름을 클릭 가능한 버튼으로 변경
            FString ModuleName = Module->GetClass()->GetName();
            FString ButtonLabel = ModuleName + "##ModuleSelectBtn" + FString::Printf(TEXT("%d"), ModuleIndex);
            if (ImGui::Button(GetData(ButtonLabel), ImVec2(ImGui::GetWindowWidth() - 65.0f, 0)))
            {
                HandleModuleSelection(Module, EmitterIndex, ModuleIndex);
            }
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
                    // 모듈을 선택하고 에디터 열기
                    HandleModuleSelection(Module, EmitterIndex, ModuleIndex);
                    // @todo Curve Editor 구현 시에 추가 바람
                    // 모듈 편집 로직 구현
                }
            }
            ImGui::PopFont();
        }
        
        // 스타일 복원
        if (bIsModuleSelected)
        {
            ImGui::PopStyleColor(4);
        }
    }

    ImGui::Separator();
}
