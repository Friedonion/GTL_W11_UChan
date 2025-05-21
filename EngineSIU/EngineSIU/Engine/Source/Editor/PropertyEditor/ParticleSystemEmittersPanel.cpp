#include "ParticleSystemEmittersPanel.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleModuleRequired.h"

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
                    
                    // 이미터 이름 변경 모달 처리
                    // ImGui 팝업 중첩 문제를 해결하기 위한 처리
                    // bShowRenameEmitterModal이 true일 때 OpenPopup을 호출하고, 다음 프레임에서 BeginPopupModal로 확인
                    if (bShowRenameEmitterModal)
                    {
                        // OpenPopup은 매 프레임 호출해야 함
                        ImGui::OpenPopup("RenameEmitter");
                    }
                    
                    // BeginPopupModal은 OpenPopup 상태에 관계없이 항상 호출 (팝업 상태 확인용)
                    if (ImGui::BeginPopupModal("RenameEmitter", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::Text("에미터 이름 변경:");
                        ImGui::InputText("##EmitterName", EmitterNameBuffer, sizeof(EmitterNameBuffer));
                        
                        if (ImGui::Button("확인", ImVec2(120, 0)))
                        {
                            // 새 이름이 비어있지 않으면 적용
                            if (strlen(EmitterNameBuffer) > 0 && EmitterToRename)
                            {
                                // 이전 이름 저장 (로그용)
                                FString OldName = EmitterToRename->EmitterName.ToString();
                                
                                // 에미터 이름 업데이트
                                EmitterToRename->EmitterName = EmitterNameBuffer;
                                
                                // 로그 출력
                                UE_LOG(ELogLevel::Display, "[PSV] Rename Emitter : %s -> %s", GetData(OldName), EmitterNameBuffer);
                            }
                            
                            bShowRenameEmitterModal = false;
                            EmitterToRename = nullptr;
                            EmitterToRenameIndex = INDEX_NONE;
                            ImGui::CloseCurrentPopup();
                        }
                        
                        ImGui::SameLine();
                        
                        if (ImGui::Button("취소", ImVec2(120, 0)))
                        {
                            bShowRenameEmitterModal = false;
                            EmitterToRename = nullptr;
                            EmitterToRenameIndex = INDEX_NONE;
                            ImGui::CloseCurrentPopup();
                        }
                        
                        ImGui::EndPopup();
                    }
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
    Selection.ParticleSystem = InParticleSystemComponent ? InParticleSystemComponent->Template : nullptr;
    
    // 이름 변경 모달 상태도 초기화
    bShowRenameEmitterModal = false;
    EmitterToRename = nullptr;
    EmitterToRenameIndex = INDEX_NONE;
    
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
                        
                        // 좌클릭 - 영역 확인 (헤더 영역 전체)
                        if (ImGui::IsItemClicked() || ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(0))
                        {
                            HandleEmitterSelection(Emitter, EmitterIndex);
                        }
                        
                        // 우클릭 - 컨텍스트 메뉴 표시
                        if (ImGui::IsItemClicked(1) || ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(1))
                        {
                            HandleEmitterSelection(Emitter, EmitterIndex);
                            ImGui::OpenPopup(("EmitterContextMenu##" + std::to_string(EmitterIndex)).c_str());
                        }
                        
                        // 에미터 컨텍스트 메뉴 렌더링
                        ShowEmitterContextMenu(Emitter, EmitterIndex);
                        
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
            
            // 우클릭 - 모듈 컨텍스트 메뉴
            if (ImGui::IsItemClicked(1))
            {
                HandleModuleSelection(Module, EmitterIndex, ModuleIndex);
                ImGui::OpenPopup(("ModuleContextMenu##" + std::to_string(EmitterIndex) + "_" + std::to_string(ModuleIndex)).c_str());
            }
            
            // 모듈 컨텍스트 메뉴 렌더링
            ShowModuleContextMenu(Module, EmitterIndex, ModuleIndex);
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

// 에미터 컨텍스트 메뉴
void ParticleSystemEmittersPanel::ShowEmitterContextMenu(UParticleEmitter* Emitter, int32 EmitterIndex)
{
    if (!Emitter)
    {
        return;
    }

    if (ImGui::BeginPopup(("EmitterContextMenu##" + std::to_string(EmitterIndex)).c_str()))
    {
        ImGui::Text("Emitter: %s", GetData(Emitter->EmitterName.ToString()));
        ImGui::Separator();

        // Emitter 카테고리
        if (ImGui::BeginMenu("Emitter"))
        {
            if (ImGui::MenuItem("Rename Emitter"))
            {
                // 이미터 이름변경 모달을 위한 상태 설정
                // 현재 컨텍스트 메뉴를 닫고 다음 프레임에서 모달이 열리도록 함
                ImGui::CloseCurrentPopup();
                OnRenameEmitter(Emitter, EmitterIndex);
            }
            
            if (ImGui::MenuItem("Duplicate Emitter"))
            {
                // 이미터 복제
                OnDuplicateEmitter(Emitter, EmitterIndex);
            }
            
            if (ImGui::MenuItem("Remove Emitter"))
            {
                // 이미터 삭제
                OnRemoveEmitter(Emitter, EmitterIndex);
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        
        // ParticleSystem 카테고리
        if (ImGui::BeginMenu("ParticleSystem"))
        {
            if (ImGui::MenuItem("Select ParticleSystem"))
            {
                // 파티클 시스템 선택
                OnSelectParticleSystem();
            }
            
            if (ImGui::MenuItem("Add New Emitter Before"))
            {
                // 앞에 새 이미터 추가
                OnAddEmitterBefore(EmitterIndex);
            }
            
            if (ImGui::MenuItem("Add New Emitter After"))
            {
                // 뒤에 새 이미터 추가
                OnAddEmitterAfter(EmitterIndex);
            }
            
            if (ImGui::MenuItem("Remove Duplicate Module"))
            {
                // 중복 모듈 제거
                OnRemoveDuplicateModule(Emitter);
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        
        // Modules
        {
            // 컬러 카테고리
            if (ImGui::BeginMenu("Color"))
            {
                if (ImGui::MenuItem("Initial Color"))
                {
                    // 초기 컬러 모듈 추가
                    OnAddInitialColor(Emitter);
                }
                
                if (ImGui::MenuItem("Color Over Life"))
                {
                    // 컬러 오버 라이프 모듈 추가
                    OnAddColorOverLife(Emitter);
                }
                
                ImGui::EndMenu();
            }
            
            // 수명 카테고리
            if (ImGui::BeginMenu("Lifetime"))
            {
                if (ImGui::MenuItem("LifeTime"))
                {
                    // 수명 모듈 추가
                    OnAddLifetime(Emitter);
                }
                
                ImGui::EndMenu();
            }
            
            // 크기 카테고리
            if (ImGui::BeginMenu("Size"))
            {
                if (ImGui::MenuItem("Initial Size"))
                {
                    // 초기 크기 모듈 추가
                    OnAddInitialSize(Emitter);
                }
                
                if (ImGui::MenuItem("Size By Life"))
                {
                    // 라이프 기준 크기 모듈 추가
                    OnAddSizeByLife(Emitter);
                }
                
                ImGui::EndMenu();
            }
            
            // 스폰 카테고리
            if (ImGui::BeginMenu("Spawn"))
            {
                if (ImGui::MenuItem("Spawn Per Unit"))
                {
                    // 단위당 스폰 모듈 추가
                    OnAddSpawnPerUnit(Emitter);
                }
                
                ImGui::EndMenu();
            }
            
            // 속도 카테고리
            if (ImGui::BeginMenu("Velocity"))
            {
                if (ImGui::MenuItem("Initial Velocity"))
                {
                    // 초기 속도 모듈 추가
                    OnAddInitialVelocity(Emitter);
                }
                
                if (ImGui::MenuItem("Velocity/Life"))
                {
                    // 속도/라이프 모듈 추가
                    OnAddVelocityOverLife(Emitter);
                }
                
                ImGui::EndMenu();
            }
        }
        
        ImGui::EndPopup();
    }
}

// 모듈 컨텍스트 메뉴
void ParticleSystemEmittersPanel::ShowModuleContextMenu(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex)
{
    if (!Module)
    {
        return;
    }

    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || EmitterIndex >= ParticleSystem->Emitters.Num())
    {
        return;
    }

    UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
    if (!Emitter)
    {
        return;
    }

    if (ImGui::BeginPopup(("ModuleContextMenu##" + std::to_string(EmitterIndex) + "_" + std::to_string(ModuleIndex)).c_str()))
    {
        FString ModuleTypeName = Module->GetClass()->GetName();
        ImGui::Text("Module: %s", GetData(ModuleTypeName));
        ImGui::Separator();

        // 모듈 활성화/비활성화
        bool bEnabled = Module->bEnabled;
        if (ImGui::MenuItem(bEnabled ? "Disable Module" : "Enable Module"))
        {
            Module->bEnabled = !bEnabled;
        }
        
        // 모듈 복제
        if (ImGui::MenuItem("Duplicate Module"))
        {
            OnDuplicateModule(Module, EmitterIndex, ModuleIndex);
        }
        
        // 모듈 제거
        if (ImGui::MenuItem("Remove Module"))
        {
            OnRemoveModule(Module, EmitterIndex, ModuleIndex);
        }
        
        ImGui::Separator();
        
        // 모듈 이동
        if (ImGui::MenuItem("Move Up", nullptr, false, ModuleIndex > 0))
        {
            OnMoveModuleUp(EmitterIndex, ModuleIndex);
        }
        
        if (ImGui::MenuItem("Move Down", nullptr, false, ModuleIndex < Emitter->LODLevels[0]->Modules.Num() - 1))
        {
            OnMoveModuleDown(EmitterIndex, ModuleIndex);
        }
        
        ImGui::EndPopup();
    }
}

// 에미터 이벤트 핸들러
void ParticleSystemEmittersPanel::OnRenameEmitter(UParticleEmitter* Emitter, int32 EmitterIndex)
{
    if (!Emitter)
    {
        return;
    }
    
    // 다음 프레임에서 모달을 표시하기 위한 상태 설정만 함
    // ImGui는 중첩된 팝업 컨텍스트를 지원하지 않으므로 OpenPopup을 직접 호출하지 않음
    bShowRenameEmitterModal = true;
    EmitterToRename = Emitter;
    EmitterToRenameIndex = EmitterIndex;
    
    // 현재 에미터 이름으로 버퍼 초기화
    strcpy_s(EmitterNameBuffer, sizeof(EmitterNameBuffer), GetData(Emitter->EmitterName.ToString()));
}

void ParticleSystemEmittersPanel::OnDuplicateEmitter(UParticleEmitter* Emitter, int32 EmitterIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || !Emitter)
    {
        return;
    }
    
    // 에미터 복제 로직
    // TODO: 실제 복제 구현
}

void ParticleSystemEmittersPanel::OnRemoveEmitter(UParticleEmitter* Emitter, int32 EmitterIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || !Emitter || EmitterIndex < 0 || EmitterIndex >= ParticleSystem->Emitters.Num())
    {
        return;
    }

    // 1. 선택 상태 확인 및 업데이트
    bool bWasSelected = (Selection.SelectedEmitter == Emitter && Selection.EmitterIndex == EmitterIndex);
    bool bUpdateSelectionIndices = (Selection.EmitterIndex > EmitterIndex);

    // 2. 에미터 인스턴스 제거
    // ParticleSystemComponent에서 에미터 인스턴스 제거
    if (ParticleSystemComponent && EmitterIndex < ParticleSystemComponent->EmitterInstances.Num())
    {
        // 인스턴스 메모리 해제
        FParticleEmitterInstance* Instance = ParticleSystemComponent->EmitterInstances[EmitterIndex];
        if (Instance)
        {
            // 메모리 해제
            delete Instance;
        }

        // 배열에서 인스턴스 제거
        ParticleSystemComponent->EmitterInstances.RemoveAt(EmitterIndex);
    }

    // 3. 파티클 시스템에서 에미터 제거
    ParticleSystem->Emitters.RemoveAt(EmitterIndex);
    
    // 4. 변경사항 반영을 위해 모듈 리스트 업데이트
    ParticleSystem->UpdateAllModuleLists();

    // 5. 선택 상태 업데이트
    if (bWasSelected)
    {
        // 삭제된 에미터가 선택되어 있었으면 선택 해제
        Selection.Reset();
        OnSelectionChanged();
    }
    else if (bUpdateSelectionIndices && Selection.EmitterIndex != INDEX_NONE)
    {
        // 삭제된 에미터 이후의 인덱스를 가진 에미터가 선택되어 있었으면 인덱스 업데이트
        Selection.EmitterIndex--;
    }
}

// 파티클 시스템 이벤트 핸들러
void ParticleSystemEmittersPanel::OnSelectParticleSystem()
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem)
    {
        return;
    }
    
    // 파티클 시스템 선택 로직
    std::cout << "Select ParticleSystem" << std::endl;
    
    // 파티클 시스템 선택 시 모든 선택 초기화 후 전체 시스템 선택
    Selection.Reset();
    Selection.ParticleSystem = ParticleSystem;
    OnSelectionChanged();
}

