#include "base_include.h"

// char

bool32 Char8IsNewline(char8 C)
{
	return (C == '\n' || C == '\r');
}

bool32 Char8IsWhitespace(char8 C)
{
	return (C == ' ' || C == '\t' || Char8IsNewline(C));
}

bool32 Char8IsAlphabetical(char8 C)
{
	return ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z'));
}

bool32 Char8IsNumeric(char8 C)
{
	return (C >= '0' && C <= '9');
}

bool32 Char8IsHexNumeric(char8 C)
{
	if (Char8IsNumeric(C)) return true;
	C = Char8Lower(C);
	return (C >= 'a' && C <= 'f');
}

char8 Char8Lower(char8 C)
{
	if (C >= 'A' && C <= 'Z')
	{
		C -= 'A' - 'a';
	}
	return C;
}

bool32 Char8IsAlphaNumeric(char8 c)
{
	return Char8IsAlphabetical(c) || Char8IsNumeric(c);
}

char8_class Char8ClassAlpha = { .IncludeAlpha = true };
char8_class Char8ClassNumeric = { .IncludeNumeric = true };
char8_class Char8ClassAlphaNumeric = { .IncludeAlpha = true, .IncludeNumeric = true };
char8_class Char8ClassWhitespace = { .IncludeWhitespace = true };

char8_class Char8Class(str8 IncludeChars, bool32 IncludeAlpha, bool32 IncludeNumeric, bool32 IncludeWhitespace)
{
	return (char8_class) {
		.IncludeChars = IncludeChars,
		.IncludeAlpha = IncludeAlpha,
		.IncludeNumeric = IncludeNumeric,
		.IncludeWhitespace = IncludeWhitespace
	};
}

bool32 Char8Match(char8 Char, char8_class CharClass, u32 MatchFlags)
{
	if (CharClass.IncludeNumeric && Char8IsNumeric(Char))
	{
		return (true != CharClass.Invert);
	}
	else if (CharClass.IncludeAlpha && Char8IsAlphabetical(Char))
	{
		return (true != CharClass.Invert);
	}
	else if (CharClass.IncludeWhitespace && Char8IsWhitespace(Char))
	{
		return (true != CharClass.Invert);
	}
	else
	{
		for (u32 CharIndex = 0; CharIndex < CharClass.IncludeChars.Count; CharIndex++)
		{
			if (MatchFlags & MatchFlag_IgnoreCase)
			{
				if (Char8Lower(Char) == Char8Lower(CharClass.IncludeChars.Data[CharIndex]))
				{
					return (true != CharClass.Invert);
				}
			}
			if (Char == CharClass.IncludeChars.Data[CharIndex])
			{
				return (true != CharClass.Invert);
			}
		}
	}
	return false;
}

// string

str8 Str8Empty()
{
	return (str8) {0};
}

str8 Str8FromCStr(char * CString)
{
	if (CString == 0) return Str8Empty();

	u32 Count = 0;
	while (CString[Count] != 0) {
		Count++;
	}
	return (str8) { (char8 *) CString, Count };
}

str8 Str8FromBuffer(memory_buffer * Buffer)
{
	return (str8) { .Count = Buffer->Count, .Data = Buffer->Data };
}

str16 Str16Empty()
{
	return (str16) {0};
}

str16 Str16FromCStr(u16 * CString)
{
	if (CString == 0) return Str16Empty();

	u32 Count = 0;
	while (CString[Count] != 0) {
		Count++;
	}
	return (str16) { (char16 *) CString, Count };
}

str16 Str16FromBuffer(memory_buffer * Buffer)
{
	return (str16) { .Count = Buffer->Count, .Data = Buffer->Data };
}

str32 Str32Empty()
{
	return (str32) {0};
}

str32 Str32FromCStr(u32 * CString)
{
	if (CString == 0) return Str32Empty();

	u32 Count = 0;
	while (CString[Count] != 0) {
		Count++;
	}
	return (str32) { (char32 *) CString, Count };
}

str32 Str32FromBuffer(memory_buffer * Buffer)
{
	return (str32) { .Count = Buffer->Count, .Data = Buffer->Data };
}

// conversion

str8 Str8FromStr16(str16 String)
{
	return (str8) { .Data = (void *) String.Data, .Count = String.Count * 2 };
}

str8 Str8FromStr32(str32 String)
{
	return (str8) { .Data = (void *) String.Data, .Count = String.Count * 4 };
}

str16 Str16FromStr8(str8 String)
{
	Assert(String.Count % 2 == 0);
	return (str16) { .Data = (void *) String.Data, .Count = String.Count / 2 };
}

str16 Str16FromStr32(str32 String)
{
	return (str16) { .Data = (void *) String.Data, .Count = String.Count * 2 };
}

str32 Str32FromStr8(str8 String)
{
	Assert(String.Count % 4 == 0);
	return (str32) { .Data = (void *) String.Data, .Count = String.Count / 4 };
}

str32 Str32FromStr16(str16 String)
{
	Assert(String.Count % 2 == 0);
	return (str32) { .Data = (void *) String.Data, .Count = String.Count / 2 };
}

#define UNICODE_ERROR ((char32) 0x80000000)

char32 UTF8Read(str8 * String)
{
	char32 CodePoint = 0;
	u32 CodePointOffset = 0;

	u32 BytesToParse = 0, CodeBits = 0, ExpectedBits = 0;
	u8 Mask = 0;
	u32 CurrentIndex = 0;

	if ((String->Data[0] & 0b10000000) == 0)
	{
		CodePoint = String->Data[0];
		Str8ParseEat(String, 1);
		return CodePoint;
	}
	else if ((String->Data[0] & 0b11111000) == 0b11110000)
	{
		BytesToParse = 4;
		CodeBits = 3;
		Mask = 0b00000111;
		ExpectedBits = 21;
	}
	else if ((String->Data[0] & 0b11110000) == 0b11100000)
	{
		BytesToParse = 3;
		CodeBits = 4;
		Mask = 0b00001111;
		ExpectedBits = 16;
	}
	else if ((String->Data[0] & 0b11100000) == 0b11000000)
	{
		BytesToParse = 2;
		CodeBits = 5;
		Mask = 0b00011111;
		ExpectedBits = 11;
	}
	else
	{
		Str8ParseEat(String, 1);
		return UNICODE_ERROR;
	}

	for (CurrentIndex = 0; CurrentIndex < BytesToParse; CurrentIndex++)
	{
		char8 CurrentChar = String->Data[CurrentIndex];

		CodePoint |= ((char32) (CurrentChar & Mask)) << (ExpectedBits - CodePointOffset - CodeBits);
		CodePointOffset += CodeBits;
		if (CurrentIndex == 0)
		{
			CodeBits = 6;
			Mask = 0b00111111;
		}
		else if ((CurrentChar & 0b11000000) != 0b10000000)
		{
			Str8ParseEat(String, 1);
			return UNICODE_ERROR;
		}
	}

	Str8ParseEat(String, CurrentIndex--);
	return CodePoint;
}


