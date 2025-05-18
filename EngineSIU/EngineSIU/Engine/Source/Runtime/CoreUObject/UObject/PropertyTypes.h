#pragma once
#include <concepts>
#include "Container/Array.h"
#include "Container/Map.h"
#include "Container/Set.h"
#include "Math/Transform.h"
#include "Template/IsTSubclassOf.h"
#include "Templates/IsArray.h"

#include "magic_enum/magic_enum.hpp"
#include "Templates/TemplateUtilities.h"


enum class EPropertyType : uint8
{
    Unknown,                       // 알 수 없는 타입
    UnresolvedPointer,             // 컴파일 타임에 알 수 없는 포인터 타입

    Int8, Int16, Int32, Int64,     // 부호 있는 정수타입
    UInt8, UInt16, UInt32, UInt64, // 부호 없는 정수타입
    Float, Double,                 // 실수 타입
    Bool,                          // Boolean 타입

    String,                        // 문자열 타입 (FString)
    Name,                          // 이름 타입 (FName)
    Vector2D,                      // FVector2D
    Vector,                        // FVector
    Vector4,                       // FVector4
    Rotator,                       // FRotator
    Quat,                          // FQuat
    Transform,                     // FTransform
    Matrix,                        // FMatrix
    Color,                         // FColor
    LinearColor,                   // FLinearColor

    Array,                         // TArray<T>
    Map,                           // TMap<T>
    Set,                           // TSet<T>

    Enum,                          // 커스텀 Enum 타입
    Struct,                        // 사용자 정의 구조체 타입
    StructPointer,                 // 사용자 정의 구조체 포인터 타입
    SubclassOf,                    // TSubclassOf
    Object,                        // UObject* 타입
};

template <typename T>
consteval EPropertyType GetPropertyType()
{
    // 기본 내장 타입들
    if constexpr (std::same_as<T, int8>)              { return EPropertyType::Int8;        }
    else if constexpr (std::same_as<T, int16>)        { return EPropertyType::Int16;       }
    else if constexpr (std::same_as<T, int32>)        { return EPropertyType::Int32;       }
    else if constexpr (std::same_as<T, int64>)        { return EPropertyType::Int64;       }
    else if constexpr (std::same_as<T, uint8>)        { return EPropertyType::UInt8;       }
    else if constexpr (std::same_as<T, uint16>)       { return EPropertyType::UInt16;      }
    else if constexpr (std::same_as<T, uint32>)       { return EPropertyType::UInt32;      }
    else if constexpr (std::same_as<T, uint64>)       { return EPropertyType::UInt64;      }
    else if constexpr (std::same_as<T, float>)        { return EPropertyType::Float;       }
    else if constexpr (std::same_as<T, double>)       { return EPropertyType::Double;      }
    else if constexpr (std::same_as<T, bool>)         { return EPropertyType::Bool;        }

    // 엔진 기본 타입들
    else if constexpr (std::same_as<T, FString>)      { return EPropertyType::String;      }
    else if constexpr (std::same_as<T, FName>)        { return EPropertyType::Name;        }
    else if constexpr (std::same_as<T, FVector2D>)    { return EPropertyType::Vector2D;    }
    else if constexpr (std::same_as<T, FVector>)      { return EPropertyType::Vector;      }
    else if constexpr (std::same_as<T, FVector4>)     { return EPropertyType::Vector4;     }
    else if constexpr (std::same_as<T, FRotator>)     { return EPropertyType::Rotator;     }
    else if constexpr (std::same_as<T, FQuat>)        { return EPropertyType::Quat;        }
    else if constexpr (std::same_as<T, FTransform>)   { return EPropertyType::Transform;   }
    else if constexpr (std::same_as<T, FMatrix>)      { return EPropertyType::Matrix;      }
    else if constexpr (std::same_as<T, FColor>)       { return EPropertyType::Color;       }
    else if constexpr (std::same_as<T, FLinearColor>) { return EPropertyType::LinearColor; }

    // TSubclassOf
    else if constexpr (TIsTSubclassOf<T>)             { return EPropertyType::SubclassOf;  }

    // 포인터 타입
    else if constexpr (std::is_pointer_v<T>)
    {
        using PointedToType = std::remove_cv_t<std::remove_pointer_t<T>>;
        
        // if문을 주석처리한 이유는 여기서 컴파일 타임에 UObject를 상속받은 클래스에 대해서 상속여부 검사를 하는데, 이때 UObject의 IsA<T>를 실제로 인스턴싱을 하게 됩니다.
        // 하지만 이 시점에서 IsA의 requires std::derived_from<T, UObject>가 T에 대한 완전한 타입정보를 가지고 있지 않기 때문에 false로 평가되어 컴파일 에러가 발생합니다.
        // 지금은 그냥 IsA의 requires를 제거하였습니다.
        
        // PointedToType가 완전한 타입일 때만 true를 반환.
        // 전방 선언 시 false가 될 수 있음.
        if constexpr (std::derived_from<PointedToType, UObject>)
        {
            return EPropertyType::Object;
        }

        // 커스텀 구조체 포인터
        else if constexpr (std::is_class_v<PointedToType> && requires { PointedToType::StaticStruct(); })
        {
            // 언리얼에서도 커스텀 구조체 포인터에 대해서 지원 안하길래,
            // UI 구현하기 귀찮으니까 그냥 static_assert를 넣었습니다.
            // static_assert 지워도 리플렉션 시스템에 정상 등록은 되지만, UI는 뜨지 않습니다.
            static_assert(TAlwaysFalse<T>, "Custom struct pointer types are not supported.");
            return EPropertyType::StructPointer;
        }

        // 전방 선언된 타입이 들어올 경우, 상속관계를 확인할 수 없음
        // 이때는 UObject일 확률도 있기 때문에 런타임에 검사
        return EPropertyType::UnresolvedPointer;
    }

    // 엔진 기본 컨테이너 타입들
    else if constexpr (TIsTArray<T> || TIsArray<T>) { return EPropertyType::Array;  }
    else if constexpr (TIsTMap<T>)                  { return EPropertyType::Map;    }
    else if constexpr (TIsTSet<T>)                  { return EPropertyType::Set;    }

    // enum class만 지원
    else if constexpr (std::is_scoped_enum_v<T>)    { return EPropertyType::Enum;   }

    // 커스텀 구조체
    else if constexpr (std::is_class_v<T> && !std::derived_from<T, UObject>)
    {
        if constexpr (requires { T::StaticStruct(); })
        {
            return EPropertyType::Struct;
        }
        else
        {
            // Reflection System에 등록이 되어있지 않은 구조체면 컴파일 에러
            static_assert(
                TAlwaysFalse<T>,
                "The type T is not registered in the reflection system. "
                "Make sure to register it using DECLARE_STRUCT() macro or provide StaticStruct() function."
            );
            return EPropertyType::Unknown;
        }
    }

    else
    {
        static_assert(TAlwaysFalse<T>, "GetPropertyType: Type T is not supported.");
        return EPropertyType::Unknown;
    }
}

