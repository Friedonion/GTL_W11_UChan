#include "PointLightComponent.h"

#include "Math/JungleMath.h"
#include "UObject/Casts.h"
#include "EngineLoop.h"

UPointLightComponent::UPointLightComponent()
{
    PointLightInfo.Position = GetComponentLocation();
    PointLightInfo.Radius = 30.f;

    PointLightInfo.LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    PointLightInfo.Intensity = 1000.f;
    PointLightInfo.Type = ELightType::POINT_LIGHT;
    PointLightInfo.Attenuation = 20.0f;

    // CubeMap이므로 6개의 ShadowMap을 생성합니다.
    // constexpr int32 ShadowMapCreationCount = 6;  
    // for (int32 i = 0; i < ShadowMapCreationCount; ++i)  
    // {
    // }
    // UE_LOG(ELogLevel::Error,
    //     TEXT("CreateSliceSRV 입장3: ArraySize=%u, MipLevels=%u"),
    //     dbg.ArraySize, dbg.MipLevels
    // );
}

//void UPointLightComponent::Initialize()
//{
//    ShadowMaps.Empty(); // 부모 생성자에서 호출하는 CreateShadowMap()를 방지하기 위해 초기화
//    CreateShadowMap();
//    D3D11_TEXTURE2D_DESC dbg;
//    ShadowMaps[0].Texture2D->GetDesc(&dbg);
//
//    UE_LOG(ELogLevel::Error,
//        TEXT("CreateSliceSRV 입장1: ArraySize=%u, MipLevels=%u"),
//        dbg.ArraySize, ShadowMaps.Num()
//    );
//    UE_LOG(ELogLevel::Error,
//        TEXT("CreateSliceSRV 입장1: ArraySize=%u, MipLevels=%u"),
//        dbg.ArraySize, dbg.MipLevels
//    );
//    InitShadowDebugView();
//
//    
//    UE_LOG(ELogLevel::Error,
//        TEXT("CreateSliceSRV 입장: ArraySize=%u, MipLevels=%u"),
//        dbg.ArraySize, dbg.MipLevels
//    );
//    
//    for (UINT i = 0; i < NUM_FACES; ++i)
//    {
//        SliceSRVs[i] = CreateSliceSRV(
//            ShadowMaps[0].Texture2D, // 1번 DSV 해당 SRV, 0번 RTV 해당 SRV
//            DXGI_FORMAT_R32_FLOAT,
//            i
//        );
//    }
//    UE_LOG(ELogLevel::Error,
//        TEXT("CreateSliceSRV 입장3: ArraySize=%u, MipLevels=%u"),
//        dbg.ArraySize, dbg.MipLevels
//    );
//}