char32 UTF16Read(str16 * String)
{
	if ((String->Data[0] & 0b1111100000000000) != 0b1101100000000000)
	{
		char32 CodePoint = String->Data[0];
		Str16ParseEat(String, 1);
		return CodePoint;
	}
	else if ((String->Data[0] & 0b0000010000000000) != 0)
	{
		Str16ParseEat(String, 1);
		return UNICODE_ERROR;
	}
	else
	{
		if ((String->Data[1] & 0b1111110000000000) != 0b1101110000000000)
		{
			Str16ParseEat(String, 1);
			return UNICODE_ERROR;
		}

		u16 Mask = 0b0000001111111111;
		char32 CodePoint = ((char32) (String->Data[0] & Mask) << 10) | (char32) (String->Data[1] & Mask);
		Str16ParseEat(String, 2);
		return CodePoint + 0x10000;
	}
}

char32 UTF32Read(str32 * String)
{
	char32 Result = String->Data[0];
	
	Str32ParseEat(String, 1);

	if (Result > 0b111111111111111111111)
	{
		return UNICODE_ERROR;
	}

	return Result;
}

void UTF8Write(memory_buffer * Buffer, char32 CodePoint)
{
	if (CodePoint == UNICODE_ERROR)
	{
		CodePoint = 0xfffd;
	}

	if (CodePoint < 0b10000000)
	{
		Str8WriteChar8(Buffer, (char8) CodePoint);
	}
	else if (CodePoint < 0b100000000000)
	{
		Str8WriteChar8(Buffer, 0b11000000 | (0b000000000011111000000 & CodePoint) >> 6);
		Str8WriteChar8(Buffer, 0b10000000 | (0b000000000000000111111 & CodePoint));
	}
	else if (CodePoint < 0b10000000000000000)
	{
		Str8WriteChar8(Buffer, 0b11100000 | (0b000001111000000000000 & CodePoint) >> 12);
		Str8WriteChar8(Buffer, 0b10000000 | (0b000000000111111000000 & CodePoint) >> 6);
		Str8WriteChar8(Buffer, 0b10000000 | (0b000000000000000111111 & CodePoint));
	}
	else if (CodePoint < 0b1000000000000000000000)
	{
		Str8WriteChar8(Buffer, 0b11110000 | (0b111000000000000000000 & CodePoint) >> 18);
		Str8WriteChar8(Buffer, 0b10000000 | (0b000111111000000000000 & CodePoint) >> 12);
		Str8WriteChar8(Buffer, 0b10000000 | (0b000000000111111000000 & CodePoint) >> 6);
		Str8WriteChar8(Buffer, 0b10000000 | (0b000000000000000111111 & CodePoint));
	}
}

void UTF16Write(memory_buffer * Buffer, char32 CodePoint)
{
	if (CodePoint == UNICODE_ERROR)
	{
		CodePoint = 0xfffd;
	}

	if (CodePoint < 0xffff)
	{
		Str16WriteChar16(Buffer, (char16) CodePoint);
	}
	else
	{
		CodePoint -= 0x10000;
		Str16WriteChar16(Buffer, 0b1101100000000000 | (0b11111111110000000000 & CodePoint) >> 10);
		Str16WriteChar16(Buffer, 0b1101110000000000 | (0b1111111111 & CodePoint));
	}
}

#undef UNICODE_ERROR

void Utf8WriteUtf16(memory_buffer * Buffer, str16 String)
{
	while (String.Count)
	{
		char32 CodePoint = UTF16Read(&String);
		UTF8Write(Buffer, CodePoint);
	}
}

void Utf8WriteUtf32(memory_buffer * Buffer, str32 String)
{
	while (String.Count)
	{
		char32 CodePoint = UTF32Read(&String);
		UTF8Write(Buffer, CodePoint);
	}
}

void Utf16WriteUtf8(memory_buffer * Buffer, str8 String)
{
	while (String.Count)
	{
		char32 CodePoint = UTF8Read(&String);
		UTF16Write(Buffer, CodePoint);
	}
}

void Utf16WriteUtf32(memory_buffer * Buffer, str32 String)
{
	while (String.Count)
	{
		char32 CodePoint = UTF32Read(&String);
		UTF16Write(Buffer, CodePoint);
	}
}

void Utf32WriteUtf8(memory_buffer * Buffer, str8 String)
{
	while (String.Count)
	{
		char32 CodePoint = UTF8Read(&String);
		Str32WriteChar32(Buffer, CodePoint);
	}
}

void Utf32WriteUtf16(memory_buffer * Buffer, str16 String)
{
	while (String.Count)
	{
		char32 CodePoint = UTF16Read(&String);
		Str32WriteChar32(Buffer, CodePoint);
	}
}

str8 Utf8FromUtf16(memory_arena * Arena, str16 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf8WriteUtf16(Buffer, String);
	return ScratchBufferEndStr8(Buffer, Arena);
}

str8 Utf8FromUtf32(memory_arena * Arena, str32 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf8WriteUtf32(Buffer, String);
	return ScratchBufferEndStr8(Buffer, Arena);
}

str16 Utf16FromUtf8(memory_arena * Arena, str8 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf16WriteUtf8(Buffer, String);
	return ScratchBufferEndStr16(Buffer, Arena);
}

str16 Utf16FromUtf32(memory_arena * Arena, str32 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf16WriteUtf32(Buffer, String);
	return ScratchBufferEndStr16(Buffer, Arena);
}

str32 Utf32FromUtf8(memory_arena * Arena, str8 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf32WriteUtf8(Buffer, String);
	return ScratchBufferEndStr32(Buffer, Arena);
}

str32 Utf32FromUtf16(memory_arena * Arena, str16 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf32WriteUtf16(Buffer, String);
	return ScratchBufferEndStr32(Buffer, Arena);
}