void ParticleSystemEmittersPanel::OnAddEmitterBefore(int32 EmitterIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem)
    {
        return;
    }
    
    // TODO: 실제 에미터 추가 구현
}

void ParticleSystemEmittersPanel::OnAddEmitterAfter(int32 EmitterIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem)
    {
        return;
    }
    
    // TODO: 실제 에미터 추가 구현
}

void ParticleSystemEmittersPanel::OnRemoveDuplicateModule(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 실제 중복 모듈 찾기 및 제거 구현
}

// 파티클 속성 이벤트 핸들러
void ParticleSystemEmittersPanel::OnAddInitialColor(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 초기 색상 모듈 추가
}

void ParticleSystemEmittersPanel::OnAddColorOverLife(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 색상/수명 모듈 추가
}

void ParticleSystemEmittersPanel::OnAddLifetime(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 수명 모듈 추가
}

void ParticleSystemEmittersPanel::OnAddInitialSize(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 초기 크기 모듈 추가
}

void ParticleSystemEmittersPanel::OnAddSizeByLife(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 크기/수명 모듈 추가
}

void ParticleSystemEmittersPanel::OnAddSpawnPerUnit(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 단위당 스폰 모듈 추가
}

void ParticleSystemEmittersPanel::OnAddInitialVelocity(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // TODO: 초기 속도 모듈 추가
}

