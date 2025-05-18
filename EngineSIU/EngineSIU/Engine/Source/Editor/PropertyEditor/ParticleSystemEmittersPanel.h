#pragma once
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorPanel.h"

class ParticleSystemEmittersPanel : public UEditorPanel
{
public:
    ParticleSystemEmittersPanel();

    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:

private:
    float Width = 0, Height = 0;
};