str8 Str8NullTerminate(memory_arena * Arena, str8 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Str8WriteStr8(Buffer, String);
	Str8WriteChar8(Buffer, 0);
	return ScratchBufferEndStr8(Buffer, Arena);
}

str8 Utf8FromUtf16NullTerminate(memory_arena * Arena, str16 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf8WriteUtf16(Buffer, String);
	Str8WriteChar8(Buffer, 0);
	return ScratchBufferEndStr8(Buffer, Arena);
}

str8 Utf8FromUtf32NullTerminate(memory_arena * Arena, str32 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf8WriteUtf32(Buffer, String);
	Str8WriteChar8(Buffer, 0);
	return ScratchBufferEndStr8(Buffer, Arena);
}

str16 Str16NullTerminate(memory_arena * Arena, str16 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Str16WriteStr16(Buffer, String);
	Str16WriteChar16(Buffer, 0);
	return ScratchBufferEndStr16(Buffer, Arena);
}

str16 Utf16FromUtf8NullTerminate(memory_arena * Arena, str8 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf16WriteUtf8(Buffer, String);
	Str16WriteChar16(Buffer, 0);
	return ScratchBufferEndStr16(Buffer, Arena);
}

str16 Utf16FromUtf32NullTerminate(memory_arena * Arena, str32 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf16WriteUtf32(Buffer, String);
	Str16WriteChar16(Buffer, 0);
	return ScratchBufferEndStr16(Buffer, Arena);
}

str32 Str32NullTerminate(memory_arena * Arena, str32 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Str32WriteStr32(Buffer, String);
	Str32WriteChar32(Buffer, 0);
	return ScratchBufferEndStr32(Buffer, Arena);
}

str32 Utf32FromUtf8NullTerminate(memory_arena * Arena, str8 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf32WriteUtf8(Buffer, String);
	Str32WriteChar32(Buffer, 0);
	return ScratchBufferEndStr32(Buffer, Arena);
}

str32 Utf32FromUtf16NullTerminate(memory_arena * Arena, str16 String)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Utf32WriteUtf16(Buffer, String);
	Str32WriteChar32(Buffer, 0);
	return ScratchBufferEndStr32(Buffer, Arena);
}

// string cutting, matching

str8_split Str8CutCount(str8 String, u32 Count)
{
	str8 CutPiece = Str8ParseEat(&String, Count);
	return (str8_split) { .First = CutPiece, .Second = String };
}

str8_split Str8CutFind(str8 String, str8 Match)
{
	i32 CharIndex = Str8Find(String, Match, MatchFlag_Normal);

	if (CharIndex != -1)
	{
		str8_split FirstHalf = Str8CutCount(String, CharIndex);
		str8_split SecondHalf = Str8CutCount(FirstHalf.Second, Match.Count);

		return (str8_split) { .First = FirstHalf.First, .Second = SecondHalf.Second };
	}

	return (str8_split) { .First = String, .Second = Str8Empty() };
}

str8_split Str8CutFindFromEnd(str8 String, str8 Match)
{
	i32 CharIndex = Str8FindFromEnd(String, Match, MatchFlag_Normal);

	if (CharIndex != -1)
	{
		str8_split FirstHalf = Str8CutCount(String, CharIndex);
		str8_split SecondHalf = Str8CutCount(FirstHalf.Second, Match.Count);

		return (str8_split) { .First = FirstHalf.First, .Second = SecondHalf.Second };
	}

	return (str8_split) { .First = String, .Second = Str8Empty() };
}

i32 Str8MatchAny(str8 String, str8 * Matches, u32 MatchesCount, match_flags MatchFlags)
{
	for (u32 MatchIndex = 0; MatchIndex < MatchesCount; MatchIndex++)
	{
		if (Matches[MatchIndex].Count <= 0) continue;

		if (Str8Match(String, Matches[MatchIndex], MatchFlags))
		{
			return MatchIndex;
		}
	}

	return -1;
}

bool32 Str8Match(str8 String, str8 Match, match_flags MatchFlags)
{
	if (String.Count != Match.Count) return false;

	for (u32 CharIndex = 0; CharIndex < Match.Count; CharIndex++)
	{
		char8 StringChar = String.Data[CharIndex];
		char8 MatchChar = Match.Data[CharIndex];

		if (MatchFlags & MatchFlag_IgnoreCase)
		{
			StringChar = Char8Lower(StringChar);
			MatchChar = Char8Lower(MatchChar);
		}

		if (StringChar != MatchChar)
		{
			return false;
		}
	}
	return true;
}

bool32 Str8MatchPrefix(str8 String, str8 Match, match_flags MatchFlags)
{
	str8 MatchCandidate = Str8Substr(String, 0, Match.Count);
	return Str8Match(MatchCandidate, Match, MatchFlags);
}


i32 Str8Find(str8 String, str8 Match, match_flags MatchFlags)
{
	if (Match.Count > String.Count) return -1;

	i32 CurrentMatchStart = -1;
	i32 CurrentMatchIndex = 0;

	// loop though string
	for (u32 StringIndex = 0; StringIndex < String.Count; StringIndex++) {
		if (String.Data[StringIndex] == Match.Data[CurrentMatchIndex]) {
			CurrentMatchIndex++; // if we match, move match index forward

			if (CurrentMatchStart == -1) {
				CurrentMatchStart = StringIndex;
			}

			if (CurrentMatchIndex == Match.Count) break;
		} else {
			CurrentMatchStart = -1;
			CurrentMatchIndex = 0;
		}
	}
	if (CurrentMatchIndex != Match.Count) return -1;
	return CurrentMatchStart;
}

i32 Str8FindFromEnd(str8 String, str8 Match, match_flags MatchFlags)
{
	if (Match.Count > String.Count) return -1;

	i32 CurrentMatchStart = -1;
	i32 CurrentMatchIndex = Match.Count - 1;

	// loop though string
	for (u32 StringIndex = String.Count - Match.Count; StringIndex >= 0; StringIndex--)
	{
		if (String.Data[StringIndex] == Match.Data[CurrentMatchIndex])
		{
			CurrentMatchIndex--; // if we match, move match index forward

			if (CurrentMatchStart == -1) {
				CurrentMatchStart = StringIndex;
			}

			if (CurrentMatchIndex == 0) break;
		}
		else {
			CurrentMatchStart = -1;
			CurrentMatchIndex = Match.Count - 1;
		}
	}
	if (CurrentMatchIndex != Match.Count) return -1;
	return CurrentMatchStart;
}

