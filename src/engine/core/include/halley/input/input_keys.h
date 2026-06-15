/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "halley/text/string_converter.h"

namespace Halley {
	enum class KeyCode : uint16_t {
		Unknown = 0,


	    A = 4,
	    B = 5,
	    C = 6,
	    D = 7,
	    E = 8,
	    F = 9,
	    G = 10,
	    H = 11,
	    I = 12,
	    J = 13,
	    K = 14,
	    L = 15,
	    M = 16,
	    N = 17,
	    O = 18,
	    P = 19,
	    Q = 20,
	    R = 21,
	    S = 22,
	    T = 23,
	    U = 24,
	    V = 25,
	    W = 26,
	    X = 27,
	    Y = 28,
	    Z = 29,
	    Num1 = 30,
	    Num2 = 31,
	    Num3 = 32,
	    Num4 = 33,
	    Num5 = 34,
	    Num6 = 35,
	    Num7 = 36,
	    Num8 = 37,
	    Num9 = 38,
	    Num0 = 39,
	    Enter = 40,
	    Esc = 41,
	    Backspace = 42,
	    Tab = 43,
	    Space = 44,
	    Minus = 45,
	    Equals = 46,
	    LeftBracket = 47,
	    RightBracket = 48,
	    Backslash = 49, // Around enter, above for US, left for ISO
	    NonUSHash = 50, // Not used
	    Semicolon = 51,
	    Apostrophe = 52,
	    Grave = 53, // Left of 1
	    Comma = 54,
	    Period = 55,
	    Slash = 56,
	    CapsLock = 57,
	    F1 = 58,
	    F2 = 59,
	    F3 = 60,
	    F4 = 61,
	    F5 = 62,
	    F6 = 63,
	    F7 = 64,
	    F8 = 65,
	    F9 = 66,
	    F10 = 67,
	    F11 = 68,
	    F12 = 69,
	    PrintScreen = 70,
	    ScrollLock = 71,
	    Pause = 72,
	    Insert = 73, // Insert on PC, Help on Mac
	    Home = 74,
	    PageUp = 75,
	    Delete = 76,
	    End = 77,
	    PageDown = 78,
	    Right = 79,
	    Left = 80,
	    Down = 81,
	    Up = 82,
	    NumLock = 83, // Num Lock/Clear
	    KeypadDivide = 84,
	    KeypadMultiply = 85,
	    KeypadMinus = 86,
	    KeypadPlus = 87,
	    KeypadEnter = 88,
	    Keypad1 = 89,
	    Keypad2 = 90,
	    Keypad3 = 91,
	    Keypad4 = 92,
	    Keypad5 = 93,
	    Keypad6 = 94,
	    Keypad7 = 95,
	    Keypad8 = 96,
	    Keypad9 = 97,
	    Keypad0 = 98,
	    KeypadPeriod = 99,
	    NonUSBackslash = 100, // Key next to LShift
	    Application = 101, // Windows context menu
	    Power = 102,
	    KeypadEquals = 103,
	    F13 = 104,
	    F14 = 105,
	    F15 = 106,
	    F16 = 107,
	    F17 = 108,
	    F18 = 109,
	    F19 = 110,
	    F20 = 111,
	    F21 = 112,
	    F22 = 113,
	    F23 = 114,
	    F24 = 115,
	    Execute = 116,
	    Help = 117,
	    Menu = 118,
	    Select = 119,
	    Stop = 120,
	    Again = 121,
	    Undo = 122,
	    Cut = 123,
	    Copy = 124,
	    Paste = 125,
	    Find = 126,
	    Mute = 127,
	    VolumeUp = 128,
	    VolumeDown = 129,
	    KeypadComma = 133,
	    KeypadEqualsAS400 = 134,
	    International1 = 135, // Used in Asian keyboards
	    International2 = 136,
	    International3 = 137, // Yen
	    International4 = 138,
	    International5 = 139,
	    International6 = 140,
	    International7 = 141,
	    International8 = 142,
	    International9 = 143,
	    Lang1 = 144, // Hangul/English toggle
	    Lang2 = 145, // Hanja conversion
	    Lang3 = 146, // Katakana
	    Lang4 = 147, // Hiragana
	    Lang5 = 148, // Zenkaku/Hankaku
	    Lang6 = 149,
	    Lang7 = 150,
	    Lang8 = 151,
	    Lang9 = 152,
	    AltErase = 153,
	    SysReq = 154,
	    Cancel = 155,
	    Clear = 156,
	    Prior = 157,
	    Return2 = 158,
	    Separator = 159,
	    Out = 160,
	    Oper = 161,
	    ClearAgain = 162,
	    CRSEL = 163,
	    EXSEL = 164,
	    Keypad00 = 176,
	    Keypad000 = 177,
	    ThousandsSeparator = 178,
	    DecimalSeparator = 179,
	    CurrencyUnit = 180,
	    CurrencySubUnit = 181,
	    KeypadLeftParen = 182,
	    KeypadRightParen = 183,
	    KeypadLeftBrace = 184,
	    KeypadRightBrace = 185,
	    KeypadTab = 186,
	    KeypadBackspace = 187,
	    KeypadA = 188,
	    KeypadB = 189,
	    KeypadC = 190,
	    KeypadD = 191,
	    KeypadE = 192,
	    KeypadF = 193,
	    KeypadXOR = 194,
	    KeypadPower = 195,
	    KeypadPercent = 196,
	    KeypadLess = 197,
	    KeypadGreater = 198,
	    KeypadAmpersand = 199,
	    KeypadDoubleAmpersand = 200,
	    KeypadVerticalBar = 201,
	    KeypadDoubleVerticalBar = 202,
	    KeypadColon = 203,
	    KeypadHash = 204,
	    KeypadSpace = 205,
	    KeypadAt = 206,
	    KeypadExclamation = 207,
	    KeypadMemStore = 208,
	    KeypadMemRecall = 209,
	    KeypadMemClear = 210,
	    KeypadMemAdd = 211,
	    KeypadMemSubtract = 212,
	    KeypadMemMultiply = 213,
	    KeypadMemDivide = 214,
	    KeypadPlusMinus = 215,
	    KeypadClear = 216,
	    KeypadClearEntry = 217,
	    KeypadBinary = 218,
	    KeypadOctal = 219,
	    KeypadDecimal = 220,
	    KeypadHexadecimal = 221,
	    LCtrl = 224,
	    LShift = 225,
	    LAlt = 226, // Option on mac
	    LGUI = 227, // Windows/Command/Meta
	    RCtrl = 228,
	    RShift = 229,
	    RAlt = 230,
	    RGUI = 231, // Windows/Command/Meta
		AndroidMenu = 246, // Is this used?
	    Mode = 257,
	    AudioNext = 258,
	    AudioPrev = 259,
	    AudioStop = 260,
	    AudioPlay = 261,
	    AudioMute = 262,
	    MediaSelect = 263,
	    WWW = 264,
	    Mail = 265,
	    Calculator = 266,
	    Computer = 267,
	    AC_Search = 268,
	    AC_Home = 269,
	    AC_Back = 270,
	    AC_Foward = 271,
	    AC_Stop = 272,
	    AC_Refresh = 273,
	    AC_Boomarks = 274,
	    BrightnessDown = 275,
	    BrightnessUp = 276,
	    DisplaySwitch = 277,
	    KeyboardIlluminationToggle = 278,
	    KeyboardIlluminationDown = 279,
	    KeyboardIllumiantionUp = 280,
	    Eject = 281,
	    Sleep = 282,
	    App1 = 283,
	    App2 = 284,
	    AudioRewind = 285,
	    AudioFastForward = 286,
		AndroidBack = 398, // Is this used?
	    Last = 512
	};