void ParticleSystemEmittersPanel::OnAddVelocityOverLife(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }

    // TODO: 속도/수명 모듈 추가
}

// 모듈 이벤트 핸들러
void ParticleSystemEmittersPanel::OnDuplicateModule(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || !Module || EmitterIndex >= ParticleSystem->Emitters.Num())
    {
        return;
    }

    UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // 모듈 복제 로직
    // TODO: 실제 모듈 복제 구현
}

void ParticleSystemEmittersPanel::OnRemoveModule(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || !Module || EmitterIndex >= ParticleSystem->Emitters.Num())
    {
        return;
    }

    UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }
    
    // 모듈 제거 로직
    // TODO: 실제 모듈 제거 구현
}

void ParticleSystemEmittersPanel::OnMoveModuleUp(int32 EmitterIndex, int32 ModuleIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || EmitterIndex >= ParticleSystem->Emitters.Num() || ModuleIndex <= 0)
    {
        return;
    }

    UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }

    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel || ModuleIndex >= LODLevel->Modules.Num())
    {
        return;
    }
    
    // 모듈을 위로 이동
    UParticleModule* CurrentModule = LODLevel->Modules[ModuleIndex];
    UParticleModule* PrevModule = LODLevel->Modules[ModuleIndex - 1];
    
    LODLevel->Modules[ModuleIndex] = PrevModule;
    LODLevel->Modules[ModuleIndex - 1] = CurrentModule;
}

void ParticleSystemEmittersPanel::OnMoveModuleDown(int32 EmitterIndex, int32 ModuleIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || EmitterIndex >= ParticleSystem->Emitters.Num())
    {
        return;
    }

    UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        return;
    }

    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel || ModuleIndex >= LODLevel->Modules.Num() - 1)
    {
        return;
    }
    
    // 모듈을 아래로 이동
    UParticleModule* CurrentModule = LODLevel->Modules[ModuleIndex];
    UParticleModule* NextModule = LODLevel->Modules[ModuleIndex + 1];
    
    LODLevel->Modules[ModuleIndex] = NextModule;
    LODLevel->Modules[ModuleIndex + 1] = CurrentModule;
}