// sub string

str8 Str8Substr(str8 String, u32 Start, u32 Count)
{
	if (Start > String.Count) Start = String.Count;
	if (Count > String.Count - Start) Count = String.Count - Start;
	return (str8) { .Data = String.Data + Start, .Count = Count };
}

str8 Str8SubstrExtend(str8 Substr, str8 String, u32 Extend)
{
	Substr.Count += Extend;
	i32 SubstrOffset = (i32) (Substr.Data - String.Data);
	if (Substr.Count > String.Count - SubstrOffset)
	{
		Substr.Count = (u32) (String.Count - SubstrOffset);
	}
	return Substr;
}

str8 Str8SubstrFromFinds(str8 String, str8 Start, str8 End, bool32 RequireEnd)
{
	i32 StartIndex = Str8Find(String, Start, MatchFlag_Normal);
	if (StartIndex == -1)
	{
		return Str8Empty();
	}
	StartIndex += Start.Count;

	Str8ParseEat(&String, StartIndex);

	i32 EndIndex = Str8Find(String, End, MatchFlag_Normal);
	if (EndIndex == -1 && RequireEnd)
	{
		return Str8Empty();
	}
	else if (EndIndex == -1 && !RequireEnd)
	{
		EndIndex = String.Count;
	}

	return Str8Substr(String, 0, EndIndex);
}

// parse

str8 Str8ParseEat(str8 * String, u32 Count)
{
	u32 TrueCount = Min(String->Count, Count);

	str8 Result = (str8) { .Data = String->Data, .Count = TrueCount};

	String->Data += TrueCount;
	String->Count -= TrueCount;

	return Result;
}

str16 Str16ParseEat(str16 * String, u32 Count)
{
	u32 TrueCount = Min(String->Count, Count);

	str16 Result = (str16) { .Data = String->Data, .Count = TrueCount};

	String->Data += TrueCount;
	String->Count -= TrueCount;

	return Result;
}

str32 Str32ParseEat(str32 * String, u32 Count)
{
	u32 TrueCount = Min(String->Count, Count);

	str32 Result = (str32) { .Data = String->Data, .Count = TrueCount};

	String->Data += TrueCount;
	String->Count -= TrueCount;

	return Result;
}

str8 Str8ParseEatOneCharOrMulticharNL(str8 * String)
{
	u32 Count = 1;
	if ((String->Data[0] == '\r' && String->Data[1] == '\n') ||
		(String->Data[0] == '\n' && String->Data[1] == '\r'))
	{
		Count++;
	}
	return Str8ParseEat(String, Count);
}

i32 Str8ParseExpectAny(str8 * String, str8 * Matches, u32 MatchesCount, match_flags MatchFlags)
{
	for (u32 MatchIndex = 0; MatchIndex < MatchesCount; MatchIndex++)
	{
		if (Str8ParseExpect(String, Matches[MatchIndex], MatchFlags))
		{
			return MatchIndex;
		}
	}

	return -1;
}

bool32 Str8ParseExpect(str8 * String, str8 Match, match_flags MatchFlags)
{
	str8 MatchCandidate = Str8Substr(*String, 0, Match.Count);

	if (Str8Match(MatchCandidate, Match, MatchFlags))
	{
		Str8ParseEat(String, Match.Count);
		return true;
	}

	return false;
}

// todo: think about how this should behave when match is not present
str8 Str8ParseEatUntilCharMatch(str8 * String, char8_class CharClass)
{
	u32 Index = 0;

	while (Index < String->Count && !Char8Match(String->Data[Index], CharClass, MatchFlag_Normal))
	{
		Index++;
	}

	str8 Result = (str8) { .Data = String->Data, .Count = Index };
	Str8ParseEat(String, Index);

	return Result;
}

// todo: think about how this should behave when match is not present
str8 Str8ParseEatUntilChar(str8 * String, char8 Char)
{
	u32 Index = 0;

	while (Index < String->Count && !(String->Data[Index] == Char))
	{
		Index++;
	}

	str8 Result = (str8) { .Data = String->Data, .Count = Index };
	Str8ParseEat(String, Index);

	return Result;
}

// todo: think about how this should behave the match is not present
str8_bool32 Str8ParseEatUntilStr8Match(str8 * String, str8 Match)
{
	if (Str8Find(*String, Match, MatchFlag_Normal) == -1)
	{
		return (str8_bool32) { .String = Str8Empty(), .Bool = false };
	}

	str8_split Split = Str8CutFind(*String, Match);

	*String = Split.Second;
	return (str8_bool32) { .String = Split.First, .Bool = true };
}

str8 Str8ParseEatWhileCharMatch(str8 * String, char8_class CharClass)
{
	u32 Index = 0;
	while (Index < String->Count && Char8Match(String->Data[Index], CharClass, MatchFlag_Normal))
	{
		Index++;
	}

	str8 Result = (str8) { .Data = String->Data, .Count = Index };
	Str8ParseEat(String, Index);

	return Result;
}

str8 Str8ParseEatWhitespace(str8 * String)
{
	return Str8ParseEatWhileCharMatch(String, Char8ClassWhitespace);
}

// non-generic parsing helpers
// these won't work well if they encounter even slightly weird characters!

str8 Str8ParseNextTokenCore(str8 * String, bool8 WithWhitespace)
{
	char8_class Char8ClassStr8ParseTokenOperators = (char8_class) { .IncludeChars = Str8Lit("+-=&~.:;?<>,[]{}()|!@#$%^&*") };
	
	str8 Whitespace = Str8ParseEatWhitespace(String);

	if (WithWhitespace && Whitespace.Count != 0)
	{
		return Whitespace;
	}

	if (String->Count == 0)
	{
		return Str8Empty();
	}

	bool8 IsOnIdentifier = Char8IsAlphaNumeric(String->Data[0]);
	bool8 IsOnOperator = Char8Match(String->Data[0], Char8ClassStr8ParseTokenOperators, MatchFlag_Normal);
	if (IsOnIdentifier)
	{
		return Str8ParseEatWhileCharMatch(String, Char8ClassAlphaNumeric);
	}
	else if (IsOnOperator)
	{
		return Str8ParseEatWhileCharMatch(String, Char8ClassStr8ParseTokenOperators);
	}
	else
	{
		return Str8ParseEatOneCharOrMulticharNL(String);
	}
}

str8 Str8ParseNextToken(str8 * String)
{
	return Str8ParseNextTokenCore(String, false);
}

