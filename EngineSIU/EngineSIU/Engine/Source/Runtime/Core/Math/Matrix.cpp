#include "Matrix.h"

#include <cmath>
#include "MathSSE.h"
#include "MathUtility.h"
#include "Quat.h"
#include "Rotator.h"
#include "Vector.h"
#include "Vector4.h"
#include "HAL/PlatformType.h"


// 단위 행렬 정의
const FMatrix FMatrix::Identity = { {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
} };

// 행렬 덧셈
FMatrix FMatrix::operator+(const FMatrix& Other) const
{
    FMatrix Result;
    for (int32 i = 0; i < 4; i++)
    {
        for (int32 j = 0; j < 4; j++)
        {
            Result.M[i][j] = M[i][j] + Other.M[i][j];
        }
    }
    return Result;
}

// 행렬 뺄셈
FMatrix FMatrix::operator-(const FMatrix& Other) const
{
    FMatrix Result;
    for (int32 i = 0; i < 4; i++)
    {
        for (int32 j = 0; j < 4; j++)
        {
            Result.M[i][j] = M[i][j] - Other.M[i][j];
        }
    }
    return Result;
}

// 행렬 곱셈
FMatrix FMatrix::operator*(const FMatrix& Other) const
{
    FMatrix Result = {};
    SSE::VectorMatrixMultiply(&Result, this, &Other);
    return Result;
}

FMatrix& FMatrix::operator*=(const FMatrix& Other)
{
    SSE::VectorMatrixMultiply(this, this, &Other);
    return *this;
}

// 스칼라 곱셈
FMatrix FMatrix::operator*(float Scalar) const
{
    FMatrix Result;
    for (int32 i = 0; i < 4; ++i)
    {
        Result.M[i][0] = M[i][0] * Scalar;
        Result.M[i][1] = M[i][1] * Scalar;
        Result.M[i][2] = M[i][2] * Scalar;
        Result.M[i][3] = M[i][3] * Scalar;
    }
    return Result;
}

// 스칼라 나눗셈
FMatrix FMatrix::operator/(float Scalar) const
{
    FMatrix Result;
    for (int32 i = 0; i < 4; i++)
    {
        for (int32 j = 0; j < 4; j++)
        {
            Result.M[i][j] = M[i][j] / Scalar;
        }
    }
    return Result;
}

float* FMatrix::operator[](int row)
{
    return M[row];
}

const float* FMatrix::operator[](int row) const
{
    return M[row];
}

FVector FMatrix::ExtractScaling(float Tolerance)
{
    FVector Scale3D(0, 0, 0);

    // For each row, find magnitude, and if its non-zero re-scale so its unit length.
    const float SquareSum0 = (M[0][0] * M[0][0]) + (M[0][1] * M[0][1]) + (M[0][2] * M[0][2]);
    const float SquareSum1 = (M[1][0] * M[1][0]) + (M[1][1] * M[1][1]) + (M[1][2] * M[1][2]);
    const float SquareSum2 = (M[2][0] * M[2][0]) + (M[2][1] * M[2][1]) + (M[2][2] * M[2][2]);

    if (SquareSum0 > Tolerance)
    {
        float Scale0 = FMath::Sqrt(SquareSum0);
        Scale3D[0] = Scale0;
        float InvScale0 = 1.f / Scale0;
        M[0][0] *= InvScale0;
        M[0][1] *= InvScale0;
        M[0][2] *= InvScale0;
    }
    else
    {
        Scale3D[0] = 0;
    }

    if (SquareSum1 > Tolerance)
    {
        float Scale1 = FMath::Sqrt(SquareSum1);
        Scale3D[1] = Scale1;
        float InvScale1 = 1.f / Scale1;
        M[1][0] *= InvScale1;
        M[1][1] *= InvScale1;
        M[1][2] *= InvScale1;
    }
    else
    {
        Scale3D[1] = 0;
    }

    if (SquareSum2 > Tolerance)
    {
        float Scale2 = FMath::Sqrt(SquareSum2);
        Scale3D[2] = Scale2;
        float InvScale2 = 1.f / Scale2;
        M[2][0] *= InvScale2;
        M[2][1] *= InvScale2;
        M[2][2] *= InvScale2;
    }
    else
    {
        Scale3D[2] = 0;
    }

    return Scale3D;
}

FVector FMatrix::GetOrigin() const
{
    return FVector{M[3][0], M[3][1], M[3][2]};
}

