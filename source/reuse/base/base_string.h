#ifndef base_string_h
#define base_string_h

#ifdef __cplusplus
extern "C" {
#endif

// types

typedef enum match_flags {
	MatchFlag_Normal = 0,
	MatchFlag_IgnoreCase = (1 << 0),
	// MatchFlag_Fuzzy = (1 << 1) or whatever
} match_flags;

typedef u8 char8;
typedef u16 char16;
typedef u32 char32;

typedef struct str8 {
	char8 * Data;
	u32 Count;
} str8;

typedef str8 blob;
#define Blob(Variable) (blob) { .Data = (void *)&(Variable), .Count = sizeof(Variable) }

typedef struct str16 {
	char16 * Data;
	u32 Count;
} str16;

typedef struct str32 {
	char32 * Data;
	u32 Count;
} str32;

// other types

typedef union str8_split {
	str8 E[2];
	struct {
		str8 First;
		str8 Second;
	};
} str8_split;

typedef struct char8_class
{
	str8 IncludeChars;
	bool32 IncludeAlpha;
	bool32 IncludeNumeric;
	bool32 IncludeWhitespace;

	bool32 Invert;
} char8_class;

typedef struct match_info {
	bool32 Match;
	i32 MatchIndex;
	i32 CharIndex;
} match_info;

typedef struct str8_bool32 {
	str8 String;
	bool32 Bool;
} str8_bool32;

// char

bool32 Char8IsNewline(char8 C);
bool32 Char8IsWhitespace(char8 C);
bool32 Char8IsAlphabetical(char8 C);
bool32 Char8IsNumeric(char8 C);
bool32 Char8IsHexNumeric(char8 C);
char8 Char8Lower(char8 C);
bool32 Char8IsAlphaNumeric(char8 c);

extern char8_class Char8ClassAlpha;
extern char8_class Char8ClassNumeric;
extern char8_class Char8ClassAlphaNumeric;
extern char8_class Char8ClassWhitespace;

char8_class Char8Class(str8 IncludeChars, bool32 IncludeAlpha, bool32 IncludeNumeric, bool32 IncludeWhitespace);
bool32 Char8Match(char8 Char, char8_class CharClass, u32 MatchFlags);

// string creation utiliy

str8 Str8Empty();
str8 Str8FromCStr(char * CString);
str8 Str8FromBuffer(memory_buffer * Buffer);

#define Str8Lit(StringLiteral) ((str8) {(char8 *) (StringLiteral), sizeof(StringLiteral) - 1})
#define Str8LitInit(StringLiteral) {(char8 *) (StringLiteral), sizeof(StringLiteral) - 1 }

str16 Str16Empty();
str16 Str16FromCStr(u16 * CStringUTF16);
str16 Str16FromBuffer(memory_buffer * Buffer);

#define Str16Lit(StringLiteral) ((str16) {(char16 *) (StringLiteral), sizeof(StringLiteral) / 2 - 1})
#define Str16LitInit(StringLiteral) {(char16 *) (StringLiteral), sizeof(StringLiteral) / 2 - 1 }

str32 Str32Empty();
str32 Str32FromCStr(u32 * CStringUTF32);
str32 Str32FromBuffer(memory_buffer * Buffer);

#define Str32Lit(StringLiteral) ((str32) {(char32 *) (StringLiteral), sizeof(StringLiteral) / 4 - 1})
#define Str32LitInit(StringLiteral) {(char32 *) (StringLiteral), sizeof(StringLiteral) / 4 - 1 }

// conversion

str8 Str8FromStr16(str16 String);
str8 Str8FromStr32(str32 String);

str16 Str16FromStr8(str8 String);
str16 Str16FromStr32(str32 String);

str32 Str32FromStr8(str8 String);
str32 Str32FromStr16(str16 String);

void Utf8WriteUtf16(memory_buffer * Buffer, str16 String);
void Utf8WriteUtf32(memory_buffer * Buffer, str32 String);

void Utf16WriteUtf8(memory_buffer * Buffer, str8 String);
void Utf16WriteUtf32(memory_buffer * Buffer, str32 String);

void Utf32WriteUtf8(memory_buffer * Buffer, str8 String);
void Utf32WriteUtf16(memory_buffer * Buffer, str16 String);

str8 Utf8FromUtf16(memory_arena * Arena, str16 String);
str8 Utf8FromUtf32(memory_arena * Arena, str32 String);

str16 Utf16FromUtf8(memory_arena * Arena, str8 String);
str16 Utf16FromUtf32(memory_arena * Arena, str32 String);

str32 Utf32FromUtf8(memory_arena * Arena, str8 String);
str32 Utf32FromUtf16(memory_arena * Arena, str16 String);

str8 Str8NullTerminate(memory_arena * Arena, str8 String);
str8 Utf8FromUtf16NullTerminate(memory_arena * Arena, str16 String);
str8 Utf8FromUtf32NullTerminate(memory_arena * Arena, str32 String);

str16 Str16NullTerminate(memory_arena * Arena, str16 String);
str16 Utf16FromUtf8NullTerminate(memory_arena * Arena, str8 String);
str16 Utf16FromUtf32NullTerminate(memory_arena * Arena, str32 String);

str32 Str32NullTerminate(memory_arena * Arena, str32 String);
str32 Utf32FromUtf8NullTerminate(memory_arena * Arena, str8 String);
str32 Utf32FromUtf16NullTerminate(memory_arena * Arena, str16 String);

// string cutting, matching, etc.

str8_split Str8CutCount(str8 String, u32 Count);
str8_split Str8CutFind(str8 String, str8 Match);
str8_split Str8CutFindFromEnd(str8 String, str8 Match);

i32 Str8MatchAny(str8 String, str8 * Matches, u32 MatchesCount, match_flags MatchFlags);
bool32 Str8Match(str8 String, str8 Match, match_flags MatchFlags);
bool32 Str8MatchPrefix(str8 String, str8 Match, match_flags MatchFlags);
i32 Str8Find(str8 String, str8 Match, match_flags MatchFlags);
i32 Str8FindFromEnd(str8 String, str8 Match, match_flags MatchFlags);

// sub string

str8 Str8Substr(str8 String, u32 Start, u32 Count);
str8 Str8SubstrExtend(str8 Substr, str8 String, u32 Extend);

str8 Str8SubstrFromFinds(str8 String, str8 Start, str8 End, bool32 RequireEnd);

// other

str8 Str8Trim(str8 String);
str8 Str8Concat(memory_arena * Arena, str8 A, str8 B);

// parse

str8 Str8ParseEat(str8 * String, u32 Count);
str16 Str16ParseEat(str16 * String, u32 Count);
str32 Str32ParseEat(str32 * String, u32 Count);

str8 Str8ParseEatOneCharOrMulticharNL(str8 * String);
i32 Str8ParseExpectAny(str8 * String, str8 * Matches, u32 MatchesCount, match_flags MatchFlags);
bool32 Str8ParseExpect(str8 * String, str8 Match, match_flags MatchFlags);
str8 Str8ParseEatUntilChar(str8 * String, char8 Char);
str8 Str8ParseEatUntilCharMatch(str8 * String, char8_class CharClass);
str8 Str8ParseEatWhileCharMatch(str8 * String, char8_class CharClass);
str8_bool32 Str8ParseEatUntilStr8Match(str8 * String, str8 Match);
str8 Str8ParseEatWhitespace(str8 * String);

// todo: these won't work well if they encounter even slightly weird characters!
str8 Str8ParseNextToken(str8 * String);
str8 Str8ParseNextTokenWithWhitespace(str8 * String);

// parse numbers

i32 I32FromStr8(str8 String, str8 * OutRemainder);
i32 I32FromHexStr8(str8 String, str8 * OutRemainder);
u64 U64FromHexStr8(str8 String, str8 * OutRemainder);
bool32 BoolFromStr8(str8 String);
f32 F32FromStr8(str8 String, str8 * OutRemainder);

// arena stuff

str8 ArenaPushStr8(memory_arena * Arena, str8 String);
str8 * ArenaPushStr8AndStruct(memory_arena * Arena, str8 String);

str16 ArenaPushStr16(memory_arena * Arena, str16 String);
str16 * ArenaPushStr16AndStruct(memory_arena * Arena, str16 String);

str32 ArenaPushStr32(memory_arena * Arena, str32 String);
str32 * ArenaPushStr32AndStruct(memory_arena * Arena, str32 String);

// string building

str8 ScratchBufferEndStr8(memory_buffer * ScratchBuffer, memory_arena * Arena);
str16 ScratchBufferEndStr16(memory_buffer * ScratchBuffer, memory_arena * Arena);
str32 ScratchBufferEndStr32(memory_buffer * ScratchBuffer, memory_arena * Arena);

void Str8WriteChar8(memory_buffer * Buffer, char8 Char);
void Str16WriteChar16(memory_buffer * Buffer, char16 Char);
void Str32WriteChar32(memory_buffer * Buffer, char32 Char);

void Str8WriteStr8(memory_buffer * Buffer, str8 String);
void Str16WriteStr16(memory_buffer * Buffer, str16 String);
void Str32WriteStr32(memory_buffer * Buffer, str32 String);

void Str8WriteBytes(memory_buffer * Buffer, void * Bytes, u32 Count);
void Str8WriteCStr(memory_buffer * Buffer, char * CString);
void Str8WriteInt(memory_buffer * Buffer, i64 Number);
void Str8WriteIntDigits(memory_buffer * Buffer, i64 Number, u32 Digits);
void Str8WriteUInt(memory_buffer * Buffer, u64 Number);
void Str8WriteHex(memory_buffer * Buffer, u64 Number);
void Str8WriteHexDigits(memory_buffer * Buffer, u64 Number, u8 MinDigits);
void Str8WriteBool(memory_buffer * Buffer, bool32 Bool);
void Str8WriteFloat(memory_buffer * Buffer, f64 Float);

void Str8WriteAndUnescapeStr8(memory_buffer * Buffer, str8 EscapedString);
void Str8WriteAndEscapeStr8(memory_buffer * Buffer, str8 UnescapedString, bool32 AddQuotes);
void Str8WriteAndURLUnescapeStr8(memory_buffer * Buffer, str8 EscapedString);
void Str8WriteAndURLEscapeStr8(memory_buffer * Buffer, str8 UnescapedString);

// void Str8WriteSingleBinaryByte(memory_buffer * Buffer, byte ByteToPrint, u8 BitCountToPrint, u8 StartBit);
void Str8WriteBinaryBytes(memory_buffer * Buffer, void * Bytes, u32 BitCountToPrint);
void Str8WriteBinary(memory_buffer * Buffer, blob Data);

// base 64 encoding

void Str8WriteBase64Decode(memory_buffer * Buffer, str8 String, char8 Padding);
void Str8WriteBase64Encode(memory_buffer * Buffer, blob Data, char8 Padding);

str8 Str8FromBase64Decode(memory_arena * Arena, str8 String, char8 Padding);
str8 Str8FromBase64Encode(memory_arena * Buffer, blob Data, char8 Padding);

// string linked list

typedef struct str8_node str8_node;
struct str8_node {
	str8 Value;
	str8_node * Next;
};

typedef struct str8ll {
	str8_node * First;
	str8_node * Last;
	u32 Count;
} str8ll;

str8_node * Str8NodeFromStr8(memory_arena * Arena, str8 String);
str8ll Str8LLFromStr8(memory_arena * Arena, str8 String);
void Str8LLPush(memory_arena * Arena, str8ll * StringList, str8 String);
void Str8LLPushFront(memory_arena * Arena, str8ll * StringList, str8 String);
str8ll Str8LLConcat(str8ll StringLLPrefix, str8ll StringLLSuffix);

void Str8WriteStr8LL(memory_buffer * Buffer, str8ll StringList);

#ifdef __cplusplus
}
#endif

#endif