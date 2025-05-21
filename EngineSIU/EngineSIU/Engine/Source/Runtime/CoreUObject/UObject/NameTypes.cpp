#include "NameTypes.h"

#include <cassert>
#include <atomic>
#include <cwchar>
#include <mutex>
#include "Core/Container/Map.h"
#include "Core/Container/String.h"
#include "Templates/TemplateUtilities.h"


enum ENameCase : uint8
{
    IgnoreCase,   // 대소문자 무시
    CaseSensitive // 대소문자 구분
};

/** ANSICAHR나 WIDECHAR를 담는 인터페이스 비슷한 클래스 */
struct FNameStringView
{
    FNameStringView() : Data(nullptr), Len(0), bIsWide(false) {}
    FNameStringView(const ANSICHAR* Str, uint32 InLen) : Ansi(Str), Len(InLen), bIsWide(false) {}
    FNameStringView(const WIDECHAR* Str, uint32 InLen) : Wide(Str), Len(InLen), bIsWide(true) {}
	FNameStringView(const void* InData, uint32 InLen, bool bInIsWide) : Data(InData), Len(InLen), bIsWide(bInIsWide) {}

	union
	{
		const void* Data;
		const ANSICHAR* Ansi;
		const WIDECHAR* Wide;
	};

	uint32 Len;
	bool bIsWide;

	bool IsAnsi() const { return !bIsWide; }
};


/** FName의 Hash값을 담는 클래스, FNameEntry에서 사용 */
struct FNameEntryId
{
	uint32 Value = 0;  // 비교 문자열이 있는 해시

	bool IsNone() const { return !Value; }

	bool operator==(const FNameEntryId& Other) const
	{
		return Value == Other.Value;
	}

	bool operator!=(const FNameEntryId& Other) const
	{
		return !(*this == Other);
	}

	explicit operator bool() const noexcept
	{
		return Value != 0;
	}
};

/** Entry에 담기는 Name의 정보 */
struct FNameEntryHeader
{
	uint16 IsWide : 1; // wchar인지 여부
	uint16 Len : 15;   // FName의 길이 0 ~ 32767
};


struct FNameEntry
{
	FNameEntryId ComparisonId; // 비교 문자열이 있는 해시
	FNameEntryHeader Header;   // Name의 정보

	union
	{
		ANSICHAR AnsiName[NAME_SIZE];
		WIDECHAR WideName[NAME_SIZE];
	};

	void StoreName(const ANSICHAR* InName, uint32 Len)
	{
		memcpy(AnsiName, InName, sizeof(ANSICHAR) * Len);
		AnsiName[Len] = '\0';
	}

	void StoreName(const WIDECHAR* InName, uint32 Len)
	{
		memcpy(WideName, InName, sizeof(WIDECHAR) * Len);
		WideName[Len] = '\0';
	}
};

namespace
{
template <typename CharType>
uint32 HashString(const CharType* Str)
{
	// djb2 문자열 해싱 알고리즘
	uint32 Hash = 5381;
	while (*Str)
	{
		Hash = ((Hash << 5) + Hash) + *Str;
		++Str;
	}
	return Hash;
}

template <typename CharType>
uint32 HashStringLower(const CharType* Str, uint32 InLen)
{
	CharType LowerStr[NAME_SIZE];
	if constexpr (std::is_same_v<CharType, wchar_t>)
	{
		for (uint32 i = 0; i < InLen; i++)
		{
			LowerStr[i] = towlower(Str[i]);
		}
		LowerStr[InLen] = '\0';
	}
	else
	{
		for (uint32 i = 0; i < InLen; ++i)
		{
			LowerStr[i] = static_cast<CharType>(tolower(Str[i]));
		}
		LowerStr[InLen] = '\0';
	}
	return HashString(LowerStr);
}

template <ENameCase Sensitivity>
uint32 HashName(FNameStringView InName);

template <>
uint32 HashName<IgnoreCase>(FNameStringView InName)
{
	return InName.IsAnsi() ? HashStringLower(InName.Ansi, InName.Len) : HashStringLower(InName.Wide, InName.Len);
}

template <>
uint32 HashName<CaseSensitive>(FNameStringView InName)
{
	return InName.IsAnsi() ? HashString(InName.Ansi) : HashString(InName.Wide);
}
}

template <ENameCase Sensitivity>
struct FNameValue
{
	explicit FNameValue(FNameStringView InName)
		: Name(InName)
		, Hash(HashName<Sensitivity>(InName))
	{}

	FNameStringView Name;
	uint32 Hash;
	FNameEntryId ComparisonId = {};
};

