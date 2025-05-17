#pragma once
#include <string_view>

/**
 * 타입을 문자열화 합니다.
 * @tparam T 문자열화 할 타입 이름
 * @return 문자열화 된 타입
 *
 * @warning string_view는 직접 값을 바꾸는게 아닌 data section에 있는 __FUNCSIG__문자열을 가리키기 때문에, view.data()로 접근하면 추가적인 문자열이 들어갈 수 있습니다.
 *          이때는 .data()와 .size()를 이용해서 직접 포인터 연산을 하거나, std::string으로 변환해서 사용하면 정상적으로 사용할 수 있습니다.
 */
template <typename T>
constexpr std::string_view GetTypeNameString(bool bIsNameOnly = true)
{
#if defined(__clang__)
    constexpr std::string_view SignatureValue = __PRETTY_FUNCTION__;
    constexpr std::string_view ActualPrefixPattern = " T = ";
    constexpr auto PrefixStartPos = SignatureValue.find(ActualPrefixPattern);
    if (PrefixStartPos == std::string_view::npos) return "__UnknownType_Clang_PrefixFail__";
    constexpr size_t ActualNameStartPosition = PrefixStartPos + ActualPrefixPattern.size();

    constexpr std::string_view SuffixString = "]";
    constexpr auto EndPosition = SignatureValue.rfind(SuffixString);
    if (EndPosition == std::string_view::npos || EndPosition <= ActualNameStartPosition) return "__UnknownType_Clang_SuffixFail__";

    std::string_view RawTypeNameString = SignatureValue.substr(ActualNameStartPosition, EndPosition - ActualNameStartPosition);

    if (!bIsNameOnly)
    {
        constexpr auto TrimWhitespaceLight = [](std::string_view Sv) -> std::string_view {
            if (Sv.empty()) return Sv;
            size_t First = 0;
            while (First < Sv.length() && (Sv[First] == ' ' || Sv[First] == '\t')) {
                First++;
            }
            size_t Last = Sv.length();
            while (Last > First && (Sv[Last - 1] == ' ' || Sv[Last - 1] == '\t')) {
                Last--;
            }
            return Sv.substr(First, Last - First);
        };
        return TrimWhitespaceLight(RawTypeNameString);
    }
    // Clang의 경우에도 네임스페이스 제거가 필요하다면 여기에 로직 추가 가능
    // 예: auto last_colon = RawTypeNameString.rfind("::");
    //     if (last_colon != std::string_view::npos) {
    //         RawTypeNameString = RawTypeNameString.substr(last_colon + 2);
    //     }
    return RawTypeNameString;


#elif defined(__GNUC__)
    constexpr std::string_view SignatureValue = __PRETTY_FUNCTION__;
    constexpr std::string_view ActualPrefixPattern = " T = ";
    constexpr auto PrefixStartPos = SignatureValue.find(ActualPrefixPattern);
    if (PrefixStartPos == std::string_view::npos) return "__UnknownType_GCC_PrefixFail__";

    constexpr size_t ActualNameStartPosition = PrefixStartPos + ActualPrefixPattern.size();

    constexpr std::string_view SuffixString = "]";
    constexpr auto EndPosition = SignatureValue.rfind(SuffixString);
    if (EndPosition == std::string_view::npos || EndPosition <= ActualNameStartPosition) return "__UnknownType_GCC_SuffixFail__";

    std::string_view RawTypeNameString = SignatureValue.substr(ActualNameStartPosition, EndPosition - ActualNameStartPosition);

    if (!bIsNameOnly)
    {
        constexpr auto TrimWhitespaceLight = [](std::string_view Sv) -> std::string_view {
             if (Sv.empty()) return Sv;
            size_t First = 0;
            while (First < Sv.length() && (Sv[First] == ' ' || Sv[First] == '\t')) {
                First++;
            }
            size_t Last = Sv.length();
            while (Last > First && (Sv[Last - 1] == ' ' || Sv[Last - 1] == '\t')) {
                Last--;
            }
            return Sv.substr(First, Last - First);
        };
        return TrimWhitespaceLight(RawTypeNameString);
    }
    return RawTypeNameString;

#elif defined(_MSC_VER)
    constexpr std::string_view FullSignature = __FUNCSIG__;

    // GetTypeNameString< 앞부분 찾기
    constexpr std::string_view GetTypeNameStringMarker = "GetTypeNameString<";
    constexpr auto TypeNameStartMarkerPos = FullSignature.find(GetTypeNameStringMarker);
    constexpr size_t ActualTypeNameStartPosition = TypeNameStartMarkerPos + GetTypeNameStringMarker.size();

    // >( 파라미터 시작 전의 꺾쇠 닫기
    // 또는 마지막 > 를 찾고, 그게 파라미터 리스트의 닫는 괄호인지 확인
    // MSVC에서 템플릿 인자가 여러 개인 경우(Foo<Bar<int>, double>) 가장 바깥쪽 꺽쇠를 찾아야 합니다.
    // __FUNCSIG__ 에서 타입 부분만 정확히 추출하는 것은 매우 까다로울 수 있습니다.
    // 여기서는 단순한 경우를 가정하여 마지막 '>'를 찾고 그 이후에 ' ('가 오는지 확인합니다.

    size_t AngleBracketDepth = 0;
    size_t TypeEndPosition = ActualTypeNameStartPosition;
    bool FoundValidEnd = false;
    for (size_t i = ActualTypeNameStartPosition; i < FullSignature.length(); ++i)
    {
        if (FullSignature[i] == '<')
        {
            AngleBracketDepth++;
        }
        else if (FullSignature[i] == '>')
        {
            if (AngleBracketDepth == 0)
            {
                // 가장 바깥쪽 닫는 꺽쇠
                TypeEndPosition = i;
                // 이 꺽쇠 다음에 바로 함수 파라미터 시작 '('가 오는지 확인
                // 또는 함수 시그니처의 끝을 나타내는 SuffixString ("(bool)" 등)으로 확인
                if (i + 1 < FullSignature.length() && FullSignature[i + 1] == '(')
                {
                    // 예: ">("
                    FoundValidEnd = true;
                    break;
                }
                else if (FullSignature.substr(i).starts_with(">(bool)"))
                {
                    // 더 구체적인 패턴
                    FoundValidEnd = true;
                    break;
                }
            }
            else
            {
                AngleBracketDepth--;
            }
        }
    }
    if (!FoundValidEnd)
    {
        // 가장 바깥쪽 닫는 꺾쇠를 찾지 못했거나, 그 뒤 패턴이 안 맞으면
        // 대체 로직: 마지막 '>'를 찾는다.
        // 이는 중첩 템플릿 (예: MyType<OtherType<int>>)에서 내부 템플릿의 '>'를 잡을 위험이 있음.
        // __FUNCSIG__ 구조에 대한 강한 가정이 필요함.
        // 일반적으로 GetTypeNameString<...TYPE...>(bool...) 형태일 것이므로,
        // 마지막 '>'가 타입의 끝일 가능성이 높습니다.
        auto TempEndPos = FullSignature.rfind('>');
        // 그 '>'가 함수 파라미터 '(' 앞에 있는지 확인
        auto ParamOpenParen = FullSignature.find('(', ActualTypeNameStartPosition);
        if (TempEndPos != std::string_view::npos && ParamOpenParen != std::string_view::npos && TempEndPos <
            ParamOpenParen)
        {
            TypeEndPosition = TempEndPos;
        }
        else if (TempEndPos != std::string_view::npos)
        {
            // 백업으로 마지막 '>' 사용
            TypeEndPosition = TempEndPos;
        }
        else
        {
            return "__UnknownType_MSVC_SuffixFail__";
        }
    }


    std::string_view RawTypeNameString = FullSignature.substr(
        ActualTypeNameStartPosition, TypeEndPosition - ActualTypeNameStartPosition
    );

    constexpr auto TrimWhitespace = [](std::string_view Sv) -> std::string_view
    {
        if (Sv.empty())
        {
            return Sv;
        }
        const char* WhitespaceChars = " \t\n\r\f\v";
        auto FirstNonWhitespace = Sv.find_first_not_of(WhitespaceChars);
        if (std::string_view::npos == FirstNonWhitespace)
        {
            return {};
        }
        auto LastNonWhitespace = Sv.find_last_not_of(WhitespaceChars);
        return Sv.substr(FirstNonWhitespace, LastNonWhitespace - FirstNonWhitespace + 1);
    };

    // constexpr로 알파벳/숫자인지 확인하는 헬퍼 람다
    constexpr auto IsAlnumChar = [](char C) -> bool
    {
        return (C >= 'a' && C <= 'z') ||
            (C >= 'A' && C <= 'Z') ||
            (C >= '0' && C <= '9');
    };

    if (!bIsNameOnly)
    {
        return TrimWhitespace(RawTypeNameString);
    }

    std::string_view ProcessedTypeNameString = TrimWhitespace(RawTypeNameString);
    bool WasModified;

    // 후행 한정자 (포인터, 참조, cv-한정자) 제거
    do
    {
        WasModified = false;
        std::string_view TempTrimmedString = TrimWhitespace(ProcessedTypeNameString);

        if (TempTrimmedString.ends_with("&&"))
        {
            ProcessedTypeNameString = TempTrimmedString.substr(0, TempTrimmedString.length() - 2);
            WasModified = true;
        }
        else if (TempTrimmedString.ends_with("&"))
        {
            ProcessedTypeNameString = TempTrimmedString.substr(0, TempTrimmedString.length() - 1);
            WasModified = true;
        }
        else if (TempTrimmedString.ends_with("*"))
        {
            ProcessedTypeNameString = TempTrimmedString.substr(0, TempTrimmedString.length() - 1);
            WasModified = true;
        }
        else if (TempTrimmedString.length() >= 6 && TempTrimmedString.substr(TempTrimmedString.length() - 6) ==
            " const")
        {
            ProcessedTypeNameString = TempTrimmedString.substr(0, TempTrimmedString.length() - 6);
            WasModified = true;
        }
        else if (TempTrimmedString.length() >= 5 && TempTrimmedString.substr(TempTrimmedString.length() - 5) == "const")
        {
            if (TempTrimmedString.length() == 5 ||
                (TempTrimmedString.length() > 5 && !IsAlnumChar(TempTrimmedString[TempTrimmedString.length() - 6])))
            {
                ProcessedTypeNameString = TempTrimmedString.substr(0, TempTrimmedString.length() - 5);
                WasModified = true;
            }
        }
        else if (TempTrimmedString.length() >= 9 && TempTrimmedString.substr(TempTrimmedString.length() - 9) ==
            " volatile")
        {
            ProcessedTypeNameString = TempTrimmedString.substr(0, TempTrimmedString.length() - 9);
            WasModified = true;
        }
        else if (TempTrimmedString.length() >= 8 && TempTrimmedString.substr(TempTrimmedString.length() - 8) ==
            "volatile")
        {
            if (TempTrimmedString.length() == 8 ||
                (TempTrimmedString.length() > 8 && !IsAlnumChar(TempTrimmedString[TempTrimmedString.length() - 9])))
            {
                ProcessedTypeNameString = TempTrimmedString.substr(0, TempTrimmedString.length() - 8);
                WasModified = true;
            }
        }

        if (WasModified)
        {
            ProcessedTypeNameString = TrimWhitespace(ProcessedTypeNameString);
        }
    }
    while (WasModified);

    // 선행 cv-한정자 제거
    do
    {
        WasModified = false;
        std::string_view TempTrimmedString = TrimWhitespace(ProcessedTypeNameString);

        if (TempTrimmedString.length() >= 6 && TempTrimmedString.substr(0, 6) == "const ")
        {
            ProcessedTypeNameString = TempTrimmedString.substr(6);
            WasModified = true;
        }
        else if (TempTrimmedString.length() >= 5 && TempTrimmedString.substr(0, 5) == "const")
        {
            if (TempTrimmedString.length() == 5 ||
                (TempTrimmedString.length() > 5 && !IsAlnumChar(TempTrimmedString[5])))
            {
                ProcessedTypeNameString = TempTrimmedString.substr(5);
                WasModified = true;
            }
        }
        else if (TempTrimmedString.length() >= 9 && TempTrimmedString.substr(0, 9) == "volatile ")
        {
            ProcessedTypeNameString = TempTrimmedString.substr(9);
            WasModified = true;
        }
        else if (TempTrimmedString.length() >= 8 && TempTrimmedString.substr(0, 8) == "volatile")
        {
            if (TempTrimmedString.length() == 8 ||
                (TempTrimmedString.length() > 8 && !IsAlnumChar(TempTrimmedString[8])))
            {
                ProcessedTypeNameString = TempTrimmedString.substr(8);
                WasModified = true;
            }
        }

        if (WasModified)
        {
            ProcessedTypeNameString = TrimWhitespace(ProcessedTypeNameString);
        }
    }
    while (WasModified);

    // 선행 타입 키워드 ("class ", "struct ", "enum ", "union ") 제거
    constexpr std::string_view TypePrefixKeywords[] = {"class ", "struct ", "enum ", "union "};
    do
    {
        WasModified = false;
        std::string_view TempTrimmedString = TrimWhitespace(ProcessedTypeNameString);
        for (const std::string_view KeywordPrefix : TypePrefixKeywords)
        {
            if (TempTrimmedString.starts_with(KeywordPrefix))
            {
                ProcessedTypeNameString = TempTrimmedString.substr(KeywordPrefix.size());
                WasModified = true;
                break;
            }
        }
        if (WasModified)
        {
            ProcessedTypeNameString = TrimWhitespace(ProcessedTypeNameString);
        }
    }
    while (WasModified);

    // 네임스페이스 제거 (bIsNameOnly == true 인 경우에만 적용)
    // 이 로직은 다른 모든 정제 작업 (cv, ptr/ref, class/struct 키워드) 이후에 적용됩니다.
    std::string_view FinalTypeNameString = TrimWhitespace(ProcessedTypeNameString);
    auto LastColonColonPos = FinalTypeNameString.rfind("::");
    if (LastColonColonPos != std::string_view::npos)
    {
        // "::" 뒤에 실제 이름이 있는지 확인 (예: "MyNamespace::"와 같은 경우 방지)
        if (LastColonColonPos + 2 < FinalTypeNameString.length())
        {
            FinalTypeNameString = FinalTypeNameString.substr(LastColonColonPos + 2);
        }
    }

    return FinalTypeNameString;

#else
    return "__UnknownType__";
#endif
}

