#pragma once
#include "Components/ParticleSystemComponent.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorPanel.h"

class UParticleModule;
class UParticleEmitter;

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

public:
    void SetParticleSystemComponent(UParticleSystemComponent* InParticleSystemComponent) { ParticleSystemComponent = InParticleSystemComponent; }

private:
    UParticleSystemComponent* ParticleSystemComponent = nullptr;

private:
    float Width = 0, Height = 0;
};
