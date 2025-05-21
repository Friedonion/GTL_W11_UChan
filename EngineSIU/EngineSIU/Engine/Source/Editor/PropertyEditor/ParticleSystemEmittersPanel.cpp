#include "ParticleSystemEmittersPanel.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/TypeData/ParticleModuleTypeDataMesh.h"
#include "Particles/Lifetime/ParticleModuleLifetime.h"
#include "Particles/Size/ParticleModuleSize.h"
#include "Particles/Velocity/ParticleModuleVelocity.h"
#include "Particles/Color/ParticleModuleColor.h"

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
                if (ImGui::Button("[X] Simulate", ImVec2(120, ButtonHeight)))
                {
                    // 시뮬레이션 로직 구현
                }
            }
            ImGui::SameLine();
            {
                // 새 에미터 추가 버튼
                if (ImGui::Button("Add Emitter", ImVec2(120, ButtonHeight)))
                {
                    // Last EmitterIndex -> ParticleSystem->Emitters.Num() - 1
                    OnAddEmitterAfter(ParticleSystem->Emitters.Num() - 1);
                }
            }
            ImGui::SameLine();
            {
                // 에미터 복제 버튼
                if (ImGui::Button("[X] Duplicate Emitter", ImVec2(120, ButtonHeight)))
                {
                    OnDuplicateEmitter(Selection.SelectedEmitter, Selection.EmitterIndex);
                }
            }
            ImGui::SameLine();
            {
                // 에미터 삭제 버튼
                if (ImGui::Button("Remove Emitter", ImVec2(120, ButtonHeight)))
                {
                    OnRemoveEmitter(Selection.SelectedEmitter, Selection.EmitterIndex);
                }
            }
            ImGui::SameLine();
            {
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 130);
                if (ImGui::Button("Exit Viewer", ImVec2(120, ButtonHeight)))
                {
                    ParticleSystemComponent = nullptr;
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

        // 에미터 목록을 가로로 정렬하는 스크롤 영역 생성
        ImGui::BeginChild("EmittersScrollRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            // 전체 에미터 레이아웃을 담을 그룹
            ImGui::BeginGroup();
            
            // 에미터 항목의 고정 너비 설정 (에미터 헤더 + 모듈 영역)
            constexpr float EmitterItemWidth = 300.0f;
            constexpr float EmitterItemSpacing = 5.0f; // 에미터 간 간격 증가
            
            // 에미터 항목의 시작 위치에 커서 설정
            ImVec2 StartPos = ImGui::GetCursorPos();

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
                ImGui::SetCursorPos(ImVec2(StartPos.x + static_cast<float>(EmitterIndex) * (EmitterItemWidth + EmitterItemSpacing), StartPos.y));
                ImGui::BeginGroup();
                {
                    ImVec2 CursorPos = ImGui::GetCursorScreenPos();
                    ImVec2 AvailableRegion = ImVec2(ContentWidth, EmitterHeaderHeight); // 에미터 헤더 영역
                    
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
                        ImGui::SameLine(AvailableRegion.x - 50.0f);
                        {
                            // 썸네일
                            bool bHasTexture = false;
                            if (auto Material = Emitter->LODLevels[0]->RequiredModule->Material)
                            {

                                if (auto TexturePath = Material->GetMaterialInfo().TextureInfos[0].TexturePath; !TexturePath.empty())
                                {
                                    ID3D11ShaderResourceView* TextureSRV = FEngineLoop::ResourceManager.GetTexture(Emitter->LODLevels[0]->RequiredModule->Material->GetMaterialInfo().TextureInfos[0].TexturePath)->TextureSRV;
                                    if (TextureSRV)
                                    {
                                        ImGui::Image(reinterpret_cast<ImTextureID>(TextureSRV), ImVec2(40, 40));
                                        bHasTexture = true;
                                    }
                                }
                                else
                                {
                                    FLinearColor MaterialColor(0.8f, 0.8f, 0.8f, 1.0f);
                                    if (Material->GetMaterialInfo().DiffuseColor.X > 0 ||
                                        Material->GetMaterialInfo().DiffuseColor.Y > 0 ||
                                        Material->GetMaterialInfo().DiffuseColor.Z > 0)
                                    {
                                        MaterialColor = FLinearColor(
                                            Material->GetMaterialInfo().DiffuseColor.X,
                                            Material->GetMaterialInfo().DiffuseColor.Y,
                                            Material->GetMaterialInfo().DiffuseColor.Z,
                                            1.0f
                                        );
                                    }

                                    ImGui::ColorButton("##MaterialItemPreview",
                                        ImVec4(MaterialColor.R, MaterialColor.G, MaterialColor.B, MaterialColor.A),
                                        ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                                        ImVec2(40, 40));
                                }
                            }
                            if (!bHasTexture)
                            {
                                ImGui::Dummy(ImVec2(40, 40));
                            }

                            // 테두리
                            ImGui::GetWindowDrawList()->AddRect(
                                ImVec2(CursorPos.x + AvailableRegion.x - 50.0f, CursorPos.y + 22.0f),
                                ImVec2(CursorPos.x + AvailableRegion.x - 10.0f, CursorPos.y + 62.0f),
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
            }
            // 전체 에미터 레이아웃 그룹 종료
            ImGui::EndGroup();
            
            // 스크롤 영역에 필요한 총 너비 계산 (마지막 에미터까지의 너비)
            float TotalContentWidth = static_cast<float>(ParticleSystem->Emitters.Num()) * (EmitterItemWidth + EmitterItemSpacing);
            
            // 스크롤 영역에 컨텐츠 크기 설정
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::Dummy(ImVec2(TotalContentWidth, 1)); // 가로 스크롤을 위한 더미 컨텐츠
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

    // 모듈 목록 영역 - 에미터 헤더와 동일한 너비 사용
    // 고정 너비를 사용하여 각 에미터의 모듈 영역이 일정하게 유지되도록 함
    ImGui::BeginChild(("ModulesRegion" + std::to_string(EmitterIndex)).c_str(), ImVec2(ContentWidth, 0.f), true);
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
            
            if (ImGui::MenuItem("[X] Duplicate Emitter"))
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
            
            if (ImGui::MenuItem("[X] Remove Duplicate Module"))
            {
                // 중복 모듈 제거
                OnRemoveDuplicateModule(Emitter);
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        
        // Modules
        {
            // 타입 데이터 카테고리
            if (ImGui::BeginMenu("Type Data"))
            {
                if (ImGui::MenuItem("New Mesh Data"))
                {
                    OnAddMeshData(Emitter);
                }

                ImGui::EndMenu();
            }
            // 컬러 카테고리
            if (ImGui::BeginMenu("Color"))
            {
                if (ImGui::MenuItem("Initial Color"))
                {
                    // 초기 컬러 모듈 추가
                    OnAddInitialColor(Emitter);
                }
                
                if (ImGui::MenuItem("[X] Color Over Life"))
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
                
                if (ImGui::MenuItem("[X] Size By Life"))
                {
                    // 라이프 기준 크기 모듈 추가
                    OnAddSizeByLife(Emitter);
                }
                
                ImGui::EndMenu();
            }
            
            // 스폰 카테고리
            if (ImGui::BeginMenu("Spawn"))
            {
                if (ImGui::MenuItem("[X] Spawn Per Unit"))
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
                
                if (ImGui::MenuItem("[X] Velocity/Life"))
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
    if (!ParticleSystem)
    {
        return;
    }
    if (EmitterIndex >= ParticleSystem->Emitters.Num())
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
    // 파티클 시스템 컴포넌트와 템플릿 가져오기
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem)
    {
        UE_LOG(ELogLevel::Warning, "Invalid ParticleSystemComponent or Template.");
        return;
    }
    
    Selection.Reset();
    Selection.ParticleSystem = ParticleSystem;
    
    // 파티클 시스템의 모든 모듈 리스트 업데이트
    ParticleSystem->UpdateAllModuleLists();
    
    // 에미터 인스턴스 업데이트
    if (ParticleSystemComponent)
    {
        // 필요한 경우 파티클 시스템 컴포넌트의 에미터 인스턴스 상태 확인 및 업데이트
        for (int32 EmitterIndex = 0; EmitterIndex < ParticleSystem->Emitters.Num(); ++EmitterIndex)
        {
            if (EmitterIndex >= ParticleSystemComponent->EmitterInstances.Num() ||
                !ParticleSystemComponent->EmitterInstances[EmitterIndex])
            {
                // 필요시 인스턴스 초기화 로직을 여기에 추가할 수 있음
                // 현재 구현은 ParticleSystemComponent에서 처리하는 것으로 가정
            }
        }
    }
    
    // UE_LOG(ELogLevel::Display, TEXT("파티클 시스템 선택: %s (에미터 수: %d)"),
    //      *ParticleSystem->GetName(), ParticleSystem->Emitters.Num());
    
    // for (int32 i = 0; i < ParticleSystem->Emitters.Num(); ++i)
    // {
    //     if (ParticleSystem->Emitters[i])
    //     {
    //         UE_LOG(ELogLevel::Display, TEXT("  에미터[%d]: %s"),
    //             i, GetData(ParticleSystem->Emitters[i]->EmitterName.ToString()));
    //     }
    // }
    
    OnSelectionChanged();
}

void ParticleSystemEmittersPanel::OnAddEmitterBefore(int32 EmitterIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add emitter: Invalid ParticleSystem");
        return;
    }
    
    // 인덱스 범위 검증
    if (EmitterIndex < 0 || EmitterIndex > ParticleSystem->Emitters.Num())
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add emitter: Invalid EmitterIndex %d", EmitterIndex);
        return;
    }
    
    // 1. 새 에미터(UParticleSpriteEmitter) 생성
    UParticleSpriteEmitter* NewEmitter = FObjectFactory::ConstructObject<UParticleSpriteEmitter>(nullptr);
    if (!NewEmitter)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create new emitter");
        return;
    }
    
    // 2. LOD 레벨 생성
    NewEmitter->CreateLODLevel(0);
    UParticleLODLevel* LODLevel = NewEmitter->GetLODLevel(0);
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create LOD level for new emitter");
        return;
    }
    
    // 3. 기본 모듈 추가
    // Lifetime 모듈 추가
    UParticleModuleLifetime* LifetimeModule = FObjectFactory::ConstructObject<UParticleModuleLifetime>(nullptr);
    LODLevel->Modules.Add(LifetimeModule);
    
    // Initial Size 모듈 추가
    UParticleModuleSize* SizeModule = FObjectFactory::ConstructObject<UParticleModuleSize>(nullptr);
    LODLevel->Modules.Add(SizeModule);
    
    // Initial Velocity 모듈 추가
    UParticleModuleVelocity* VelocityModule = FObjectFactory::ConstructObject<UParticleModuleVelocity>(nullptr);
    LODLevel->Modules.Add(VelocityModule);

    // Color Over Life 모듈 추가
    UParticleModuleColor* ColorModule = FObjectFactory::ConstructObject<UParticleModuleColor>(nullptr);
    LODLevel->Modules.Add(ColorModule);
    
    // 4. 에미터 이름 설정
    FString NewEmitterName = TEXT("Particle Emitter_") + FString::Printf(TEXT("%d"), ParticleSystem->Emitters.Num());
    NewEmitter->EmitterName = NewEmitterName;
    
    // 5. 파티클 시스템에 에미터 추가 (앞에 추가)
    ParticleSystem->Emitters.Insert(NewEmitter, EmitterIndex);
    
    // 선택 상태 업데이트
    bool bUpdateSelectionIndices = (Selection.EmitterIndex >= EmitterIndex);
    
    // 6. 에미터 인스턴스 생성 및 추가
    FParticleEmitterInstance* NewInstance = NewEmitter->CreateInstance(ParticleSystemComponent);
    if (NewInstance)
    {
        ParticleSystemComponent->EmitterInstances.Insert(NewInstance, EmitterIndex);
    }
    
    // 7. 모듈 리스트 업데이트
    ParticleSystem->UpdateAllModuleLists();
    
    // 8. 선택 상태 업데이트
    if (bUpdateSelectionIndices && Selection.EmitterIndex != INDEX_NONE)
    {
        // 추가된 에미터 이후의 인덱스를 가진 에미터가 선택되어 있었으면 인덱스 업데이트
        Selection.EmitterIndex++;
    }
    
    // 9. 새 에미터를 선택 상태로 변경
    Selection.SelectedEmitter = NewEmitter;
    Selection.EmitterIndex = EmitterIndex;
    Selection.SelectedModule = nullptr;
    Selection.ModuleIndex = INDEX_NONE;
    
    // 선택 변경 이벤트 발생
    OnSelectionChanged();
}

void ParticleSystemEmittersPanel::OnAddEmitterAfter(int32 EmitterIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add emitter: Invalid ParticleSystem");
        return;
    }

    if (ParticleSystem->Emitters.Num() != 0)
    {
        // 인덱스 범위 검증
        if (EmitterIndex < 0 || EmitterIndex >= ParticleSystem->Emitters.Num())
        {
            UE_LOG(ELogLevel::Warning, "[PSV] Cannot add emitter: Invalid EmitterIndex %d", EmitterIndex);
            return;
        }
    }
    
    // 1. 새 에미터(UParticleSpriteEmitter) 생성
    UParticleSpriteEmitter* NewEmitter = FObjectFactory::ConstructObject<UParticleSpriteEmitter>(nullptr);
    if (!NewEmitter)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create new emitter");
        return;
    }
    
    // 2. LOD 레벨 생성
    NewEmitter->CreateLODLevel(0);
    UParticleLODLevel* LODLevel = NewEmitter->GetLODLevel(0);
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create LOD level for new emitter");
        return;
    }
    
    // 3. 기본 모듈 추가
    // Lifetime 모듈 추가
    UParticleModuleLifetime* LifetimeModule = FObjectFactory::ConstructObject<UParticleModuleLifetime>(nullptr);
    LODLevel->Modules.Add(LifetimeModule);
    
    // Initial Size 모듈 추가
    UParticleModuleSize* SizeModule = FObjectFactory::ConstructObject<UParticleModuleSize>(nullptr);
    LODLevel->Modules.Add(SizeModule);
    
    // Initial Velocity 모듈 추가
    UParticleModuleVelocity* VelocityModule = FObjectFactory::ConstructObject<UParticleModuleVelocity>(nullptr);
    LODLevel->Modules.Add(VelocityModule);

    // Color 모듈 추가
    UParticleModuleColor* ColorModule = FObjectFactory::ConstructObject<UParticleModuleColor>(nullptr);
    LODLevel->Modules.Add(ColorModule);

    // 4. 에미터 이름 설정
    FString NewEmitterName = TEXT("Particle Emitter");
    NewEmitter->EmitterName = NewEmitterName;
    
    // 5. 파티클 시스템에 에미터 추가 (뒤에 추가)
    int32 NewEmitterIndex = EmitterIndex + 1;
    ParticleSystem->Emitters.Insert(NewEmitter, NewEmitterIndex);
    
    // 선택 상태 업데이트
    bool bUpdateSelectionIndices = (Selection.EmitterIndex >= NewEmitterIndex);
    
    // 6. 에미터 인스턴스 생성 및 추가
    FParticleEmitterInstance* NewInstance = NewEmitter->CreateInstance(ParticleSystemComponent);
    if (NewInstance)
    {
        ParticleSystemComponent->EmitterInstances.Insert(NewInstance, NewEmitterIndex);
    }
    
    // 7. 모듈 리스트 업데이트
    ParticleSystem->UpdateAllModuleLists();
    
    // 8. 선택 상태 업데이트
    if (bUpdateSelectionIndices && Selection.EmitterIndex != INDEX_NONE)
    {
        // 추가된 에미터 이후의 인덱스를 가진 에미터가 선택되어 있었으면 인덱스 업데이트
        Selection.EmitterIndex++;
    }
    
    // 9. 새 에미터를 선택 상태로 변경
    Selection.SelectedEmitter = NewEmitter;
    Selection.EmitterIndex = NewEmitterIndex;
    Selection.SelectedModule = nullptr;
    Selection.ModuleIndex = INDEX_NONE;
    
    // 선택 변경 이벤트 발생
    OnSelectionChanged();
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
void ParticleSystemEmittersPanel::OnAddMeshData(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add Mesh Data module: Invalid Emitter or no LOD levels");
        return;
    }

    // 현재 LOD 레벨 (단순화를 위해 LOD 0만 처리)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Cannot add Mesh Data module: Invalid LOD level");
        return;
    }

    // 메쉬 데이터 모듈 생성
    UParticleModuleTypeDataMesh* TypeDataMeshModule = FObjectFactory::ConstructObject<UParticleModuleTypeDataMesh>(nullptr);
    if (!TypeDataMeshModule)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create Mesh Data module");
        return;
    }

    // 기본 속성 설정

    // 모듈 활성화
    TypeDataMeshModule->bEnabled = true;

    // LOD 레벨에 모듈 추가
    LODLevel->TypeDataModule = TypeDataMeshModule;
    LODLevel->Modules.Add(TypeDataMeshModule);

    UE_LOG(ELogLevel::Display, "[PSV] Added Type Data Mesh module to emitter: %s", GetData(Emitter->EmitterName.ToString()));
}
void ParticleSystemEmittersPanel::OnAddInitialColor(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add Initial Color module: Invalid Emitter or no LOD levels");
        return;
    }
    
    // 현재 LOD 레벨 (단순화를 위해 LOD 0만 처리)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Cannot add Initial Color module: Invalid LOD level");
        return;
    }
    
    // 초기 색상 모듈 생성
    UParticleModuleColor* ColorModule = FObjectFactory::ConstructObject<UParticleModuleColor>(nullptr);
    if (!ColorModule)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create Initial Color module");
        return;
    }
    
    // 기본 속성 설정
    //ColorModule->StartColor.Distribution.Mode = EDistributionParamMode::DPM_Constant;
    ColorModule->StartColor.Distribution->Constant = FVector(1.0f, 1.0f, 1.0f); // 흰색
    
    //ColorModule->StartAlpha.Distribution.Mode = EDistributionParamMode::DPM_Constant;
    ColorModule->StartAlpha.Distribution->Constant = 1.0f;
    
    // 모듈 활성화
    ColorModule->bEnabled = true;
    
    // LOD 레벨에 모듈 추가
    LODLevel->Modules.Add(ColorModule);
    
    UE_LOG(ELogLevel::Display, "[PSV] Added Initial Color module to emitter: %s", GetData(Emitter->EmitterName.ToString()));
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
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add Lifetime module: Invalid Emitter or no LOD levels");
        return;
    }
    
    // 현재 LOD 레벨 (단순화를 위해 LOD 0만 처리)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Cannot add Lifetime module: Invalid LOD level");
        return;
    }
    
    // 수명 모듈 생성
    UParticleModuleLifetime* LifetimeModule = FObjectFactory::ConstructObject<UParticleModuleLifetime>(nullptr);
    if (!LifetimeModule)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create Lifetime module");
        return;
    }
    
    // 기본 속성 설정 - 기본 파티클 수명은 1초로 설정
    LifetimeModule->Lifetime = 1.0f;
    
    // 모듈 활성화
    LifetimeModule->bEnabled = true;
    
    // LOD 레벨에 모듈 추가
    LODLevel->Modules.Add(LifetimeModule);
    
    UE_LOG(ELogLevel::Display, "[PSV] Added Lifetime module to emitter: %s", GetData(Emitter->EmitterName.ToString()));
}