str8 Str8ParseNextTokenWithWhitespace(str8 * String)
{
	return Str8ParseNextTokenCore(String, true);
}

// parse numbers

i32 IntFromStr8(str8 String, str8 * OutRemainder)
{
	i32 Result = 0;
	bool32 Negative = false;

	Negative = (String.Data[0] == '-');

	if (OutRemainder) *OutRemainder = Str8Empty();

	for (u32 Place = Negative ? 1 : 0; Place < String.Count; Place++)
	{
		char8 Digit = String.Data[Place];
		if (!Char8IsNumeric(Digit))
		{
			if (OutRemainder) *OutRemainder = Str8Substr(String, Place, String.Count);
			return Result;
		}
		Result *= 10;
		Result += Digit - '0';
	}

	return Result * (Negative ? -1 : 1);
}

i32 IntFromHexStr8(str8 String, str8 * OutRemainder)
{
	i32 Result = 0;
	bool32 Negative = false;

	Negative = (String.Data[0] == '-');

	if (OutRemainder) *OutRemainder = Str8Empty();

	for (u32 Place = Negative ? 1 : 0; Place < String.Count; Place++)
	{
		char8 Digit = String.Data[Place];
		if (!Char8IsHexNumeric(Digit))
		{
			if (OutRemainder) *OutRemainder = Str8Substr(String, Place, String.Count);
			return Result;
		}
		Result <<= 4;
		if (Char8IsNumeric(Digit)) {
			Result += Digit - '0';
		} else {
			Result += (Char8Lower(Digit) - 'a') + 10;
		}
	}

	return Result * (Negative ? -1 : 1);
}

bool32 BoolFromStr8(str8 String)
{
	if (Str8Match(String, Str8Lit("true"), MatchFlag_Normal))
	{
		return true;
	}
	else if (Str8Match(String, Str8Lit("false"), MatchFlag_Normal))
	{
		return false;
	}

	// some sort of error logging?

	return false;
}

f32 FloatFromStr8(str8 String, str8 * OutRemainder)
{
	str8 Remainder;
	f32 FirstPart = (f32) IntFromStr8(String, &Remainder);

	if (Remainder.Count && Remainder.Data[0] == '.')
	{
		Str8ParseEat(&Remainder, 1);
		if (Remainder.Count > 4) Remainder.Count = 4;

		u32 DecimalLength = Remainder.Count;
		f32 SecondPart = (f32) IntFromStr8(Remainder, &Remainder);
		DecimalLength -= Remainder.Count;

		FirstPart += (SecondPart / DecPowers[DecimalLength]);
	}

	if (OutRemainder) *OutRemainder = Remainder;
	return FirstPart;
}

// arena stuff

str8 ArenaPushStr8(memory_arena * Arena, str8 String)
{
	return (str8) { .Data = ArenaPushArrayAndCopy(Arena, char8, String.Count, String.Data), .Count = String.Count };
}

str8 * ArenaPushStr8AndStruct(memory_arena * Arena, str8 String)
{
	str8 Result = ArenaPushStr8(Arena, String);
	return (str8 *) ArenaPushAndCopy(Arena, str8, &Result);
}

str16 ArenaPushStr16(memory_arena * Arena, str16 String)
{
	return (str16) { .Data = ArenaPushArrayAndCopy(Arena, char16, String.Count, String.Data), .Count = String.Count };
}

str16 * ArenaPushStr16AndStruct(memory_arena * Arena, str16 String)
{
	str16 Result = ArenaPushStr16(Arena, String);
	return (str16 *) ArenaPushAndCopy(Arena, str16, &Result);
}

str32 ArenaPushStr32(memory_arena * Arena, str32 String)
{
	return (str32) { .Data = ArenaPushArrayAndCopy(Arena, char32, String.Count, String.Data), .Count = String.Count };
}

str32 * ArenaPushStr32AndStruct(memory_arena * Arena, str32 String)
{
	str32 Result = ArenaPushStr32(Arena, String);
	return (str32 *) ArenaPushAndCopy(Arena, str32, &Result);
}

// string building

str8 ScratchBufferEndStr8(memory_buffer * ScratchBuffer, memory_arena * Arena)
{
	u32 Count = ScratchBuffer->Count;
	return (str8) { .Data = ScratchBufferEnd(ScratchBuffer, Arena, alignof(char8)), .Count = Count };
}

str16 ScratchBufferEndStr16(memory_buffer * ScratchBuffer, memory_arena * Arena)
{
	u32 ByteCount = ScratchBuffer->Count;
	Assert(ByteCount % 2 == 0);
	return (str16) { .Data = ScratchBufferEnd(ScratchBuffer, Arena, alignof(char16)), .Count = ByteCount / 2 };
}

str32 ScratchBufferEndStr32(memory_buffer * ScratchBuffer, memory_arena * Arena)
{
	u32 ByteCount = ScratchBuffer->Count;
	Assert(ByteCount % 4 == 0);
	return (str32) { .Data = ScratchBufferEnd(ScratchBuffer, Arena, alignof(char32)), .Count = ByteCount / 4 };
}

void Str8WriteChar8(memory_buffer * Buffer, char8 Char)
{
	BufferPush(Buffer, &Char, 1);
}

void Str16WriteChar16(memory_buffer * Buffer, char16 Char)
{
	BufferPush(Buffer, &Char, 2);
}

void Str32WriteChar32(memory_buffer * Buffer, char32 Char)
{
	BufferPush(Buffer, &Char, 4);
}

void Str8WriteStr8(memory_buffer * Buffer, str8 String)
{
	BufferPush(Buffer, String.Data, String.Count);
}

void Str16WriteStr16(memory_buffer* Buffer, str16 String)
{
	BufferPush(Buffer, String.Data, String.Count);
}

void Str32WriteStr32(memory_buffer* Buffer, str32 String)
{
	BufferPush(Buffer, String.Data, String.Count);
}

void Str8WriteBytes(memory_buffer * Buffer, void * Bytes, u32 Count)
{
	BufferPush(Buffer, Bytes, Count);
}

void Str8WriteCStr(memory_buffer * Buffer, char * CString)
{
	Str8WriteStr8(Buffer, Str8FromCStr(CString));
}

void Str8WriteInt(memory_buffer * Buffer, i64 Number)
{
	Str8WriteIntDigits(Buffer, Number, 0);
}

#define NumBufCount 256