using FNameComparisonValue = FNameValue<IgnoreCase>;
using FNameDisplayValue = FNameValue<CaseSensitive>;

/** Open Addressing을 이용한 커스텀 Hash Set 이었는데, Hash 충돌 문제로 사용 못함 */
#pragma region Deprecated Pool Shard
#if 0  // NOLINT(readability-avoid-unconditional-preprocessor-if)
template <ENameCase Sensitivity>
class FNamePoolShard
{
	template <typename T>
	using Vec = std::vector<T, FDefaultAllocator<T>>;

	// 요소 상태를 나타내는 enum
	enum class ESlotState : uint8
	{
		Empty,    // 비어있는 슬롯
		Occupied, // 사용 중인 슬롯
		Deleted   // 삭제된 슬롯
	};

	// 슬롯 구조체
	struct FSlot
	{
		FNameEntry Value;   // 저장된 값
		ESlotState State;    // 슬롯의 상태

		FSlot()
			: Value()
			, State(ESlotState::Empty)
		{
		}

		bool Used() const
		{
			return State == ESlotState::Occupied;
		}

		void SetValue(const FNameEntry& InValue)
		{
			Value = InValue;
			State = ESlotState::Occupied;
		}
	};

private:
	mutable Vec<FSlot> Slots;
	std::atomic<uint32> ElementCount; // 총 요소 개수
	float MaxLoadFactorValue;        // 최대 부하 계수

	mutable std::mutex Mutex;

	// 해시 테이블 리사이징 및 리해싱
	void Rehash(uint32 NewCapacity)
	{
		Vec<FSlot> OldSlots = std::move(Slots);
		Slots.clear();
		Slots.resize(NewCapacity);

		ElementCount = 0;
		for (const auto& Slot : OldSlots)
		{
			if (Slot.State == ESlotState::Occupied)
			{
				Slots[Slot.Value.ComparisonId.Value & Slots.size()] = Slot;
			}
		}
	}

public:
	FNamePoolShard(int32 InitCapacity = 1024, float InMaxLoadFactor = 0.75f)
		: Slots(InitCapacity)
		, ElementCount(0)
		, MaxLoadFactorValue(InMaxLoadFactor)
	{
	}

	FSlot& Probe(uint32 InHash) const
	{
		return Slots[InHash & Slots.size()-1];
	}

	FNameEntryId Find(const FNameValue<Sensitivity>& Value) const
	{
		FNameEntryId Result;
		{
			std::lock_guard _(Mutex);

			const FSlot& Slot = Probe(Value.Hash);
			Result = Slot.Value.ComparisonId;
		}
		return Result;
	}

	FNameEntryId Insert(const FNameValue<Sensitivity>& Value)
	{
		// 부하 계수 확인 및 필요시 리해싱
		if (ElementCount >= Slots.size() * MaxLoadFactorValue)
		{
			Rehash(Slots.size() * 2);
		}

		std::lock_guard _(Mutex);
		FSlot& Slot = Probe(Value.Hash);

		if (Slot.Used())
		{
			return Slot.Value.ComparisonId;
		}

		// 요소 삽입
		FNameEntry Entry;
		Entry.ComparisonId = Value.ComparisonId;
		Entry.Header = {
			.IsWide = Value.Name.bIsWide,
			.Len = static_cast<uint16>(Value.Name.Len)
		};
		if (Value.Name.bIsWide)
		{
			Entry.StoreName(Value.Name.Wide, Value.Name.Len);
		}
		else
		{
			Entry.StoreName(Value.Name.Ansi, Value.Name.Len);
		}
		Slot.SetValue(Entry);
		ElementCount.fetch_add(1, std::memory_order_relaxed);
		return {Value.Hash};
	}
};
#endif
#pragma endregion Deprecated Pool Shard

struct FNamePool
{
public:
    static FNamePool& Get()
    {
        static FNamePool Instance;
        return Instance;
    }

private:
	// FNamePoolShard<CaseSensitive> DisplayPool;
	// FNamePoolShard<IgnoreCase> ComparisonPool;

	TMap<uint32, FNameEntry> DisplayPool;
	TMap<uint32, FNameEntry> ComparisonPool;

private:
	template <ENameCase Sensitivity>
	FNameEntry MakeEntry(const FNameValue<Sensitivity>& Value) const
	{
		FNameEntry Result;
		Result.ComparisonId = Value.ComparisonId;
		Result.Header = {
			.IsWide = Value.Name.bIsWide,
			.Len = static_cast<uint16>(Value.Name.Len)
		};
		if (Value.Name.bIsWide)
		{
			Result.StoreName(Value.Name.Wide, Value.Name.Len);
		}
		else
		{
			Result.StoreName(Value.Name.Ansi, Value.Name.Len);
		}
		return Result;
	}

public:
	/** Hash로 원본 문자열을 가져옵니다. */
	const FNameEntry& Resolve(uint32 Hash) const
	{
	    const FNameEntry* Entry = DisplayPool.Find(Hash);
        assert(Entry && "Failed to resolve FNameEntry for the given Hash. The Hash might not exist in the DisplayPool.");
        return *Entry;
	}