	enum class KeyMods : uint8_t {
		None = 0,
		Shift = 1,
		Ctrl = 2,
		CtrlShift = Ctrl | Shift,
		Alt = 4,
		ShiftAlt = Shift | Alt,
		CtrlAlt = Ctrl | Alt,
		CtrlShiftAlt = Ctrl | Shift | Alt,
		Mod = 8,
		ShiftMod = Shift | Mod,
		CtrlMod = Ctrl | Mod,
		CtrlShiftMod = Ctrl | Shift | Mod,
		AltMod = Alt | Mod,
		ShiftAltMod = Shift | Alt | Mod,
		CtrlAltMod = Ctrl | Alt | Mod,
		CtrlShiftAltMod = Ctrl | Shift | Alt | Mod,
	};

	inline KeyMods operator&(KeyMods a, KeyMods b)
	{
		return static_cast<KeyMods>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
	}

	inline KeyMods operator|(KeyMods a, KeyMods b)
	{
		return static_cast<KeyMods>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
	}

	class KeyCodes {
	public:
		static String toString(KeyCode code, KeyMods mods = KeyMods::None);
		static String toName(KeyCode code);
		static String toName(KeyMods mods);
		static std::optional<String> tryToName(KeyCode code);
		static KeyCode fromString(const String& str);
		static std::optional<KeyCode> tryFromString(const String& str);
		static std::pair<KeyCode, KeyMods> fromStringWithMods(const String& str);
	};
	
	template<>
	struct ToStringConverter<KeyCode>
	{
		String operator()(KeyCode s) const
		{
			return KeyCodes::toString(s);
		}
	};

	template<>
	struct FromStringConverter<KeyCode>
	{
		KeyCode operator()(const String& s) const
		{
			return KeyCodes::fromString(s);
		}
	};

	template <>
	struct TryFromStringConverter<KeyCode> {
		std::optional<KeyCode> operator()(const String& s) const
		{
			return KeyCodes::tryFromString(s);
		}
	};

}
