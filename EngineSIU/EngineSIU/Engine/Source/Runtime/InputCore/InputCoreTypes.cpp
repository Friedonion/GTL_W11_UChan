#include "InputCoreTypes.h"

FInputKeyManager& FInputKeyManager::Get()
{
    static FInputKeyManager Instance;
    return Instance;
}

FInputKeyManager::FInputKeyManager()
{
    // --------------------------------------
    // Win32 가상키를 EKeys::Type으로 매핑
    // --------------------------------------

    // 특수키(제어키) 매핑
    KeyMapVirtualToEnum.Add(VK_TAB, EKeys::Tab);
    KeyMapVirtualToEnum.Add(VK_RETURN, EKeys::Enter);
    KeyMapVirtualToEnum.Add(VK_PAUSE, EKeys::Pause);
    KeyMapVirtualToEnum.Add(VK_CAPITAL, EKeys::CapsLock);
    KeyMapVirtualToEnum.Add(VK_ESCAPE, EKeys::Escape);
    KeyMapVirtualToEnum.Add(VK_SPACE, EKeys::SpaceBar);
    KeyMapVirtualToEnum.Add(VK_PRIOR, EKeys::PageUp);
    KeyMapVirtualToEnum.Add(VK_NEXT, EKeys::PageDown);
    KeyMapVirtualToEnum.Add(VK_END, EKeys::End);
    KeyMapVirtualToEnum.Add(VK_HOME, EKeys::Home);
    KeyMapVirtualToEnum.Add(VK_LEFT, EKeys::Left);
    KeyMapVirtualToEnum.Add(VK_UP, EKeys::Up);
    KeyMapVirtualToEnum.Add(VK_RIGHT, EKeys::Right);
    KeyMapVirtualToEnum.Add(VK_DOWN, EKeys::Down);
    KeyMapVirtualToEnum.Add(VK_INSERT, EKeys::Insert);
    KeyMapVirtualToEnum.Add(VK_DELETE, EKeys::Delete);
    KeyMapVirtualToEnum.Add(VK_BACK, EKeys::BackSpace);

    // 넘패드 키
    KeyMapVirtualToEnum.Add(VK_NUMPAD0, EKeys::NumPadZero);
    KeyMapVirtualToEnum.Add(VK_NUMPAD1, EKeys::NumPadOne);
    KeyMapVirtualToEnum.Add(VK_NUMPAD2, EKeys::NumPadTwo);
    KeyMapVirtualToEnum.Add(VK_NUMPAD3, EKeys::NumPadThree);
    KeyMapVirtualToEnum.Add(VK_NUMPAD4, EKeys::NumPadFour);
    KeyMapVirtualToEnum.Add(VK_NUMPAD5, EKeys::NumPadFive);
    KeyMapVirtualToEnum.Add(VK_NUMPAD6, EKeys::NumPadSix);
    KeyMapVirtualToEnum.Add(VK_NUMPAD7, EKeys::NumPadSeven);
    KeyMapVirtualToEnum.Add(VK_NUMPAD8, EKeys::NumPadEight);
    KeyMapVirtualToEnum.Add(VK_NUMPAD9, EKeys::NumPadNine);
    KeyMapVirtualToEnum.Add(VK_MULTIPLY, EKeys::Multiply);
    KeyMapVirtualToEnum.Add(VK_ADD, EKeys::Add);
    KeyMapVirtualToEnum.Add(VK_SUBTRACT, EKeys::Subtract);
    KeyMapVirtualToEnum.Add(VK_DECIMAL, EKeys::Decimal);
    KeyMapVirtualToEnum.Add(VK_DIVIDE, EKeys::Divide);

    // F1-F12 키
    KeyMapVirtualToEnum.Add(VK_F1, EKeys::F1);
    KeyMapVirtualToEnum.Add(VK_F2, EKeys::F2);
    KeyMapVirtualToEnum.Add(VK_F3, EKeys::F3);
    KeyMapVirtualToEnum.Add(VK_F4, EKeys::F4);
    KeyMapVirtualToEnum.Add(VK_F5, EKeys::F5);
    KeyMapVirtualToEnum.Add(VK_F6, EKeys::F6);
    KeyMapVirtualToEnum.Add(VK_F7, EKeys::F7);
    KeyMapVirtualToEnum.Add(VK_F8, EKeys::F8);
    KeyMapVirtualToEnum.Add(VK_F9, EKeys::F9);
    KeyMapVirtualToEnum.Add(VK_F10, EKeys::F10);
    KeyMapVirtualToEnum.Add(VK_F11, EKeys::F11);
    KeyMapVirtualToEnum.Add(VK_F12, EKeys::F12);

    // 기타 제어 키
    KeyMapVirtualToEnum.Add(VK_NUMLOCK, EKeys::NumLock);
    KeyMapVirtualToEnum.Add(VK_SCROLL, EKeys::ScrollLock);
    KeyMapVirtualToEnum.Add(VK_LSHIFT, EKeys::LeftShift);
    KeyMapVirtualToEnum.Add(VK_RSHIFT, EKeys::RightShift);
    KeyMapVirtualToEnum.Add(VK_LCONTROL, EKeys::LeftControl);
    KeyMapVirtualToEnum.Add(VK_RCONTROL, EKeys::RightControl);
    KeyMapVirtualToEnum.Add(VK_LMENU, EKeys::LeftAlt);
    KeyMapVirtualToEnum.Add(VK_RMENU, EKeys::RightAlt);
    KeyMapVirtualToEnum.Add(VK_LWIN, EKeys::LeftCommand);
    KeyMapVirtualToEnum.Add(VK_RWIN, EKeys::RightCommand);

    // 특수 문자
    KeyMapVirtualToEnum.Add(VK_OEM_1, EKeys::Semicolon);      // ;:
    KeyMapVirtualToEnum.Add(VK_OEM_PLUS, EKeys::Equals);      // =+
    KeyMapVirtualToEnum.Add(VK_OEM_COMMA, EKeys::Comma);      // ,<
    KeyMapVirtualToEnum.Add(VK_OEM_MINUS, EKeys::Hyphen);     // -_
    KeyMapVirtualToEnum.Add(VK_OEM_PERIOD, EKeys::Period);    // .>
    KeyMapVirtualToEnum.Add(VK_OEM_2, EKeys::Slash);          // /?
    KeyMapVirtualToEnum.Add(VK_OEM_3, EKeys::Tilde);          // `~
    KeyMapVirtualToEnum.Add(VK_OEM_4, EKeys::LeftBracket);    // [{
    KeyMapVirtualToEnum.Add(VK_OEM_5, EKeys::Backslash);      // \|
    KeyMapVirtualToEnum.Add(VK_OEM_6, EKeys::RightBracket);   // ]}
    KeyMapVirtualToEnum.Add(VK_OEM_7, EKeys::Apostrophe);     // '"

    // --------------------------------------
    // 문자 코드를 EKeys::Type으로 매핑
    // --------------------------------------

    // 숫자 문자
    KeyMapCharToEnum.Add('0', EKeys::Zero);
    KeyMapCharToEnum.Add('1', EKeys::One);
    KeyMapCharToEnum.Add('2', EKeys::Two);
    KeyMapCharToEnum.Add('3', EKeys::Three);
    KeyMapCharToEnum.Add('4', EKeys::Four);
    KeyMapCharToEnum.Add('5', EKeys::Five);
    KeyMapCharToEnum.Add('6', EKeys::Six);
    KeyMapCharToEnum.Add('7', EKeys::Seven);
    KeyMapCharToEnum.Add('8', EKeys::Eight);
    KeyMapCharToEnum.Add('9', EKeys::Nine);

    // 알파벳 대문자
    KeyMapCharToEnum.Add('A', EKeys::A);
    KeyMapCharToEnum.Add('B', EKeys::B);
    KeyMapCharToEnum.Add('C', EKeys::C);
    KeyMapCharToEnum.Add('D', EKeys::D);
    KeyMapCharToEnum.Add('E', EKeys::E);
    KeyMapCharToEnum.Add('F', EKeys::F);
    KeyMapCharToEnum.Add('G', EKeys::G);
    KeyMapCharToEnum.Add('H', EKeys::H);
    KeyMapCharToEnum.Add('I', EKeys::I);
    KeyMapCharToEnum.Add('J', EKeys::J);
    KeyMapCharToEnum.Add('K', EKeys::K);
    KeyMapCharToEnum.Add('L', EKeys::L);
    KeyMapCharToEnum.Add('M', EKeys::M);
    KeyMapCharToEnum.Add('N', EKeys::N);
    KeyMapCharToEnum.Add('O', EKeys::O);
    KeyMapCharToEnum.Add('P', EKeys::P);
    KeyMapCharToEnum.Add('Q', EKeys::Q);
    KeyMapCharToEnum.Add('R', EKeys::R);
    KeyMapCharToEnum.Add('S', EKeys::S);
    KeyMapCharToEnum.Add('T', EKeys::T);
    KeyMapCharToEnum.Add('U', EKeys::U);
    KeyMapCharToEnum.Add('V', EKeys::V);
    KeyMapCharToEnum.Add('W', EKeys::W);
    KeyMapCharToEnum.Add('X', EKeys::X);
    KeyMapCharToEnum.Add('Y', EKeys::Y);
    KeyMapCharToEnum.Add('Z', EKeys::Z);

    // 알파벳 소문자 (같은 키 매핑)
    KeyMapCharToEnum.Add('a', EKeys::A);
    KeyMapCharToEnum.Add('b', EKeys::B);
    KeyMapCharToEnum.Add('c', EKeys::C);
    KeyMapCharToEnum.Add('d', EKeys::D);
    KeyMapCharToEnum.Add('e', EKeys::E);
    KeyMapCharToEnum.Add('f', EKeys::F);
    KeyMapCharToEnum.Add('g', EKeys::G);
    KeyMapCharToEnum.Add('h', EKeys::H);
    KeyMapCharToEnum.Add('i', EKeys::I);
    KeyMapCharToEnum.Add('j', EKeys::J);
    KeyMapCharToEnum.Add('k', EKeys::K);
    KeyMapCharToEnum.Add('l', EKeys::L);
    KeyMapCharToEnum.Add('m', EKeys::M);
    KeyMapCharToEnum.Add('n', EKeys::N);
    KeyMapCharToEnum.Add('o', EKeys::O);
    KeyMapCharToEnum.Add('p', EKeys::P);
    KeyMapCharToEnum.Add('q', EKeys::Q);
    KeyMapCharToEnum.Add('r', EKeys::R);
    KeyMapCharToEnum.Add('s', EKeys::S);
    KeyMapCharToEnum.Add('t', EKeys::T);
    KeyMapCharToEnum.Add('u', EKeys::U);
    KeyMapCharToEnum.Add('v', EKeys::V);
    KeyMapCharToEnum.Add('w', EKeys::W);
    KeyMapCharToEnum.Add('x', EKeys::X);
    KeyMapCharToEnum.Add('y', EKeys::Y);
    KeyMapCharToEnum.Add('z', EKeys::Z);

    // 특수 문자
    KeyMapCharToEnum.Add(';', EKeys::Semicolon);
    KeyMapCharToEnum.Add(':', EKeys::Semicolon);
    KeyMapCharToEnum.Add('=', EKeys::Equals);
    KeyMapCharToEnum.Add('+', EKeys::Equals);
    KeyMapCharToEnum.Add(',', EKeys::Comma);
    KeyMapCharToEnum.Add('<', EKeys::Comma);
    KeyMapCharToEnum.Add('-', EKeys::Hyphen);
    KeyMapCharToEnum.Add('_', EKeys::Underscore);
    KeyMapCharToEnum.Add('.', EKeys::Period);
    KeyMapCharToEnum.Add('>', EKeys::Period);
    KeyMapCharToEnum.Add('/', EKeys::Slash);
    KeyMapCharToEnum.Add('?', EKeys::Slash);
    KeyMapCharToEnum.Add('`', EKeys::Tilde);
    KeyMapCharToEnum.Add('~', EKeys::Tilde);
    KeyMapCharToEnum.Add('[', EKeys::LeftBracket);
    KeyMapCharToEnum.Add('{', EKeys::LeftBracket);
    KeyMapCharToEnum.Add('\\', EKeys::Backslash);
    KeyMapCharToEnum.Add('|', EKeys::Backslash);
    KeyMapCharToEnum.Add(']', EKeys::RightBracket);
    KeyMapCharToEnum.Add('}', EKeys::RightBracket);
    KeyMapCharToEnum.Add('\'', EKeys::Apostrophe);
    KeyMapCharToEnum.Add('"', EKeys::Quote);

    // 추가 특수 문자
    KeyMapCharToEnum.Add('&', EKeys::Ampersand);
    KeyMapCharToEnum.Add('*', EKeys::Asterix);
    KeyMapCharToEnum.Add('^', EKeys::Caret);
    KeyMapCharToEnum.Add('$', EKeys::Dollar);
    KeyMapCharToEnum.Add('!', EKeys::Exclamation);
    KeyMapCharToEnum.Add('(', EKeys::LeftParentheses);
    KeyMapCharToEnum.Add(')', EKeys::RightParentheses);
}

EKeys::Type FInputKeyManager::GetKeyFromVirtual(uint32 VirtualKey) const
{
    // Win32 가상키를 EKeys::Type으로 변환
    if (const EKeys::Type* FoundKey = KeyMapVirtualToEnum.Find(VirtualKey))
    {
        return *FoundKey;
    }

    // 매핑된 키를 찾을 수 없으면 EKeys::Invalid 반환
    return EKeys::Invalid;
}

EKeys::Type FInputKeyManager::GetKeyFromChar(TCHAR CharCode) const
{
    // 문자 코드를 EKeys::Type으로 변환
    if (const EKeys::Type* FoundKey = KeyMapCharToEnum.Find(CharCode))
    {
        return *FoundKey;
    }

    // 매핑된 키를 찾을 수 없으면 EKeys::Invalid 반환
    return EKeys::Invalid;
}