void Str8WriteIntDigits(memory_buffer * Buffer, i64 Number, u32 MinDigits)
{
	if (Number == 0 && MinDigits == 0) { MinDigits = 1; }

	char8 NumBuf[NumBufCount] = { 0 };
	u8 DigitCount = 0;
	bool32 Negative = Number < 0;

	if (Negative) 
	{
		Number *= -1;
	}

	while (Number != 0 || DigitCount < MinDigits)
	{
		char8 Digit = '0' + Number % 10;
		NumBuf[NumBufCount - DigitCount - 1] = Digit;
		Number /= 10;
		DigitCount++;
	}

	if (Negative)
	{
		Str8WriteChar8(Buffer, '-');
	}
	Str8WriteStr8(Buffer, (str8) { .Data = &NumBuf[NumBufCount - DigitCount], .Count = DigitCount } );
}

void Str8WriteUInt(memory_buffer * Buffer, u64 Number)
{
	if (Number == 0) {
		Str8WriteChar8(Buffer, '0');
		return;
	}

	char8 NumBuf[NumBufCount] = { 0 };
	u8 DigitCount = 0;

	while (Number != 0)
	{
		char8 Digit = '0' + Number % 10;
		NumBuf[NumBufCount - DigitCount - 1] = Digit;
		Number /= 10;
		DigitCount++;
	}

	Str8WriteStr8(Buffer, (str8) { .Data = &NumBuf[NumBufCount - DigitCount], .Count = DigitCount } );
}

void Str8WriteUIntNoTrailingZeros(memory_buffer * Buffer, u64 Number, u32 MinDigits)
{
	if (Number == 0) return;

	char8 NumBuf[NumBufCount] = { 0 };
	u8 DigitCount = 0;
	u8 FirstNonZero = 0;

	while (Number != 0 || DigitCount < MinDigits)
	{
		char8 Digit = '0' + Number % 10;
		NumBuf[NumBufCount - DigitCount - 1] = Digit;
		Number /= 10;
		DigitCount++;
		if (Digit != '0' && FirstNonZero == 0)
		{
			FirstNonZero = DigitCount;
		}
	}

	Str8WriteStr8(Buffer, (str8) { .Data = &NumBuf[NumBufCount - DigitCount], .Count = DigitCount - FirstNonZero + 1 } );
}

void Str8WriteHex(memory_buffer * Buffer, u64 Number)
{
	char8 NumBuf[NumBufCount] = { 0 };
	u8 DigitCount = 0;

	while (Number > 0)
	{
		u8 DigitValue = Number & 0xf;
		char8 Digit;
		if (DigitValue < 10)
		{
			Digit = '0' + DigitValue;
		}
		else
		{
			Digit = DigitValue - 10 + 'a';
		}
		NumBuf[NumBufCount - DigitCount - 1] = Digit;

		Number >>= 4;
		DigitCount++;
	}

	Str8WriteStr8(Buffer, (str8) { .Data = &NumBuf[NumBufCount - DigitCount], .Count = DigitCount } );
}

void Str8WriteHexDigits(memory_buffer * Buffer, u64 Number, u8 MinDigits)
{
	char8 NumBuf[NumBufCount] = { 0 };
	u8 DigitCount = 0;

	while (Number > 0 || DigitCount < MinDigits)
	{
		u8 DigitValue = Number & 0xf;
		char8 Digit;
		if (DigitValue < 10)
		{
			Digit = '0' + DigitValue;
		}
		else
		{
			Digit = DigitValue - 10 + 'a';
		}
		NumBuf[NumBufCount - DigitCount - 1] = Digit;

		Number >>= 4;
		DigitCount++;
	}

	Str8WriteStr8(Buffer, (str8) { .Data = &NumBuf[NumBufCount - DigitCount], .Count = DigitCount } );
}


void Str8WriteBool(memory_buffer * Buffer, bool32 Bool)
{
	Str8WriteStr8(Buffer, Bool ? Str8Lit("true") : Str8Lit("false"));
}

// todo: this needs a lot of work

#define FloatPrintPrecision 4

void Str8WriteFloat(memory_buffer * Buffer, f64 Float)
{
	Str8WriteInt(Buffer, (i64) Float);
	if (Float < 0) 
	{
		Float *= -1;
	}

	f64 DecimalPart = Float - ((u64) Float);
	u64 DecimalPartNumber = (u64) (DecimalPart * DecPowers[FloatPrintPrecision]);
	bool32 IsPrinting = false;

	if (DecimalPartNumber != 0)
	{
		Str8WriteChar8(Buffer, '.');
	}
 
	Str8WriteUIntNoTrailingZeros(Buffer, DecimalPartNumber, FloatPrintPrecision);
}

#undef FloatPrintPrecision

#undef NumBufCount

void Str8WriteAndUnescapeStr8(memory_buffer * Buffer, str8 EscapedString)
{
	str8 CurrentSegment = EscapedString;
	CurrentSegment.Count = 0;

	for (u32 CharIndex = 0; CharIndex < EscapedString.Count; CharIndex++)
	{
		if (EscapedString.Data[CharIndex] == '\\')
		{
			if (CharIndex + 1 >= EscapedString.Count)
			{
				return; // if we see a \, it can't be the end of the string
			}

			Str8WriteStr8(Buffer, CurrentSegment);

			char EscapedChar;
			switch (EscapedString.Data[CharIndex + 1])
			{
				case 'n': EscapedChar = '\n'; break;
				case 'r': EscapedChar = '\r'; break;
				case 't': EscapedChar = '\t'; break;
				case '\\': EscapedChar = '\\'; break;
				case '\"': EscapedChar = '\"'; break;
				case '\'': EscapedChar = '\''; break;
				case '0': EscapedChar = '\0'; break;
				default: EscapedChar = EscapedString.Data[CharIndex + 1]; break;
			}

			Str8WriteChar8(Buffer, EscapedChar);

			CharIndex++;

			CurrentSegment.Data = EscapedString.Data + CharIndex + 1;
			CurrentSegment.Count = 0;
		}
		else
		{
			CurrentSegment.Count++;
		}
	}

	Str8WriteStr8(Buffer, CurrentSegment);
}

