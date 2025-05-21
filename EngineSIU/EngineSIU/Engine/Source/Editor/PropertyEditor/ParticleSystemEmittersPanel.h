#pragma once
#include "Components/ParticleSystemComponent.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorPanel.h"
#include <functional>

class UParticleModule;
class UParticleEmitter;

/**
 * 선택된 객체 정보를 저장하는 구조체
 */
struct FSelectedObject
{
    UParticleSystem* ParticleSystem = nullptr;
    UParticleEmitter* SelectedEmitter = nullptr;
    UParticleModule* SelectedModule = nullptr;
    int32 EmitterIndex = INDEX_NONE;
    int32 ModuleIndex = INDEX_NONE;

    bool HasSelection() const { return SelectedEmitter != nullptr || SelectedModule != nullptr; }
    void Reset() { SelectedEmitter = nullptr; SelectedModule = nullptr; EmitterIndex = ModuleIndex = INDEX_NONE; }
};

/**
 * 파티클 시스템 에미터 및 모듈을 관리하고 표시하는 패널
 */
class ParticleSystemEmittersPanel : public UEditorPanel
{
public:
    ParticleSystemEmittersPanel() = default;

    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    void RenderEmitters(UParticleSystem* ParticleSystem);
    void RenderModules(UParticleEmitter* Emitter, int32 EmitterIndex);
    void RenderModuleCategory(const TArray<UParticleModule*>& Modules, int32 EmitterIndex);
    
    /**
     * 에미터 컨텍스트 메뉴 표시
     *
     * @param Emitter 컨텍스트 메뉴를 표시할 에미터
     * @param EmitterIndex 에미터 인덱스
     */
    void ShowEmitterContextMenu(UParticleEmitter* Emitter, int32 EmitterIndex);
    
    /**
     * 모듈 컨텍스트 메뉴 표시
     *
     * @param Module 컨텍스트 메뉴를 표시할 모듈
     * @param EmitterIndex 에미터 인덱스
     * @param ModuleIndex 모듈 인덱스
     */
    void ShowModuleContextMenu(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex);
    
    // 에미터 이벤트 핸들러
    void OnRenameEmitter(UParticleEmitter* Emitter, int32 EmitterIndex);
    void OnDuplicateEmitter(UParticleEmitter* Emitter, int32 EmitterIndex);
    void OnRemoveEmitter(UParticleEmitter* Emitter, int32 EmitterIndex);
    
    // 파티클 시스템 이벤트 핸들러
    void OnSelectParticleSystem();
    void OnAddEmitterBefore(int32 EmitterIndex);
    void OnAddEmitterAfter(int32 EmitterIndex);
    void OnRemoveDuplicateModule(UParticleEmitter* Emitter);
    
    // 파티클 모듈 추가 이벤트 핸들러
    void OnAddInitialColor(UParticleEmitter* Emitter);
    void OnAddColorOverLife(UParticleEmitter* Emitter);
    void OnAddLifetime(UParticleEmitter* Emitter);
    void OnAddInitialSize(UParticleEmitter* Emitter);
    void OnAddSizeByLife(UParticleEmitter* Emitter);
    void OnAddSpawnPerUnit(UParticleEmitter* Emitter);
    void OnAddInitialVelocity(UParticleEmitter* Emitter);
    void OnAddVelocityOverLife(UParticleEmitter* Emitter);
    
    // 모듈 이벤트 핸들러
    void OnDuplicateModule(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex);
    void OnRemoveModule(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex);
    void OnMoveModuleUp(int32 EmitterIndex, int32 ModuleIndex);
    void OnMoveModuleDown(int32 EmitterIndex, int32 ModuleIndex);
    
    /**
     * 에미터 선택 처리
     *
     * @param Emitter 선택된 에미터
     * @param EmitterIndex 에미터 인덱스
     * @return 선택 상태가 변경되었는지 여부
     */
    bool HandleEmitterSelection(UParticleEmitter* Emitter, int32 EmitterIndex);
    
    /**
     * 모듈 선택 처리
     *
     * @param Module 선택된 모듈
     * @param EmitterIndex 모듈이 속한 에미터의 인덱스
     * @param ModuleIndex 모듈 인덱스
     * @return 선택 상태가 변경되었는지 여부
     */
    bool HandleModuleSelection(UParticleModule* Module, int32 EmitterIndex, int32 ModuleIndex);
    
    /**
     * 선택 상태가 변경될 때 호출되는 함수
     */
    void OnSelectionChanged();

public:
    void SetParticleSystemComponent(UParticleSystemComponent* InParticleSystemComponent);
    
    /**
     * 선택된 객체 정보 반환
     */
    const FSelectedObject& GetSelectedObject() const { return Selection; }
    
    /**
     * 선택 변경 콜백 설정
     */
    void SetSelectionChangedCallback(std::function<void(const FSelectedObject&, UParticleSystemComponent*)> InCallback) { SelectionChangedCallback = InCallback; }
    
    /**
     * 모듈이 속한 에미터를 찾는 함수
     *
     * @param Module 찾을 모듈
     * @param OutEmitterIndex 찾은 에미터의 인덱스 (실패시 INDEX_NONE)
     * @return 찾은 에미터 포인터 (실패시 nullptr)
     */
    UParticleEmitter* FindEmitterForModule(UParticleModule* Module, int32& OutEmitterIndex);

private:
    UParticleSystemComponent* ParticleSystemComponent = nullptr;
    
    // 선택 상태 관리
    FSelectedObject Selection;
    std::function<void(const FSelectedObject&, UParticleSystemComponent*)> SelectionChangedCallback;

    // 이미터 이름 변경 상태 관리
    bool bShowRenameEmitterModal = false;
    UParticleEmitter* EmitterToRename = nullptr;
    int32 EmitterToRenameIndex = INDEX_NONE;
    char EmitterNameBuffer[256] = {};

private:
    float Width = 0, Height = 0;
    const float ContentWidth = 300.0f;
};