void ParticleSystemEmittersPanel::OnAddInitialSize(UParticleEmitter* Emitter)
{
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add Initial Size module: Invalid Emitter or no LOD levels");
        return;
    }
    
    // 현재 LOD 레벨 (단순화를 위해 LOD 0만 처리)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Cannot add Initial Size module: Invalid LOD level");
        return;
    }
    
    // 초기 크기 모듈 생성
    UParticleModuleSize* SizeModule = FObjectFactory::ConstructObject<UParticleModuleSize>(nullptr);
    if (!SizeModule)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create Initial Size module");
        return;
    }
    
    // 기본 속성 설정 - 기본 크기는 (10, 10, 10) 벡터로 설정
    SizeModule->StartSize = FVector(1.0f, 1.0f, 1.0f);
    
    // 모듈 활성화
    SizeModule->bEnabled = true;
    
    // LOD 레벨에 모듈 추가
    LODLevel->Modules.Add(SizeModule);
    
    UE_LOG(ELogLevel::Display, "[PSV] Added Initial Size module to emitter: %s", GetData(Emitter->EmitterName.ToString()));
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
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot add Initial Velocity module: Invalid Emitter or no LOD levels");
        return;
    }
    
    // 현재 LOD 레벨 (단순화를 위해 LOD 0만 처리)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Cannot add Initial Velocity module: Invalid LOD level");
        return;
    }
    
    // 초기 속도 모듈 생성
    UParticleModuleVelocity* VelocityModule = FObjectFactory::ConstructObject<UParticleModuleVelocity>(nullptr);
    if (!VelocityModule)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create Initial Velocity module");
        return;
    }
    
    // 기본 속성 설정
    //VelocityModule->StartVelocity.Mode = EDistributionParamMode::DPM_Constant;
    VelocityModule->StartVelocity.Distribution->Constant = FVector(1.0f, 1.0f, 1.0f);
    
    //VelocityModule->StartVelocityRadial.Mode = EDistributionParamMode::DPM_Constant;
    VelocityModule->StartVelocityRadial = 0.0f;
    
    // 모듈 활성화
    VelocityModule->bEnabled = true;
    
    // LOD 레벨에 모듈 추가
    LODLevel->Modules.Add(VelocityModule);
    
    UE_LOG(ELogLevel::Display, "[PSV] Added Initial Velocity module to emitter: %s", GetData(Emitter->EmitterName.ToString()));
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
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot duplicate module: Invalid ParticleSystem, Module, or EmitterIndex");
        return;
    }

    UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot duplicate module: Invalid Emitter or no LOD levels");
        return;
    }

    // LOD 레벨 가져오기 (현재 구현에서는 LOD 0만 사용)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot duplicate module: Invalid LOD level");
        return;
    }

    // 모듈 인덱스 검증
    if (ModuleIndex < 0 || ModuleIndex >= LODLevel->Modules.Num())
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot duplicate module: Invalid ModuleIndex %d", ModuleIndex);
        return;
    }

    // 모듈 클래스 획득
    UClass* ModuleClass = Module->GetClass();
    if (!ModuleClass)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Cannot duplicate module: Failed to get module class");
        return;
    }

    // 새 모듈 인스턴스 생성
    UParticleModule* NewModule = FObjectFactory::ConstructObject<UParticleModule>(ModuleClass);
    if (!NewModule)
    {
        UE_LOG(ELogLevel::Error, "[PSV] Failed to create duplicate module");
        return;
    }

    // 원본 모듈의 속성 복사
    // 간단한 복사 대신 실제로는 모듈 타입에 따라 특정 속성들을 복사해야 할 수 있음
    NewModule->bEnabled = Module->bEnabled;

    // 모듈 특성에 따른 추가 속성 복사
    // 모듈 타입별 특수 처리
    if (UParticleModuleLifetime* SourceLifetime = Cast<UParticleModuleLifetime>(Module))
    {
        UParticleModuleLifetime* DestLifetime = Cast<UParticleModuleLifetime>(NewModule);
        if (DestLifetime)
        {
            DestLifetime->Lifetime = SourceLifetime->Lifetime;
        }
    }
    else if (UParticleModuleSize* SourceSize = Cast<UParticleModuleSize>(Module))
    {
        UParticleModuleSize* DestSize = Cast<UParticleModuleSize>(NewModule);
        if (DestSize)
        {
            DestSize->StartSize = SourceSize->StartSize;
        }
    }
    else if (UParticleModuleVelocity* SourceVelocity = Cast<UParticleModuleVelocity>(Module))
    {
        UParticleModuleVelocity* DestVelocity = Cast<UParticleModuleVelocity>(NewModule);
        if (DestVelocity)
        {
            DestVelocity->StartVelocity = SourceVelocity->StartVelocity;
            DestVelocity->StartVelocityRadial = SourceVelocity->StartVelocityRadial;
        }
    }
    else if (UParticleModuleColor* SourceColor = Cast<UParticleModuleColor>(Module))
    {
        UParticleModuleColor* DestColor = Cast<UParticleModuleColor>(NewModule);
        if (DestColor)
        {
            DestColor->StartColor = SourceColor->StartColor;
            DestColor->StartAlpha = SourceColor->StartAlpha;
        }
    }

    // 새 모듈을 원본 모듈 바로 다음에 삽입
    int32 NewModuleIndex = ModuleIndex + 1;
    LODLevel->Modules.Insert(NewModule, NewModuleIndex);

    // 선택 상태 업데이트
    bool bUpdateSelectionIndices = (Selection.EmitterIndex == EmitterIndex &&
                                     Selection.ModuleIndex >= NewModuleIndex);
    
    // 파티클 시스템 모듈 리스트 업데이트
    ParticleSystem->UpdateAllModuleLists();
    
    // 선택 상태 업데이트
    if (bUpdateSelectionIndices && Selection.ModuleIndex != INDEX_NONE)
    {
        // 추가된 모듈 이후의 인덱스를 가진 모듈이 선택되어 있었으면 인덱스 업데이트
        Selection.ModuleIndex++;
    }
    
    // 새 모듈을 선택 상태로 변경
    Selection.SelectedModule = NewModule;
    Selection.ModuleIndex = NewModuleIndex;
    Selection.EmitterIndex = EmitterIndex;
    Selection.SelectedEmitter = Emitter;
    
    // 선택 변경 이벤트 발생
    OnSelectionChanged();
}