float FMatrix::Determinant() const
{
    return	M[0][0] * (
        M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
        M[2][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) +
        M[3][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2])
        ) -
        M[1][0] * (
            M[0][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
            M[2][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
            M[3][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2])
            ) +
        M[2][0] * (
            M[0][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) -
            M[1][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
            M[3][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
            ) -
        M[3][0] * (
            M[0][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2]) -
            M[1][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2]) +
            M[2][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
            );
}

void FMatrix::SetAxis(int32 i, const FVector& Axis)
{
    if (0 <= i && i <= 2)
    {
        M[i][0] = Axis.X;
        M[i][1] = Axis.Y;
        M[i][2] = Axis.Z;
    }
}

FVector FMatrix::GetScaledAxis(EAxis::Type InAxis) const
{
    switch (InAxis)
    {
    case EAxis::X:
        return FVector{M[0][0], M[0][1], M[0][2]};

    case EAxis::Y:
        return FVector{M[1][0], M[1][1], M[1][2]};

    case EAxis::Z:
        return FVector{M[2][0], M[2][1], M[2][2]};

    default:
        return FVector::ZeroVector;
    }
}

// 전치 행렬
FMatrix FMatrix::Transpose(const FMatrix& Mat)
{
    FMatrix dst;

    // 각 행을 128비트(4 float) 단위로 로드
    VectorRegister4Float row0 = _mm_loadu_ps(&Mat.M[0][0]); // 첫 번째 행
    VectorRegister4Float row1 = _mm_loadu_ps(&Mat.M[1][0]); // 두 번째 행
    VectorRegister4Float row2 = _mm_loadu_ps(&Mat.M[2][0]); // 세 번째 행
    VectorRegister4Float row3 = _mm_loadu_ps(&Mat.M[3][0]); // 네 번째 행

    _MM_TRANSPOSE4_PS(row0, row1, row2, row3);

    // 전치된 행을 저장
    _mm_storeu_ps(&dst.M[0][0], row0);
    _mm_storeu_ps(&dst.M[1][0], row1);
    _mm_storeu_ps(&dst.M[2][0], row2);
    _mm_storeu_ps(&dst.M[3][0], row3);

    return dst;
}

FMatrix FMatrix::Inverse(const FMatrix& Mat)
{
    FMatrix Result;
    FMatrix Tmp;
    float Det[4];

    Tmp[0][0] = Mat[2][2] * Mat[3][3] - Mat[2][3] * Mat[3][2];
    Tmp[0][1] = Mat[1][2] * Mat[3][3] - Mat[1][3] * Mat[3][2];
    Tmp[0][2] = Mat[1][2] * Mat[2][3] - Mat[1][3] * Mat[2][2];

    Tmp[1][0] = Mat[2][2] * Mat[3][3] - Mat[2][3] * Mat[3][2];
    Tmp[1][1] = Mat[0][2] * Mat[3][3] - Mat[0][3] * Mat[3][2];
    Tmp[1][2] = Mat[0][2] * Mat[2][3] - Mat[0][3] * Mat[2][2];

    Tmp[2][0] = Mat[1][2] * Mat[3][3] - Mat[1][3] * Mat[3][2];
    Tmp[2][1] = Mat[0][2] * Mat[3][3] - Mat[0][3] * Mat[3][2];
    Tmp[2][2] = Mat[0][2] * Mat[1][3] - Mat[0][3] * Mat[1][2];

    Tmp[3][0] = Mat[1][2] * Mat[2][3] - Mat[1][3] * Mat[2][2];
    Tmp[3][1] = Mat[0][2] * Mat[2][3] - Mat[0][3] * Mat[2][2];
    Tmp[3][2] = Mat[0][2] * Mat[1][3] - Mat[0][3] * Mat[1][2];

    Det[0] = Mat[1][1] * Tmp[0][0] - Mat[2][1] * Tmp[0][1] + Mat[3][1] * Tmp[0][2];
    Det[1] = Mat[0][1] * Tmp[1][0] - Mat[2][1] * Tmp[1][1] + Mat[3][1] * Tmp[1][2];
    Det[2] = Mat[0][1] * Tmp[2][0] - Mat[1][1] * Tmp[2][1] + Mat[3][1] * Tmp[2][2];
    Det[3] = Mat[0][1] * Tmp[3][0] - Mat[1][1] * Tmp[3][1] + Mat[2][1] * Tmp[3][2];

    const float Determinant = Mat[0][0] * Det[0] - Mat[1][0] * Det[1] + Mat[2][0] * Det[2] - Mat[3][0] * Det[3];

    if (Determinant == 0.0f || !std::isfinite(Determinant))
    {
        return Identity;
    }

    const float RDet = 1.0f / Determinant;

    Result[0][0] = RDet * Det[0];
    Result[0][1] = -RDet * Det[1];
    Result[0][2] = RDet * Det[2];
    Result[0][3] = -RDet * Det[3];
    Result[1][0] = -RDet * (Mat[1][0] * Tmp[0][0] - Mat[2][0] * Tmp[0][1] + Mat[3][0] * Tmp[0][2]);
    Result[1][1] = RDet * (Mat[0][0] * Tmp[1][0] - Mat[2][0] * Tmp[1][1] + Mat[3][0] * Tmp[1][2]);
    Result[1][2] = -RDet * (Mat[0][0] * Tmp[2][0] - Mat[1][0] * Tmp[2][1] + Mat[3][0] * Tmp[2][2]);
    Result[1][3] = RDet * (Mat[0][0] * Tmp[3][0] - Mat[1][0] * Tmp[3][1] + Mat[2][0] * Tmp[3][2]);
    Result[2][0] = RDet * (
        Mat[1][0] * (Mat[2][1] * Mat[3][3] - Mat[2][3] * Mat[3][1]) -
        Mat[2][0] * (Mat[1][1] * Mat[3][3] - Mat[1][3] * Mat[3][1]) +
        Mat[3][0] * (Mat[1][1] * Mat[2][3] - Mat[1][3] * Mat[2][1])
    );
    Result[2][1] = -RDet * (
        Mat[0][0] * (Mat[2][1] * Mat[3][3] - Mat[2][3] * Mat[3][1]) -
        Mat[2][0] * (Mat[0][1] * Mat[3][3] - Mat[0][3] * Mat[3][1]) +
        Mat[3][0] * (Mat[0][1] * Mat[2][3] - Mat[0][3] * Mat[2][1])
    );
    Result[2][2] = RDet * (
        Mat[0][0] * (Mat[1][1] * Mat[3][3] - Mat[1][3] * Mat[3][1]) -
        Mat[1][0] * (Mat[0][1] * Mat[3][3] - Mat[0][3] * Mat[3][1]) +
        Mat[3][0] * (Mat[0][1] * Mat[1][3] - Mat[0][3] * Mat[1][1])
    );
    Result[2][3] = -RDet * (
        Mat[0][0] * (Mat[1][1] * Mat[2][3] - Mat[1][3] * Mat[2][1]) -
        Mat[1][0] * (Mat[0][1] * Mat[2][3] - Mat[0][3] * Mat[2][1]) +
        Mat[2][0] * (Mat[0][1] * Mat[1][3] - Mat[0][3] * Mat[1][1])
    );
    Result[3][0] = -RDet * (
        Mat[1][0] * (Mat[2][1] * Mat[3][2] - Mat[2][2] * Mat[3][1]) -
        Mat[2][0] * (Mat[1][1] * Mat[3][2] - Mat[1][2] * Mat[3][1]) +
        Mat[3][0] * (Mat[1][1] * Mat[2][2] - Mat[1][2] * Mat[2][1])
    );
    Result[3][1] = RDet * (
        Mat[0][0] * (Mat[2][1] * Mat[3][2] - Mat[2][2] * Mat[3][1]) -
        Mat[2][0] * (Mat[0][1] * Mat[3][2] - Mat[0][2] * Mat[3][1]) +
        Mat[3][0] * (Mat[0][1] * Mat[2][2] - Mat[0][2] * Mat[2][1])
    );
    Result[3][2] = -RDet * (
        Mat[0][0] * (Mat[1][1] * Mat[3][2] - Mat[1][2] * Mat[3][1]) -
        Mat[1][0] * (Mat[0][1] * Mat[3][2] - Mat[0][2] * Mat[3][1]) +
        Mat[3][0] * (Mat[0][1] * Mat[1][2] - Mat[0][2] * Mat[1][1])
    );
    Result[3][3] = RDet * (
        Mat[0][0] * (Mat[1][1] * Mat[2][2] - Mat[1][2] * Mat[2][1]) -
        Mat[1][0] * (Mat[0][1] * Mat[2][2] - Mat[0][2] * Mat[2][1]) +
        Mat[2][0] * (Mat[0][1] * Mat[1][2] - Mat[0][2] * Mat[1][1])
    );

    return Result;
}


FMatrix FMatrix::CreateTranslationMatrix(const FVector& V)
{
    FMatrix translationMatrix = FMatrix::Identity;
    translationMatrix.M[3][0] = V.X;
    translationMatrix.M[3][1] = V.Y;
    translationMatrix.M[3][2] = V.Z;
    return translationMatrix;
}

FMatrix FMatrix::CreateRotationMatrix(const FRotator& R)
{
    const float RadRoll = FMath::DegreesToRadians(R.Roll);
    const float RadPitch = FMath::DegreesToRadians(R.Pitch);
    const float RadYaw = FMath::DegreesToRadians(R.Yaw);

    const float CosRoll = FMath::Cos(RadRoll), SinRoll = FMath::Sin(RadRoll);
    const float CosPitch = FMath::Cos(RadPitch), SinPitch = FMath::Sin(RadPitch);
    const float CosYaw = FMath::Cos(RadYaw), SinYaw = FMath::Sin(RadYaw);

    // Z축 (Yaw) 회전
    const FMatrix RotationZ = { {
        { CosYaw, SinYaw, 0, 0 },
        { -SinYaw, CosYaw, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    } };

    // Y축 (Pitch) 회전
    const FMatrix RotationY = { {
        { CosPitch, 0, SinPitch, 0 },
        { 0, 1, 0, 0 },
        { -SinPitch, 0, CosPitch, 0 },
        { 0, 0, 0, 1 }
    } };

    // X축 (Roll) 회전
    const FMatrix RotationX = { {
        { 1, 0, 0, 0 },
        { 0, CosRoll, -SinRoll, 0 },
        { 0, SinRoll, CosRoll, 0 },
        { 0, 0, 0, 1 }
    } };

    // DirectX 표준 순서: Z(Yaw) → Y(Pitch) → X(Roll)  
    return RotationX * RotationY * RotationZ; // 이렇게 하면 오른쪽 부터 적용됨
}

FMatrix FMatrix::CreateRotationMatrix(const FQuat& Q)
{
    FMatrix Result;
    FQuat NormalizedQ = Q; // 복사본 생성
    NormalizedQ.Normalize(); // 정규화된 쿼터니언 사용

    const float X = NormalizedQ.X;
    const float Y = NormalizedQ.Y;
    const float Z = NormalizedQ.Z;
    const float W = NormalizedQ.W;

    const float XX = X * X;
    const float XY = X * Y;
    const float XZ = X * Z;
    const float XW = X * W;

    const float YY = Y * Y;
    const float YZ = Y * Z;
    const float YW = Y * W;

    const float ZZ = Z * Z;
    const float ZW = Z * W;

    Result.M[0][0] = 1.0f - 2.0f * (YY + ZZ);
    Result.M[0][1] = 2.0f * (XY - ZW);
    Result.M[0][2] = 2.0f * (XZ + YW);
    Result.M[0][3] = 0.0f;

    Result.M[1][0] = 2.0f * (XY + ZW);
    Result.M[1][1] = 1.0f - 2.0f * (XX + ZZ);
    Result.M[1][2] = 2.0f * (YZ - XW);
    Result.M[1][3] = 0.0f;

    Result.M[2][0] = 2.0f * (XZ - YW);
    Result.M[2][1] = 2.0f * (YZ + XW);
    Result.M[2][2] = 1.0f - 2.0f * (XX + YY);
    Result.M[2][3] = 0.0f;

    Result.M[3][0] = 0.0f;
    Result.M[3][1] = 0.0f;
    Result.M[3][2] = 0.0f;
    Result.M[3][3] = 1.0f;

    return Result;
}

FMatrix FMatrix::CreateScaleMatrix(const FVector& V)
{
    return {{
        {V.X, 0, 0, 0},
        {0, V.Y, 0, 0},
        {0, 0, V.Z, 0},
        {0, 0, 0, 1}
    }};
}

FVector FMatrix::TransformVector(const FVector& V, const FMatrix& M)
{
    FVector result;

    // 4x4 행렬을 사용하여 벡터 변환 (W = 0으로 가정, 방향 벡터)
    result.X = V.X * M.M[0][0] + V.Y * M.M[1][0] + V.Z * M.M[2][0] + 0.0f * M.M[3][0];
    result.Y = V.X * M.M[0][1] + V.Y * M.M[1][1] + V.Z * M.M[2][1] + 0.0f * M.M[3][1];
    result.Z = V.X * M.M[0][2] + V.Y * M.M[1][2] + V.Z * M.M[2][2] + 0.0f * M.M[3][2];


    return result;
}

// FVector4를 변환하는 함수
FVector4 FMatrix::TransformVector(const FVector4& V, const FMatrix& M)
{
    FVector4 result;
    result.X = V.X * M.M[0][0] + V.Y * M.M[1][0] + V.Z * M.M[2][0] + V.W * M.M[3][0];
    result.Y = V.X * M.M[0][1] + V.Y * M.M[1][1] + V.Z * M.M[2][1] + V.W * M.M[3][1];
    result.Z = V.X * M.M[0][2] + V.Y * M.M[1][2] + V.Z * M.M[2][2] + V.W * M.M[3][2];
    result.W = V.X * M.M[0][3] + V.Y * M.M[1][3] + V.Z * M.M[2][3] + V.W * M.M[3][3];
    return result;
}

FVector4 FMatrix::TransformFVector4(const FVector4& vector) const
{
    return FVector4{
        M[0][0] * vector.X + M[1][0] * vector.Y + M[2][0] * vector.Z + M[3][0] * vector.W,
        M[0][1] * vector.X + M[1][1] * vector.Y + M[2][1] * vector.Z + M[3][1] * vector.W,
        M[0][2] * vector.X + M[1][2] * vector.Y + M[2][2] * vector.Z + M[3][2] * vector.W,
        M[0][3] * vector.X + M[1][3] * vector.Y + M[2][3] * vector.Z + M[3][3] * vector.W
    };
}

FVector FMatrix::TransformPosition(const FVector& vector) const
{
    float x = M[0][0] * vector.X + M[1][0] * vector.Y + M[2][0] * vector.Z + M[3][0];
    float y = M[0][1] * vector.X + M[1][1] * vector.Y + M[2][1] * vector.Z + M[3][1];
    float z = M[0][2] * vector.X + M[1][2] * vector.Y + M[2][2] * vector.Z + M[3][2];
    float w = M[0][3] * vector.X + M[1][3] * vector.Y + M[2][3] * vector.Z + M[3][3];
    return w != 0.0f ? FVector{x / w, y / w, z / w} : FVector{x, y, z};
}

FQuat FMatrix::ToQuat() const
{
    return FQuat{*this};
}

FVector FMatrix::GetScaleVector(float Tolerance) const
{
    FVector Scale3D(1, 1, 1);

    // For each row, find magnitude, and if its non-zero re-scale so its unit length.
    for (int32 i = 0; i < 3; i++)
    {
        const float SquareSum = (M[i][0] * M[i][0]) + (M[i][1] * M[i][1]) + (M[i][2] * M[i][2]);
        if (SquareSum > Tolerance)
        {
            Scale3D[i] = FMath::Sqrt(SquareSum);
        }
        else
        {
            Scale3D[i] = 0.f;
        }
    }

    return Scale3D;
}

FVector FMatrix::GetTranslationVector() const
{
    return FVector{M[3][0], M[3][1], M[3][2]};
}

FMatrix FMatrix::GetMatrixWithoutScale(float Tolerance) const
{
    FMatrix Result = *this;
    Result.RemoveScaling(Tolerance);
    return Result;
}

void FMatrix::RemoveScaling(float Tolerance)
{
    const float SquareSum0 = (M[0][0] * M[0][0]) + (M[0][1] * M[0][1]) + (M[0][2] * M[0][2]);
    const float SquareSum1 = (M[1][0] * M[1][0]) + (M[1][1] * M[1][1]) + (M[1][2] * M[1][2]);
    const float SquareSum2 = (M[2][0] * M[2][0]) + (M[2][1] * M[2][1]) + (M[2][2] * M[2][2]);
    const float Scale0 = (SquareSum0 - Tolerance) >= 0.f ? FMath::InvSqrt(SquareSum0) : 1.f;
    const float Scale1 = (SquareSum1 - Tolerance) >= 0.f ? FMath::InvSqrt(SquareSum1) : 1.f;
    const float Scale2 = (SquareSum2 - Tolerance) >= 0.f ? FMath::InvSqrt(SquareSum2) : 1.f;
    M[0][0] *= Scale0;
    M[0][1] *= Scale0;
    M[0][2] *= Scale0;
    M[1][0] *= Scale1;
    M[1][1] *= Scale1;
    M[1][2] *= Scale1;
    M[2][0] *= Scale2;
    M[2][1] *= Scale2;
    M[2][2] *= Scale2;
}

bool FMatrix::Equals(const FMatrix& Other, float Tolerance) const
{
    for (int32 X = 0; X < 4; X++)
    {
        for (int32 Y = 0; Y < 4; Y++)
        {
            if (FMath::Abs(M[X][Y] - Other.M[X][Y]) > Tolerance)
            {
                return false;
            }
        }
    }

    return true;
}

FVector FMatrix::InverseTransformVector(const FVector& V) const
{
    FMatrix Inverse = FMatrix::Inverse(*this);
    return FMatrix::TransformVector(V, Inverse);
}