void Str8WriteAndEscapeStr8(memory_buffer * Buffer, str8 UnescapedString, bool32 AddQuotes)
{
	if (AddQuotes) Str8WriteChar8(Buffer, '\"');

	for (u32 I = 0; I < UnescapedString.Count; I++)
	{
		char Character = UnescapedString.Data[I];
		switch (Character)
		{
			case '\n': Str8WriteStr8(Buffer, Str8Lit("\\n")); break;
			case '\\': Str8WriteStr8(Buffer, Str8Lit("\\\\")); break;
			case '\"': Str8WriteStr8(Buffer, Str8Lit("\\\"")); break;
			case '\'': Str8WriteStr8(Buffer, Str8Lit("\\\'")); break;

			default: Str8WriteChar8(Buffer, Character); break;
		}
	}

	if (AddQuotes) Str8WriteChar8(Buffer, '\"');
}

void Str8WriteAndURLUnescapeStr8(memory_buffer * Buffer, str8 EscapedString)
{
	str8 CurrentSegment = EscapedString;
	CurrentSegment.Count = 0;

	for (u32 CharIndex = 0; CharIndex < EscapedString.Count; CharIndex++)
	{
		if (EscapedString.Data[CharIndex] == '%')
		{
			if (CharIndex + 2 >= EscapedString.Count)
			{
				return; // if we see a \, it can't be the end of the string
			}
			Str8WriteStr8(Buffer, CurrentSegment);

			str8 PercentCodeString = (str8) { .Data = &EscapedString.Data[CharIndex] + 1, .Count = 2};
			char8 EscapedChar = (char8) IntFromHexStr8(PercentCodeString, 0);

			Str8WriteChar8(Buffer, EscapedChar);

			CharIndex += 2;

			CurrentSegment.Data = EscapedString.Data + CharIndex + 1;
			CurrentSegment.Count = 0;
		}
		else if (EscapedString.Data[CharIndex] == '+')
		{
			Str8WriteStr8(Buffer, CurrentSegment);
			Str8WriteChar8(Buffer, ' ');

			CurrentSegment.Data = EscapedString.Data + CharIndex + 1;
			CurrentSegment.Count = 0;
		}
		else
		{
			CurrentSegment.Count++;
		}
	}

	Str8WriteStr8(Buffer, CurrentSegment);
}

void Str8WriteAndURLEscapeStr8(memory_buffer * Buffer, str8 UnescapedString)
{
	for (u32 I = 0; I < UnescapedString.Count; I++)
	{
		char Character = UnescapedString.Data[I];
		switch (Character)
		{
			case '\x01': case '\x02': case '\x03': case '\x04':
			case '\x05': case '\x06': case '\x07': case '\x08':
			case '\x09': case '\x0a': case '\x0b': case '\x0c':
			case '\x0d': case '\x0e': case '\x0f': 

			case '\x11': case '\x12': case '\x13': case '\x14':
			case '\x15': case '\x16': case '\x17': case '\x18':
			case '\x19': case '\x1a': case '\x1b': case '\x1c':
			case '\x1d': case '\x1e': case '\x1f': 

			case '<': case '>': case '#': case '%': case '\"':
			case ' ': case '{': case '}': case '|': case '\\':
			case '^': case '[': case ']': case '`': case ';':
			case '/': case '?': case ':': case '@': case '&':
			case '=': case '$': case ',': {
				Str8WriteChar8(Buffer, '%');
				Str8WriteHexDigits(Buffer, Character, 2);
			} break;

			default: Str8WriteChar8(Buffer, Character); break;
		}
	}
}

// 00000000
// 00100111
//   E   S    bitcount = 4
// 10000000

void Str8WriteSingleBinaryByte(memory_buffer * Buffer, u8 ByteToPrint, u8 BitCountToPrint, u8 StartBit)
{
	u8 Bit = 1 << (BitCountToPrint - 1 + StartBit);
	for (int BitIndex = 0; BitIndex < BitCountToPrint; BitIndex++)
	{
		Str8WriteChar8(Buffer, '0' + !!(Bit & ByteToPrint));
		Bit >>= 1;
	}
}

void Str8WriteBinaryBytes(memory_buffer * Buffer, void * Bytes, u32 BitCountToPrint)
{
	u8 ByteCountToPrint = BitCountToPrint / 8;
	u8 LeftoverBitCount = BitCountToPrint % 8;

	if (LeftoverBitCount)
	{
		Str8WriteSingleBinaryByte(Buffer, ((u8 *)Bytes)[ByteCountToPrint], LeftoverBitCount, 0);
	}

	for (i32 ByteIndex = ByteCountToPrint - 1; ByteIndex >= 0; ByteIndex--)
	{
		Str8WriteSingleBinaryByte(Buffer, ((u8 *)Bytes)[ByteIndex], 8, 0);
	}
}

void Str8WriteBinary(memory_buffer * Buffer, blob Data)
{
	Str8WriteBinaryBytes(Buffer, Data.Data, Data.Count * 8);
}

// base 64 encoding

/* Diagram:

	byte
	0 1 2 3 4 5 6 7

	digit
	0 1 2 3 4 5

	| BYTE 1             | BYTE 2             | BYTE 3
	0 1 2 3 4 5      6 7 0 1 2 3      4 5 6 7 0 1      2 3 4 5 6 7
	0 1 2 3 4 5      0 1 2 3 4 5      0 1 2 3 4 5      0 1 2 3 4 5
	| DIGIT 1        | DIGIT 2        | DIGIT 3        | DIGIT 4

*/