// HRESULT UPointLightComponent::CreateShadowMap()
// {
//     // Shadow Cube Map 생성
//     // Texture2D : 기존 그대로
//     // DepthStencilView : Texture2DArray로 생성
//     // ShaderResourceView : Texture2DArray로 생성
//     FDepthStencilRHI NewResource;
//
//     HRESULT hr = S_OK;
//
//     D3D11_TEXTURE2D_DESC CubeMapTextureDesc = {};
//     CubeMapTextureDesc.Width = ShadowMapWidth;
//     CubeMapTextureDesc.Height = ShadowMapHeight;
//     CubeMapTextureDesc.MipLevels = 1;
//     CubeMapTextureDesc.ArraySize = NUM_FACES;
//     CubeMapTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
//     CubeMapTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
//     CubeMapTextureDesc.SampleDesc.Count = 1;
//     CubeMapTextureDesc.SampleDesc.Quality = 0;
//     CubeMapTextureDesc.Usage = D3D11_USAGE_DEFAULT;
//     CubeMapTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
//
//     hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&CubeMapTextureDesc, nullptr, &NewResource.Texture2D);
//     if (FAILED(hr))
//     {
//         UE_LOG(ELogLevel::Error, TEXT("Failed to create Shadow Cube Map texture!"));
//         return hr;
//     }
//
//     D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
//     rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
//     rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
//     rtvDesc.Texture2D.MipSlice = 0;
//     rtvDesc.Texture2DArray.FirstArraySlice = 0;
//     rtvDesc.Texture2DArray.ArraySize = NUM_FACES;  // DSV 생성시 모든 6면을 포함
//     hr = FEngineLoop::GraphicDevice.Device->CreateRenderTargetView(NewResource.Texture2D, &rtvDesc, &DepthRTVArray);
//     if (FAILED(hr))
//     {
//         UE_LOG(ELogLevel::Error, TEXT("Failed to create Shadow Cube Map RTV!"));
//         return hr;
//     }
//
//     D3D11_SHADER_RESOURCE_VIEW_DESC CubeMapSRVDesc = {};
//     CubeMapSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
//     CubeMapSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
//     CubeMapSRVDesc.Texture2DArray.MostDetailedMip = 0;
//     CubeMapSRVDesc.Texture2DArray.MipLevels = 1;
//     CubeMapSRVDesc.Texture2DArray.FirstArraySlice = 0;
//     CubeMapSRVDesc.Texture2DArray.ArraySize = NUM_FACES;
//     hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(NewResource.Texture2D, &CubeMapSRVDesc, &NewResource.SRV);
//     if (FAILED(hr))
//     {
//         UE_LOG(ELogLevel::Error, TEXT("Failed to create Shadow Cube Map SRV!"));
//         return hr;
//     }
//
//     ShadowMaps.Add(NewResource);
//
//     FDepthStencilRHI DSVResource;
//     // 1) 텍스처 생성 (Typeless)
//     D3D11_TEXTURE2D_DESC desc = {};
//     desc.Width = ShadowMapWidth;
//     desc.Height = ShadowMapHeight;
//     desc.MipLevels = 1;
//     desc.ArraySize = NUM_FACES;
//     desc.Format = DXGI_FORMAT_R32_TYPELESS;                    // <-- Typeless
//     desc.SampleDesc.Count = 1;
//     desc.Usage = D3D11_USAGE_DEFAULT;
//     desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
//     desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // DSV + SRV
//     hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&desc, nullptr, &DSVResource.Texture2D);
//     if (FAILED(hr) || !DSVResource.Texture2D) {
//         UE_LOG(ELogLevel::Error, TEXT("CreateTexture2D(Depth) failed: 0x%08X"), hr);
//         return hr;
//     }
//
//     // 2) DepthStencilView 생성
//     D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
//     dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;                    // <-- DSV 에 맞는 포맷
//     dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
//     dsvDesc.Texture2DArray.MipSlice = 0;
//     dsvDesc.Texture2DArray.FirstArraySlice = 0;
//     dsvDesc.Texture2DArray.ArraySize = NUM_FACES;
//     hr = FEngineLoop::GraphicDevice.Device->CreateDepthStencilView(DSVResource.Texture2D, &dsvDesc, &DSVResource.DSV);
//     if (FAILED(hr) || !DSVResource.DSV) {
//         UE_LOG(ELogLevel::Error, TEXT("CreateDepthStencilView failed: 0x%08X"), hr);
//         return hr;
//     }
//
//     // 3) ShaderResourceView 생성 (나중에 샘플링용)
//     D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//     srvDesc.Format = DXGI_FORMAT_R32_FLOAT;                    // <-- SRV 에 맞는 포맷
//     srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
//     srvDesc.TextureCube.MostDetailedMip = 0;
//     srvDesc.TextureCube.MipLevels = 1;
//     hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(DSVResource.Texture2D, &srvDesc, &DSVResource.SRV);
//     if (FAILED(hr) || !DSVResource.SRV) {
//         UE_LOG(ELogLevel::Error, TEXT("CreateShaderResourceView failed: 0x%08X"), hr);
//         return hr;
//     }
//
//     D3D11_TEXTURE2D_DESC actual;
//     NewResource.Texture2D->GetDesc(&actual);
//     UE_LOG(ELogLevel::Error,
//         TEXT(">> 실제 ArraySize=%u, MipLevels=%u, Format=%u, MiscFlags=0x%X"),
//         actual.ArraySize,
//         actual.MipLevels,
//         actual.Format,
//         actual.MiscFlags
//     
//     );
//     ShadowMaps.Add(DSVResource);
//     return hr;
// }

void UPointLightComponent::InitShadowDebugView()
{
    for (int i = 0; i < 6; ++i)
    {
        D3D11_TEXTURE2D_DESC TexDesc = {};
        TexDesc.Width = ShadowMapWidth;
        TexDesc.Height = ShadowMapHeight;
        TexDesc.MipLevels = 1;
        TexDesc.ArraySize = 1;
        TexDesc.Format = DXGI_FORMAT_R32_FLOAT;
        TexDesc.SampleDesc.Count = 1;
        TexDesc.Usage = D3D11_USAGE_DEFAULT;
        TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        TexDesc.CPUAccessFlags = 0;
        TexDesc.MiscFlags = 0;

        ID3D11Texture2D* Texture = nullptr;
        auto hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&TexDesc, nullptr, &Texture);
        if (FAILED(hr))
        {
            UE_LOG(ELogLevel::Error, TEXT("Failed to create shadow debug texture!"));
            return;
        }
        OutputTextures.Add(Texture);

        D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
        SrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SrvDesc.Texture2D.MostDetailedMip = 0;
        SrvDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* Srv = nullptr;
        hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(Texture, &SrvDesc, &Srv);
        if (FAILED(hr))
        {
            UE_LOG(ELogLevel::Error, TEXT("Failed to create shadow debug SRV!"));
            return;
        }
        OutputSRVs.Add(Srv);
    }
}