	/**
	 * 문자열을 찾거나, 없으면 Hash화 해서 저장합니다.
	 *
	 * @return DisplayName의 Hash
	 */
	FNameEntryId FindOrStoreString(const FNameStringView& Name)
	{
		// DisplayPool에 같은 문자열이 있다면, 문자열의 Hash 반환
		FNameDisplayValue DisplayValue{Name};
		if (DisplayPool.Find(DisplayValue.Hash))
		{
			return {DisplayValue.Hash};
		}

		const FNameComparisonValue ComparisonValue{Name};
		if (!ComparisonPool.Find(ComparisonValue.Hash))
		{
			const FNameEntry Entry = MakeEntry(ComparisonValue);
			ComparisonPool.Add(ComparisonValue.Hash, Entry);
		}

		DisplayValue.ComparisonId = {ComparisonValue.Hash};
		DisplayPool.Add(DisplayValue.Hash, MakeEntry(DisplayValue));
		return {DisplayValue.Hash};
	}
};

struct FNameHelper
{
	template <typename CharType>
	static FName MakeFName(const CharType* Str)
	{
		if constexpr (std::is_same_v<CharType, char>)
		{
			return MakeFName(Str, static_cast<uint32>(strlen(Str)));
		}
		else if constexpr (std::is_same_v<CharType, wchar_t>)
		{
			return MakeFName(Str, static_cast<uint32>(wcslen(Str)));
		}
		else
		{
			static_assert(TAlwaysFalse<CharType>, "Invalid Character type");
			return {};
		}
	}

	template <typename CharType>
	static FName MakeFName(const CharType* Char, uint32 Len)
	{
		// 문자열의 길이가 NAME_SIZE를 초과하면 None 반환
		if (Len >= NAME_SIZE)
		{
		    assert(0 && "FName size is too large");
			return {};
		}

		const FNameEntryId DisplayId = FNamePool::Get().FindOrStoreString({Char, Len});

		FName Result;
		Result.DisplayIndex = DisplayId.Value;
		Result.ComparisonIndex = ResolveComparisonId(DisplayId).Value;
		#if defined(_DEBUG)
        Result.DebugEntryPtr = &FNamePool::Get().Resolve(DisplayId.Value);
#endif
		return Result;
	}

	static FNameEntryId ResolveComparisonId(FNameEntryId DisplayId)
	{
		if (DisplayId.IsNone())
		{
			return {};
		}
		return FNamePool::Get().Resolve(DisplayId.Value).ComparisonId;
	}
};

#if defined(_DEBUG)
// .natvis에서 사용하는 디버그용 변수
namespace { [[maybe_unused]] const FNamePool& GDebugNamePool = FNamePool::Get(); }
#endif

FName::FName(const WIDECHAR* Name)
	: FName(FNameHelper::MakeFName(Name))
{
}

FName::FName(const WIDECHAR* Name, uint32 Len)
	: FName(FNameHelper::MakeFName(Name, Len))
{
}

FName::FName(const ANSICHAR* Name)
	: FName(FNameHelper::MakeFName(Name))
{
}

FName::FName(const ANSICHAR* Name, uint32 Len)
	: FName(FNameHelper::MakeFName(Name, Len))
{
}

FName::FName(const FString& Name)
	: FName(FNameHelper::MakeFName(*Name, Name.Len()))
{
}

FString FName::ToString() const
{
	if (DisplayIndex == 0 && ComparisonIndex == 0)
	{
		return {TEXT("None")};
	}

	// TODO: WIDECHAR에 대응 해야함
	FNameEntry Entry = FNamePool::Get().Resolve(DisplayIndex);
    if (Entry.Header.IsWide)
    {
        return {Entry.WideName};
    }
    else
    {
        return {Entry.AnsiName};
    }
}

bool FName::operator==(const FName& Other) const
{
	return ComparisonIndex == Other.ComparisonIndex;
}

bool FName::operator==(ENameNone) const
{
    return ComparisonIndex == NAME_None;
}

bool FName::operator!=(ENameNone) const
{
    return ComparisonIndex != NAME_None;
}

bool FName::operator!=(const FName& Other) const
{
    return ComparisonIndex != Other.ComparisonIndex;
}