void ParticleSystemEmittersPanel::OnRemoveModule(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex)
{
    UParticleSystem* ParticleSystem = ParticleSystemComponent ? ParticleSystemComponent->Template : nullptr;
    if (!ParticleSystem || !Module || EmitterIndex >= ParticleSystem->Emitters.Num())
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot remove module: Invalid ParticleSystem, Module, or EmitterIndex");
        return;
    }

    UParticleEmitter* Emitter = ParticleSystem->Emitters[EmitterIndex];
    if (!Emitter || Emitter->LODLevels.Num() == 0)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot remove module: Invalid Emitter or no LOD levels");
        return;
    }

    // LOD 레벨 가져오기 (현재 구현에서는 LOD 0만 사용)
    UParticleLODLevel* LODLevel = Emitter->LODLevels[0];
    if (!LODLevel)
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot remove module: Invalid LOD level");
        return;
    }

    // 모듈 인덱스 검증
    if (ModuleIndex < 0 || ModuleIndex >= LODLevel->Modules.Num())
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot remove module: Invalid ModuleIndex %d", ModuleIndex);
        return;
    }

    // Required 또는 Spawn 모듈인 경우 제거하지 않음
    if (Cast<UParticleModuleRequired>(Module) || Cast<UParticleModuleSpawn>(Module))
    {
        UE_LOG(ELogLevel::Warning, "[PSV] Cannot remove Required or Spawn module: This module is essential for the emitter");
        return;
    }

	if (Cast<UParticleModuleTypeDataMesh>(Module))
	{
		// MeshTypedata 인 경우 TypeDataModule 제거하여 스프라이트로 돌아오도록 함
		Emitter->GetLODLevel(0)->TypeDataModule = nullptr;
	}

    // 선택 상태 확인 및 업데이트
    bool bWasSelected = (Selection.SelectedModule == Module &&
                          Selection.ModuleIndex == ModuleIndex &&
                          Selection.EmitterIndex == EmitterIndex);
    bool bUpdateSelectionIndices = (Selection.EmitterIndex == EmitterIndex &&
                                     Selection.ModuleIndex > ModuleIndex);

    // 모듈 제거
    LODLevel->Modules.RemoveAt(ModuleIndex);
    UE_LOG(ELogLevel::Display, "[PSV] Module removed from emitter '%s'", GetData(Emitter->EmitterName.ToString()));

    // 선택 상태 업데이트
    if (bWasSelected)
    {
        // 삭제된 모듈이 선택되어 있었으면 에미터만 선택 상태로 유지
        Selection.SelectedModule = nullptr;
        Selection.ModuleIndex = INDEX_NONE;
        OnSelectionChanged();
    }
    else if (bUpdateSelectionIndices && Selection.ModuleIndex != INDEX_NONE)
    {
        // 삭제된 모듈 이후의 인덱스를 가진 모듈이 선택되어 있었으면 인덱스 업데이트
        Selection.ModuleIndex--;
    }

    // 파티클 시스템 모듈 리스트 업데이트
    ParticleSystem->UpdateAllModuleLists();
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
