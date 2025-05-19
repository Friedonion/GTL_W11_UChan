#include "CubeComp.h"
#include "Engine/FObjLoader.h"
#include "UObject/ObjectFactory.h"


UCubeComp::UCubeComp()
{
    SetType(StaticClass()->GetName());
    AABB.MaxLocation = { 1,1,1 };
    AABB.MinLocation = { -1,-1,-1 };
    FObjManager::CreateStaticMesh("Contents/Reference/Reference.obj");
    SetStaticMesh(FObjManager::GetStaticMesh(L"Reference.obj"));
}

void UCubeComp::InitializeComponent()
{
    Super::InitializeComponent();

    //FManagerCreateStaticMesh("Assets/helloBlender.obj");
    //SetStaticMesh(FManagerGetStaticMesh(L"helloBlender.obj"));
    // 
    // Begin Test
    FObjManager::CreateStaticMesh("Contents/Reference/Reference.obj");
    SetStaticMesh(FObjManager::GetStaticMesh(L"Reference.obj"));
    // End Test
}

void UCubeComp::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

}
