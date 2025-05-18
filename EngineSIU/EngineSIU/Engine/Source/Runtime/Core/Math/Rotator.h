#pragma once
#include "Container/String.h"
#include "Serialization/Archive.h"

struct FVector;
struct FQuat;
struct FMatrix;

#include "MathUtility.h"

// 회전 정보를 Degree 단위로 저장하는 구조체
struct FRotator
{
    float Pitch;
    float Yaw;
    float Roll;

    explicit FRotator()
        : Pitch(0.0f), Yaw(0.0f), Roll(0.0f)
    {}

    explicit FRotator(float InPitch, float InYaw, float InRoll)
        : Pitch(InPitch), Yaw(InYaw), Roll(InRoll)
    {}

    FRotator(const FRotator& Other)
        : Pitch(Other.Pitch), Yaw(Other.Yaw), Roll(Other.Roll)
    {}

    explicit FRotator(const FVector& InVector);
    explicit FRotator(const FQuat& InQuat);

    static const FRotator ZeroRotator;

    FRotator operator+(const FRotator& Other) const;
    FRotator& operator+=(const FRotator& Other);

    FRotator operator-(const FRotator& Other) const;
    FRotator& operator-=(const FRotator& Other);

    FRotator operator*(float Scalar) const;
    FRotator& operator*=(float Scalar);

    friend FRotator operator*(float Scalar, const FRotator& Rotator)
    {
        return Rotator * Scalar; // 기존 멤버 함수 재활용
    }

    FRotator operator/(const FRotator& Other) const;
    FRotator operator/(float Scalar) const;
    FRotator& operator/=(float Scalar);

    FRotator operator-() const;

    bool operator==(const FRotator& Other) const;
    bool operator!=(const FRotator& Other) const;

    bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const;
    bool IsZero() const;

    bool Equals(const FRotator& Other, float Tolerance = KINDA_SMALL_NUMBER) const;

    FRotator Add(float DeltaPitch, float DeltaYaw, float DeltaRoll) const;

    FRotator FromQuaternion(const FQuat& InQuat) const;
    FQuat Quaternion() const;
    FVector ToVector() const;
    FVector RotateVector(const FVector& Vec) const;
    FMatrix ToMatrix() const;

    
    static float ClampAxis(float Angle);
    FRotator GetNormalized() const;
    void Normalize();
    
    FString ToString() const;
    bool InitFromString(const FString& InSourceString);

    static float NormalizeAxis(float Angle);
    static FRotator MakeLookAtRotation(const FVector& From, const FVector& To);
};

inline FArchive& operator<<(FArchive& Ar, FRotator& R)
{
    Ar << R.Pitch << R.Yaw << R.Roll;
    return Ar;
}