UObject* UPointLightComponent::Duplicate(UObject* InOuter)
{

    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    if (NewComponent)
    {
        NewComponent->PointLightInfo = PointLightInfo;
    }
    return NewComponent;
}

void UPointLightComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("Radius"), FString::Printf(TEXT("%f"), PointLightInfo.Radius));
    OutProperties.Add(TEXT("LightColor"), FString::Printf(TEXT("%s"), *PointLightInfo.LightColor.ToString()));
    OutProperties.Add(TEXT("Intensity"), FString::Printf(TEXT("%f"), PointLightInfo.Intensity));
    OutProperties.Add(TEXT("Type"), FString::Printf(TEXT("%d"), PointLightInfo.Type));
    OutProperties.Add(TEXT("Attenuation"), FString::Printf(TEXT("%f"), PointLightInfo.Attenuation));
    OutProperties.Add(TEXT("Position"), FString::Printf(TEXT("%s"), *PointLightInfo.Position.ToString()));
}

void UPointLightComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("Radius"));
    if (TempStr)
    {
        PointLightInfo.Radius = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("LightColor"));
    if (TempStr)
    {
        PointLightInfo.LightColor.InitFromString(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Intensity"));
    if (TempStr)
    {
        PointLightInfo.Intensity = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Type"));
    if (TempStr)
    {
        PointLightInfo.Type = FString::ToInt(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Attenuation"));
    if (TempStr)
    {
        PointLightInfo.Attenuation = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Position"));
    if (TempStr)
    {
        PointLightInfo.Position.InitFromString(*TempStr);
    }
    
}

FPointLightInfo& UPointLightComponent::GetPointLightInfo()
{
    return PointLightInfo;
}

void UPointLightComponent::SetPointLightInfo(const FPointLightInfo& InPointLightInfo)
{
    PointLightInfo = InPointLightInfo;
}


float UPointLightComponent::GetRadius() const
{
    return PointLightInfo.Radius;
}

void UPointLightComponent::SetRadius(float InRadius)
{
    PointLightInfo.Radius = InRadius;
}

FLinearColor UPointLightComponent::GetLightColor() const
{
    return PointLightInfo.LightColor;
}

void UPointLightComponent::SetLightColor(const FLinearColor& InColor)
{
    PointLightInfo.LightColor = InColor;
}


float UPointLightComponent::GetIntensity() const
{
    return PointLightInfo.Intensity;
}

void UPointLightComponent::SetIntensity(float InIntensity)
{
    PointLightInfo.Intensity = InIntensity;
}

int UPointLightComponent::GetType() const
{
    return PointLightInfo.Type;
}

void UPointLightComponent::SetType(int InType)
{
    PointLightInfo.Type = InType;
}

void UPointLightComponent::UpdateViewMatrix()
{
    FVector PointLightPos = GetComponentLocation();
    // ViewMatrices 배열의 크기가 6인지 확인하고, 아니면 조정합니다.
    // 생성자 등에서 미리 크기를 6으로 설정하는 것이 더 효율적일 수 있습니다.
    if (ViewMatrices.Num() != 6)
    {
        ViewMatrices.SetNum(6);
    }
    // 1. +X 면 (World Forward)
    // Target: 정면 / Up: 월드 위쪽 (Z+)
    ViewMatrices[0] = JungleMath::CreateViewMatrix(PointLightPos, PointLightPos + FVector::ForwardVector, FVector::RightVector);
    // 2. -X 면 (World Backward)
    // Target: 후면 / Up: 월드 위쪽 (Z+)
    ViewMatrices[1] = JungleMath::CreateViewMatrix(PointLightPos, PointLightPos - FVector::ForwardVector, FVector::RightVector);
    // 3. +Y 면 (World Right)
    // Target: 우측 / Up: 월드 위쪽 (Z+)
    ViewMatrices[2] = JungleMath::CreateViewMatrix(PointLightPos, PointLightPos + FVector::RightVector, FVector::DownVector);
    // 4. -Y 면 (World Left)
    // Target: 좌측 / Up: 월드 위쪽 (Z+)
    ViewMatrices[3] = JungleMath::CreateViewMatrix(PointLightPos, PointLightPos - FVector::RightVector, FVector::UpVector);
    // 5. +Z 면 (World Up)
    // Target: 위쪽 / Up: 월드 정면 (X+) -> 위를 볼 때, 화면 상단이 월드의 정면 방향이 되도록 설정
    // Up 벡터가 시선 방향(Z+)과 평행하면 안 되므로 다른 축 사용
    ViewMatrices[4] = JungleMath::CreateViewMatrix(PointLightPos, PointLightPos + FVector::UpVector, FVector::RightVector); // Up: World Forward (+X)
    // 6. -Z 면 (World Down)
    // Target: 아래쪽 / Up: 월드 정면 (X+) -> 아래를 볼 때, 화면 상단이 월드의 정면 방향이 되도록 설정
    // Up 벡터가 시선 방향(-Z)과 평행하면 안 되므로 다른 축 사용
    ViewMatrices[5] = JungleMath::CreateViewMatrix(PointLightPos, PointLightPos - FVector::UpVector, FVector::RightVector); // Up: World Forward (+X)
}

void UPointLightComponent::UpdateProjectionMatrix()
{
    // 1. 포인트 라이트용 파라미터 설정
    // FOV: 큐브맵 각 면은 90도 시야각을 가짐 (PI / 2 라디안)
    const float FieldOfViewRadians = PI / 2.0f; // 90 degrees in radians

    // Aspect Ratio: 큐브맵 각 면은 정사각형이므로 종횡비는 1.0
    const float CurrentAspectRatio = 1.0f;

    // Near Clip Plane 값 설정 (매우 작은 값 사용)
    const float NearClipPlane = NEAR_PLANE; // 또는 직접 상수 값 사용 (예: 0.01f)

    // Far Clip Plane 값 설정 (라이트의 감쇠 반경 사용)
    // GetRadius() 함수나 멤버 변수(PointLightInfo.Radius)를 통해 가져옴
    const float FarClipPlane = GetRadius();

    // 2. 원근 투영 행렬 생성
    ProjectionMatrix = JungleMath::CreateProjectionMatrix(
        FieldOfViewRadians,
        CurrentAspectRatio,
        NearClipPlane,
        FarClipPlane
    );
}


//TArray<FDepthStencilRHI>& UPointLightComponent::GetShadowMap()
//{
//    // ShadowMap의 크기가 바뀐 경우 새로 생성합니다.
//    if (bDirtyFlag)
//    {
//        if (HasShadowMap())
//        {
//            ReleaseShadowMap();
//        }
//
//        // CubeMap이므로 6개의 ShadowMap을 생성합니다. (함수 내부에서)
//        constexpr int32 ShadowMapCreationCount = 6;
//        CreateShadowMap();
//
//        bDirtyFlag = false;
//    }
//    return ShadowMaps;
//}
//
//ID3D11ShaderResourceView* UPointLightComponent::GetSliceSRV(int SliceIndex) const
//{
//    return SliceSRVs[SliceIndex];
//    // 1) 아까 만든 Texture2DArray 에서 해당 슬라이스만 OutputTextures[SliceIndex] 로 복사
//    UINT subresource = D3D11CalcSubresource(
//        /*MipSlice=*/0,
//        /*ArraySlice=*/SliceIndex,
//        /*NumMips=*/1
//    );
//    FEngineLoop::GraphicDevice.DeviceContext
//        ->CopySubresourceRegion(
//            OutputTextures[SliceIndex],  // dst 텍스처
//            /*DstSubresource=*/0,
//            /*DstX=*/0, /*DstY=*/0, /*DstZ=*/0,
//            ShadowMaps[1].Texture2D,     // src 텍스처
//            subresource,
//            /*pSrcBox=*/nullptr
//        );
//
//   //  2) 복사된 2D 텍스처에 대한 SRV 반환
//    return OutputSRVs[SliceIndex];
//
//}
//
//ID3D11ShaderResourceView* UPointLightComponent::CreateSliceSRV(
//    ID3D11Texture2D* texArray,
//    DXGI_FORMAT     format,
//    UINT            sliceIndex)
//{
//    D3D11_TEXTURE2D_DESC ddesc;
//    ShadowMaps[0].Texture2D->GetDesc(&ddesc);
//    UE_LOG(ELogLevel::Error, TEXT("Cubemap.ArraySize=%u"), ddesc.ArraySize);
//
//    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
//    desc.Format = format;
//    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;  // ← 여기!! CUBE면 DSV로생성, 2DArray면 RTV
//    desc.Texture2DArray.MostDetailedMip = 0;
//    desc.Texture2DArray.MipLevels = 1;
//    desc.Texture2DArray.FirstArraySlice = sliceIndex;
//    desc.Texture2DArray.ArraySize = 1;
//
//    ID3D11ShaderResourceView* srv = nullptr;
//    auto hr = FEngineLoop::GraphicDevice.Device
//        ->CreateShaderResourceView(texArray, &desc, &srv);
//    if (FAILED(hr))
//    {
//        UE_LOG(ELogLevel::Error, TEXT("Failed to create slice SRV!"));
//        return nullptr;
//    }
//    return srv;
//}