const char * Base64DigitSymbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void Str8WriteBase64Encode(memory_buffer * Buffer, blob Data, char8 Padding)
{
	// we want to go in groups of THREE
	for (u32 ByteIndex = 0; ByteIndex < Data.Count; ByteIndex += 3)
	{
		u8 Byte0, Byte1, Byte2;
		u8 Digit0, Digit1, Digit2, Digit3;

		i32 PaddingCount = 0;

		Byte0 = Data.Data[ByteIndex + 0];

		if (ByteIndex + 1 < Data.Count) { Byte1 = Data.Data[ByteIndex + 1]; }
		else { Byte1 = 0; PaddingCount = 2; }

		if (ByteIndex + 2 < Data.Count) { Byte2 = Data.Data[ByteIndex + 2]; }
		else { Byte2 = 0; PaddingCount = Max(PaddingCount, 1); }

		Digit0 = ((Byte0 & 0xfc) >> 2);
		Digit1 = ((Byte0 & 0x03) << 4) | ((Byte1 & 0xf0) >> 4); 
		Digit2 = ((Byte1 & 0x0f) << 2) | ((Byte2 & 0xc0) >> 6); 
		Digit3 = ((Byte2 & 0x3f) >> 0);

		//                   /* 01234567 */
		// Digit0 = ((Byte0 & 0b11111100) >> 2);          /*  01234567 */
		// Digit1 = ((Byte0 & 0b00000011) << 4) | ((Byte1 & 0b11110000) >> 4);
		// Digit2 = ((Byte1 & 0b00001111) << 2) | ((Byte2 & 0b11000000) >> 6);
		// Digit3 = ((Byte2 & 0b00111111) >> 0);

		Str8WriteChar8(Buffer, Base64DigitSymbols[Digit0]);
		Str8WriteChar8(Buffer, Base64DigitSymbols[Digit1]);

		if (PaddingCount == 2)
		{
			Str8WriteChar8(Buffer, Padding);
			Str8WriteChar8(Buffer, Padding);
		}
		else if (PaddingCount == 1)
		{
			Str8WriteChar8(Buffer, Base64DigitSymbols[Digit2]);
			Str8WriteChar8(Buffer, Padding);
		}
		else
		{
			Str8WriteChar8(Buffer, Base64DigitSymbols[Digit2]);
			Str8WriteChar8(Buffer, Base64DigitSymbols[Digit3]);
		}
	}
}

void Str8WriteBase64Decode(memory_buffer * Buffer, str8 String, char8 Padding)
{
	i32 Ending = 0;

	for (u32 CharIndex = 0; CharIndex < String.Count; CharIndex += 4)
	{
		byte Bytes[3] = { 0 };
		byte DigitValues[4] = { 0 };

		for (u32 DigitIndex = 0; DigitIndex < 4; DigitIndex++)
		{
			char8 Digit;
			if (CharIndex + DigitIndex < String.Count) { Digit = String.Data[CharIndex + DigitIndex]; }
			else { Digit = 0; }

			if (Digit >= 'A' && Digit <= 'Z') DigitValues[DigitIndex] = Digit - 'A';
			if (Digit >= 'a' && Digit <= 'z') DigitValues[DigitIndex] = Digit - 'a' + 26;
			if (Digit >= '0' && Digit <= '9') DigitValues[DigitIndex] = Digit - '0' + 26 * 2;
			if (Digit == '+') DigitValues[DigitIndex] = 26 * 2 + 10;
			if (Digit == '/') DigitValues[DigitIndex] = 26 * 2 + 10 + 1;
			if (Digit == Padding)
			{
				DigitValues[DigitIndex] = 0;
				Ending++;
			}
		}

		//                              /* 01234567 */                         /* 01234567 */
		// Bytes[0] = ((DigitValues[0] & 0b00111111) << 2) | ((DigitValues[1] & 0b00110000) >> 4);
		// Bytes[1] = ((DigitValues[1] & 0b00001111) << 4) | ((DigitValues[2] & 0b00111100) >> 2);
		// Bytes[2] = ((DigitValues[2] & 0b00000011) << 6) | ((DigitValues[3] & 0b00111111) << 0);

		Bytes[0] = ((DigitValues[0] & 0x3f) << 2) | ((DigitValues[1] & 0x30) >> 4);
		Bytes[1] = ((DigitValues[1] & 0x0f) << 4) | ((DigitValues[2] & 0x3c) >> 2);
		Bytes[2] = ((DigitValues[2] & 0x03) << 6) | ((DigitValues[3] & 0x3f) << 0);

		Str8WriteBytes(Buffer, Bytes, 3 - Ending);

		if (Ending) break;
	}
}

str8 Str8FromBase64Decode(memory_arena * Arena, str8 String, char8 Padding)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Str8WriteBase64Decode(Buffer, String, Padding);
	return ScratchBufferEndStr8(Buffer, Arena);
}

str8 Str8FromBase64Encode(memory_arena * Arena, blob Data, char8 Padding)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Str8WriteBase64Encode(Buffer, Data, Padding);
	return ScratchBufferEndStr8(Buffer, Arena);
}

// string linked list

str8_node * Str8NodeFromStr8(memory_arena * Arena, str8 String)
{
	str8_node Node = (str8_node) { .Value = String, .Next = 0 };
	return (str8_node *) ArenaPushAndCopy(Arena, str8_node, &Node);
}

str8ll Str8LLFromStr8(memory_arena * Arena, str8 String)
{
	str8_node * Node = Str8NodeFromStr8(Arena, String);
	return (str8ll) { .First = Node, .Last = Node, .Count = Node->Value.Count };
}

void Str8LLPush(memory_arena * Arena, str8ll * StringList, str8 String)
{
	str8_node * Node = Str8NodeFromStr8(Arena, String);
	if (!StringList->First)
	{
		StringList->First = Node;
	}
	if (StringList->Last)
	{
		StringList->Last->Next = Node;
	}
	StringList->Last = Node;
	StringList->Count += String.Count;
}

void Str8LLPushFront(memory_arena * Arena, str8ll * StringList, str8 String)
{
	str8_node * Node = Str8NodeFromStr8(Arena, String);
	if (!StringList->Last)
	{
		StringList->Last = Node;
	}
	if (StringList->First)
	{
		Node->Next = StringList->First;
	}
	StringList->First = Node;
	StringList->Count += String.Count;
}

str8ll Str8LLConcat(str8ll StringLLPrefix, str8ll StringLLSuffix)
{
	if (StringLLPrefix.Count == 0)
	{
		return StringLLSuffix;
	}
	if (StringLLSuffix.Count == 0)
	{
		return StringLLPrefix;
	}

	str8ll Result = {0};

	StringLLPrefix.Last->Next = StringLLSuffix.First;
	Result.Count = StringLLPrefix.Count + StringLLSuffix.Count;
	Result.First = StringLLPrefix.First;
	Result.Last = StringLLSuffix.Last;

	return Result;
}

void Str8WriteStr8LL(memory_buffer * Buffer, str8ll StringList)
{
	if (!StringList.First || !StringList.Last)
	{
		return;
	}

	str8_node * OnePastLast = StringList.Last->Next;
	for (
		str8_node * StringNode = StringList.First; 
		StringNode && StringNode != OnePastLast;
		StringNode = StringNode->Next
	) {
		Str8WriteStr8(Buffer, StringNode->Value);
	}
}

str8 Str8FromStr8LL(memory_arena * Arena, str8ll StringList)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Str8WriteStr8LL(Buffer, StringList);
	return ScratchBufferEndStr8(Buffer, Arena);
}