#pragma region GetTypeNameString Test Cases
#if false  // NOLINT(readability-avoid-unconditional-preprocessor-if)
enum class MyEnum {};
inline void RunTypeNameTests()
{
    std::cout << std::boolalpha; // bool 값을 "true" 또는 "false"로 출력

    std::cout << "--- GetTypeNameString Test Cases ---" << std::endl;

    // 헬퍼 함수: 테스트 결과를 출력
    auto PrintTestResult = [](const char* testName, std::string_view expected, std::string_view actualTrue, std::string_view actualFalse)
    {
        std::cout << "\nTest: " << testName << std::endl;
        bool passed = (actualTrue == expected);
        assert(passed);
        std::cout << "  bIsNameOnly = true: " << (passed ? "PASSED" : "FAILED") << std::endl;
        std::cout << "    Expected: \"" << expected << "\"" << std::endl;
        std::cout << "    Actual  : \"" << actualTrue << "\"" << std::endl;
        std::cout << "  bIsNameOnly = false (Raw Signature View for MSVC/etc.):" << std::endl;
        std::cout << "    Actual  : \"" << actualFalse << "\"" << std::endl;
    };

    // 케이스 1: <MyEnum ****** * ** >
    // C++ 타입으로 해석: MyEnum********* (9개의 포인터)
    // bIsNameOnly = true 시 기대 결과: "MyEnum"
    {
        using TestType1 = MyEnum*********; // 9개의 '*'
        PrintTestResult("MyEnum*********", "MyEnum", GetTypeNameString<TestType1>(true), GetTypeNameString<TestType1>(false));
    }

    // 케이스 2: <volatile const volatile const MyEnum ** * ** const volatile>
    // C++ 타입으로 해석: const volatile MyEnum***** const volatile
    // (C++에서 'volatile const volatile const MyEnum'은 'const volatile MyEnum'과 동일)
    // ('** * **'는 5개의 포인터로 해석: *****)
    // bIsNameOnly = true 시 기대 결과: "MyEnum"
    {
        using TestType2 = const volatile MyEnum***** const volatile;
        PrintTestResult("const volatile MyEnum***** const volatile", "MyEnum", GetTypeNameString<TestType2>(true), GetTypeNameString<TestType2>(false));
    }

    // === 추가적인 일반 테스트 케이스 ===

    PrintTestResult("int", "int", GetTypeNameString<int>(true), GetTypeNameString<int>(false));
    PrintTestResult("const int", "int", GetTypeNameString<const int>(true), GetTypeNameString<const int>(false));
    PrintTestResult("volatile int", "int", GetTypeNameString<volatile int>(true), GetTypeNameString<volatile int>(false));
    PrintTestResult("const volatile int", "int", GetTypeNameString<const volatile int>(true), GetTypeNameString<const volatile int>(false));

    PrintTestResult("int*", "int", GetTypeNameString<int*>(true), GetTypeNameString<int*>(false));
    PrintTestResult("const int*", "int", GetTypeNameString<const int*>(true), GetTypeNameString<const int*>(false)); // int가 const, 포인터는 아님
    PrintTestResult("int* const", "int", GetTypeNameString<int* const>(true), GetTypeNameString<int* const>(false)); // 포인터가 const
    PrintTestResult("int* volatile", "int", GetTypeNameString<int* volatile>(true), GetTypeNameString<int* volatile>(false));
    PrintTestResult("int* const volatile", "int", GetTypeNameString<int* const volatile>(true), GetTypeNameString<int* const volatile>(false));
    PrintTestResult("const int* const", "int", GetTypeNameString<const int* const>(true), GetTypeNameString<const int* const>(false));

    PrintTestResult("int&", "int", GetTypeNameString<int&>(true), GetTypeNameString<int&>(false));
    PrintTestResult("const int&", "int", GetTypeNameString<const int&>(true), GetTypeNameString<const int&>(false));
    PrintTestResult("volatile int&", "int", GetTypeNameString<volatile int&>(true), GetTypeNameString<volatile int&>(false));
    PrintTestResult("int&&", "int", GetTypeNameString<int&&>(true), GetTypeNameString<int&&>(false));

    // 사용자 정의 타입
    struct AnotherStruct
    {
    };
    PrintTestResult("AnotherStruct", "AnotherStruct", GetTypeNameString<AnotherStruct>(true), GetTypeNameString<AnotherStruct>(false));
    PrintTestResult("const AnotherStruct&", "AnotherStruct", GetTypeNameString<const AnotherStruct&>(true), GetTypeNameString<const AnotherStruct&>(false));
    PrintTestResult(
        "volatile AnotherStruct*", "AnotherStruct", GetTypeNameString<volatile AnotherStruct*>(true), GetTypeNameString<volatile AnotherStruct*>(false)
    );

    // 다중 포인터 및 cv 한정자
    PrintTestResult(
        "MyEnum const*volatile**const*", "MyEnum", GetTypeNameString<MyEnum const*volatile**const*>(true), GetTypeNameString<MyEnum const*volatile**const*>(false)
    );
    PrintTestResult(
        "const volatile MyEnum * const * volatile *", "MyEnum", GetTypeNameString<const volatile MyEnum* const * volatile *>(true),
        GetTypeNameString<const volatile MyEnum* const * volatile *>(false)
    );


    // 사용자가 언급한 "MyEnum*****volatile" 결과가 나오는 경우를 재현하기 위한 입력 타입 추정
    // 만약 GetTypeNameString< TYPE >() 의 결과가 "MyEnum*****volatile" 이었다면,
    // TYPE 은 예를 들어 (MyEnum*****)volatile 또는 MyEnum volatile ***** 와 유사한 구조일 수 있습니다.
    // 또는 (MyEnum***** const) volatile 같은 형태.
    // 아래는 그러한 타입에 대한 테스트입니다.
    {
        using FiveStarMyEnum = MyEnum*****;
        using TestTypeUserIssue1 = FiveStarMyEnum volatile; // (MyEnum*****) volatile
        PrintTestResult("(MyEnum*****) volatile", "MyEnum", GetTypeNameString<TestTypeUserIssue1>(true), GetTypeNameString<TestTypeUserIssue1>(false));
    }
    {
        using TestTypeUserIssue2 = MyEnum volatile*****;
        PrintTestResult("MyEnum volatile *****", "MyEnum", GetTypeNameString<TestTypeUserIssue2>(true), GetTypeNameString<TestTypeUserIssue2>(false));
    }
    {
        // 사용자의 예시: volatile const volatile const MyEnum ***** const volatile
        // 이것은 위에서 TestType2로 이미 테스트되었고, 기대값은 "MyEnum" 입니다.
        // 만약 이것이 "MyEnum*****volatile"로 나온다면, GetTypeNameString 내부 로직 (특히 MSVC 파서)에 문제가 있는 것입니다.
        // 이 테스트는 해당 케이스가 "MyEnum"을 반환하는지 재확인합니다.
        using TypeFromUserExample = volatile const volatile const MyEnum***** const volatile;
        PrintTestResult(
            "FROM USER: volatile const volatile const MyEnum ***** const volatile", "MyEnum", GetTypeNameString<TypeFromUserExample>(true),
            GetTypeNameString<TypeFromUserExample>(false)
        );
    }


    // 배열 타입 (현재 로직은 배열의 '[]'를 제거하지 않음)
    // MSVC에서 `const char[N]`은 `char const [N]`으로 나올 수 있음.
    PrintTestResult("char[10]", "char[10]", GetTypeNameString<char[10]>(true), GetTypeNameString<char[10]>(false));
    // 현재 로직으로는 'const char[N]' 또는 'char const [N]'에서 'const'만 제거하기는 어려움.
    // MSVC `__FUNCSIG__`는 "char const [5]" 형태를 띌 수 있습니다.
    // 현재 IsAlnumChar 기반 제거 로직은 "const"가 다른 식별자와 붙어있지 않은 경우에만 작동합니다.
    // "char const [5]"의 "const"는 "char " 뒤, " [5]" 앞에 있어, "const " 또는 " const" 패턴으로 제거될 수 있지만,
    // 공백 없는 "const" 제거 로직은 앞뒤가 IsAlnumChar가 아닐 때 작동하므로, "char const [5]"의 "const"를 제거할 수도 있습니다.
    // 만약 "char const [5]" -> "char [5]" 로 만든다면, 현재 코드는 그렇게 동작할 가능성이 있습니다.
    PrintTestResult("const char[10]", "char[10]", GetTypeNameString<const char[10]>(true), GetTypeNameString<const char[10]>(false));


    // 함수 포인터 (단순화가 어려움, bIsNameOnly=false의 출력이 더 유용)
    using FunctionPtrType = void(*)(int, double);
    std::cout << "\nTest: void(*)(int, double)" << std::endl;
    std::cout << "  bIsNameOnly = true: Actual: \"" << GetTypeNameString<FunctionPtrType>(true) << "\"" << std::endl;
    std::cout << "  bIsNameOnly = false: Actual: \"" << GetTypeNameString<FunctionPtrType>(false) << "\"" << std::endl;


    std::cout << "\n--- All Tests Completed ---" << std::endl;
}
#endif
#pragma endregion