enum EPropertyFlags : uint32  // NOLINT(performance-enum-size)
{
    PropertyNone    = 0,         // 플래그 없음
    VisibleAnywhere = 1 << 0,    // ImGui에서 읽기 전용으로 표시
    EditAnywhere    = 1 << 1,    // ImGui에서 읽기/쓰기 가능
    LuaReadOnly     = 1 << 2,    // Lua에 읽기 전용으로 바인딩
    LuaReadWrite    = 1 << 3,    // Lua에 읽기/쓰기로 바인딩
    // ... 필요한 다른 플래그들 (예: SaveGame, Replicated 등)
};

// magic_enum에서 EPropertyFlags를 사용하기 위함
template <>
struct magic_enum::customize::enum_range<EPropertyFlags>
{
    static constexpr bool is_flags = true;
};

// 비트 플래그 연산을 위한 헬퍼 함수들
constexpr EPropertyFlags operator|(EPropertyFlags Lhs, EPropertyFlags Rhs)
{
    return static_cast<EPropertyFlags>(static_cast<uint32>(Lhs) | static_cast<uint32>(Rhs));
}

constexpr EPropertyFlags& operator|=(EPropertyFlags& Lhs, EPropertyFlags Rhs)
{
    Lhs = Lhs | Rhs;
    return Lhs;
}

constexpr bool HasAllFlags(EPropertyFlags Flags, EPropertyFlags FlagToCheck)
{
    return (static_cast<uint32>(Flags) & static_cast<uint32>(FlagToCheck)) == static_cast<uint32>(FlagToCheck);
}

template <EPropertyFlags Flags>
constexpr bool HasAllFlags(EPropertyFlags FlagToCheck)
{
    return HasAllFlags(Flags, FlagToCheck);
}

constexpr bool HasAnyFlags(EPropertyFlags Flags, EPropertyFlags FlagToCheck)
{
    return (static_cast<uint32>(Flags) & static_cast<uint32>(FlagToCheck)) != 0;
}

template <EPropertyFlags Flags>
constexpr bool HasAnyFlags(EPropertyFlags FlagToCheck)
{
    return HasAnyFlags(Flags, FlagToCheck);
}

// 특정 플래그만 제외하는 연산자
constexpr EPropertyFlags operator~(EPropertyFlags Flags)
{
    return static_cast<EPropertyFlags>(~static_cast<uint32>(Flags));
}

constexpr EPropertyFlags operator&(EPropertyFlags Lhs, EPropertyFlags Rhs)
{
    return static_cast<EPropertyFlags>(static_cast<uint32>(Lhs) & static_cast<uint32>(Rhs));
}
