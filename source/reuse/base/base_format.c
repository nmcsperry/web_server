#include "base_include.h"

// #include <stdio.h>

format_properties ParseFormatProperties(str8 FormatString, va_list_ptr FormatArguments)
{
	format_properties Result = { 0 };

	// skip format type
	Str8ParseNextToken(&FormatString);
	Str8ParseEatWhitespace(&FormatString);

	str8 PropertyList[] = {
		Str8Lit("leftjustify"),
		Str8Lit("sign"),
		Str8Lit("signspace"),
		Str8Lit("zeropad"),
		Str8Lit("width"),
		Str8Lit("precision")
	};

	while (FormatString.Count)
	{
		i32 ArgType = Str8ParseExpectAny(&FormatString, PropertyList, ArrayCount(PropertyList), MatchFlag_IgnoreCase);
		if (ArgType >= 0 && ArgType <= 3) // boolean, do not expect anything else
		{
			bool32 Value = true;

			if (FormatString.Count != 0 && Char8IsWhitespace(FormatString.Data[0]))
			{
				if (Str8ParseExpect(&FormatString, Str8Lit(":"), MatchFlag_Normal))
				{
					str8 Token = Str8ParseEatUntilCharMatch(&FormatString, Char8ClassWhitespace);
					
					if (Str8Match(Str8Lit("*"), Token, MatchFlag_Normal))
					{
						Value = va_arg(DerefVarArgs(FormatArguments), bool32);
					}
					else
					{
						Value = BoolFromStr8(Token);
					}
				}
			}

			Result.BooleanProperties[ArgType] = Value;
		}
		else if (ArgType >= 4 && ArgType <= 5)
		{
			if (!Str8ParseExpect(&FormatString, Str8Lit(":"), MatchFlag_Normal))
			{
				return Result;
			}

			i32 Value = 0;
			str8 Token = Str8ParseEatUntilCharMatch(&FormatString, Char8ClassWhitespace);

			if (Str8Match(Str8Lit("*"), Token, MatchFlag_Normal))
			{
				Value = va_arg(DerefVarArgs(FormatArguments), i32);
			}
			else
			{
				Value = IntFromStr8(Token, 0);
			}
			Result.IntegerProperties[ArgType - 4] = Value;
		}
		else if (ArgType == -1)
		{
			// todo: handle this better
			Str8ParseEatUntilCharMatch(&FormatString, Char8ClassWhitespace);
		}

		Str8ParseEatWhitespace(&FormatString);
	}

	return Result;
}

void FormatDebug(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	format_properties Props = ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteStr8(Buffer, Str8Lit("%{leftjustify:"));
	Str8WriteBool(Buffer, Props.LeftJustify);
	Str8WriteStr8(Buffer, Str8Lit(", sign:"));
	Str8WriteBool(Buffer, Props.Sign);
	Str8WriteStr8(Buffer, Str8Lit(", signspace:"));
	Str8WriteBool(Buffer, Props.SignSpacing);
	Str8WriteStr8(Buffer, Str8Lit(", zeropad:"));
	Str8WriteBool(Buffer, Props.ZeroPadding);
	Str8WriteStr8(Buffer, Str8Lit(", width:"));
	Str8WriteInt(Buffer, Props.Width);
	Str8WriteStr8(Buffer, Str8Lit(", precision:"));
	Str8WriteInt(Buffer, Props.Precision);
	Str8WriteStr8(Buffer, Str8Lit("}"));
}

void FormatStr8(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	str8 Value = va_arg(DerefVarArgs(FormatArguments), str8);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteStr8(Buffer, Value);
}

void FormatStr16(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	str16 Value = va_arg(DerefVarArgs(FormatArguments), str16);
	format_properties Props = ParseFormatProperties(FormatString, FormatArguments);
	Utf8WriteUtf16(Buffer, Value);

	i32 Width = Value.Count; // ugh
	while (Width <= Props.Width)
	{
		Str8WriteChar8(Buffer, ' ');
		Width++;
	}
}

void FormatStr32(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	str32 Value = va_arg(DerefVarArgs(FormatArguments), str32);
	format_properties Props = ParseFormatProperties(FormatString, FormatArguments);
	Utf8WriteUtf32(Buffer, Value);

	i32 Width = Value.Count; // ugh
	while (Width <= Props.Width)
	{
		Str8WriteChar8(Buffer, ' ');
		Width++;
	}
}

void FormatI32(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	i32 Value = va_arg(DerefVarArgs(FormatArguments), i32);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteInt(Buffer, Value);
}

void FormatI64(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	i64 Value = va_arg(DerefVarArgs(FormatArguments), i64);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteInt(Buffer, Value);
}

void FormatU32(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	u32 Value = va_arg(DerefVarArgs(FormatArguments), u32);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteUInt(Buffer, Value);
}

void FormatU64(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	u64 Value = va_arg(DerefVarArgs(FormatArguments), u64);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteUInt(Buffer, Value);
}

void FormatU32Hex(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	u32 Value = va_arg(DerefVarArgs(FormatArguments), u32);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteHex(Buffer, Value);
}

void FormatU64Hex(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	u64 Value = va_arg(DerefVarArgs(FormatArguments), u64);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteHex(Buffer, Value);
}

void FormatChar8(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	char8 Value = va_arg(DerefVarArgs(FormatArguments), char32);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteChar8(Buffer, Value);
}

void FormatChar16(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	char16 Value = va_arg(DerefVarArgs(FormatArguments), char32);
	ParseFormatProperties(FormatString, FormatArguments);
	str16 String = (str16) { .Data = &Value, .Count = 1 };
	Utf8WriteUtf16(Buffer, String);
}

void FormatChar32(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	char32 Value = va_arg(DerefVarArgs(FormatArguments), char32);
	ParseFormatProperties(FormatString, FormatArguments);
	str32 String = (str32) { .Data = &Value, .Count = 1 };
	Utf8WriteUtf32(Buffer, String);
}

void FormatF64(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	f64 Value = va_arg(DerefVarArgs(FormatArguments), f64);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteFloat(Buffer, Value);
}

void FormatBool(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	bool32 Value = va_arg(DerefVarArgs(FormatArguments), bool32);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteBool(Buffer, Value);
}

void FormatCStr8(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	char * Value = va_arg(DerefVarArgs(FormatArguments), char *);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteCStr(Buffer, Value);
}

void FormatCStr16(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	char16 * Value = va_arg(DerefVarArgs(FormatArguments), char16 *);
	ParseFormatProperties(FormatString, FormatArguments);
	str16 CString = Str16FromCStr(Value);
	Utf8WriteUtf16(Buffer, CString);
}

void FormatCStr32(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	char32 * Value = va_arg(DerefVarArgs(FormatArguments), char32 *);
	ParseFormatProperties(FormatString, FormatArguments);
	str32 CString = Str32FromCStr(Value);
	Utf8WriteUtf32(Buffer, CString);
}

void FormatBinary(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	blob Data = va_arg(DerefVarArgs(FormatArguments), blob);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteBinary(Buffer, Data);
}

void FormatBase64(memory_buffer * Buffer, str8 FormatString, va_list_ptr FormatArguments) {
	blob Data = va_arg(DerefVarArgs(FormatArguments), blob);
	char8 Padding = (char) va_arg(DerefVarArgs(FormatArguments), u32);
	ParseFormatProperties(FormatString, FormatArguments);
	Str8WriteBase64Encode(Buffer, Data, Padding);
}

format_function_mapping FormatFunctions[] = {
	{ Str8LitInit("formatdebug"), &FormatDebug, true },
	
	{ Str8LitInit("i8"), &FormatI32, true },
	{ Str8LitInit("i16"), &FormatI32, true },
	{ Str8LitInit("i32"), &FormatI32, true },
	{ Str8LitInit("i64"), &FormatI64, true },
	{ Str8LitInit("int"), &FormatI32, true },
	
	{ Str8LitInit("u8"), &FormatU32, true },
	{ Str8LitInit("u16"), &FormatU32, true },
	{ Str8LitInit("u32"), &FormatU32, true },
	{ Str8LitInit("u64"), &FormatU64, true },
	{ Str8LitInit("hex32"), &FormatU32Hex, true },
	{ Str8LitInit("hex64"), &FormatU64Hex, true },
	
	{ Str8LitInit("char8"), &FormatChar8, true },
	{ Str8LitInit("char16"), &FormatChar16, true },
	{ Str8LitInit("char32"), &FormatChar32, true },
	{ Str8LitInit("char"), &FormatChar8, true },
	
	{ Str8LitInit("f32"), &FormatF64, true },
	{ Str8LitInit("f64"), &FormatF64, true },
	{ Str8LitInit("float"), &FormatF64, true },
	{ Str8LitInit("double"), &FormatF64, true },
	
	{ Str8LitInit("bool8"), &FormatBool, true },
	{ Str8LitInit("bool16"), &FormatBool, true },
	{ Str8LitInit("bool32"), &FormatBool, true },
	{ Str8LitInit("bool"), &FormatBool, true },
	
	{ Str8LitInit("str8"), &FormatStr8, true },
	{ Str8LitInit("str16"), &FormatStr16, true },
	{ Str8LitInit("str32"), &FormatStr32, true },
	
	{ Str8LitInit("cstr8"), &FormatCStr8, true },
	{ Str8LitInit("cstr16"), &FormatCStr16, true },
	{ Str8LitInit("cstr32"), &FormatCStr32, true },
	{ Str8LitInit("cstr"), &FormatCStr8, true },
	
	{ Str8LitInit("binary"), &FormatBinary, true },
	{ Str8LitInit("base64"), &FormatBase64, true },
};

format_function * FormatFunctionFromName(str8 Name)
{
	for (i32 FormatFunctionIndex = 0; FormatFunctionIndex < ArrayCount(FormatFunctions); FormatFunctionIndex++)
	{
		if (!FormatFunctions[FormatFunctionIndex].Valid)
		{
			return 0;
		}
		
		str8 CurrentName = FormatFunctions[FormatFunctionIndex].Name;

		if (Str8Match(CurrentName, Name, MatchFlag_IgnoreCase))
		{
			return FormatFunctions[FormatFunctionIndex].FormatFunction;
		}
	}

	return 0;
}

void Str8WriteFmtCore(memory_buffer * Buffer, str8 Format, va_list FormatArguments)
{
	char8_class PercentChar = (char8_class) { .IncludeChars = Str8Lit("%") };
	char8_class EndBraceChar = (char8_class) { .IncludeChars = Str8Lit("}") };

	while (Format.Count > 0)
	{
		Str8WriteStr8(Buffer, Str8ParseEatUntilCharMatch(&Format, PercentChar));
		Str8ParseEat(&Format, 1);
		
		if (Format.Count > 0)
		{
			if (Format.Data[0] == '{')
			{
				Str8ParseEat(&Format, 1);
				str8 FormatString = Str8ParseEatUntilCharMatch(&Format, EndBraceChar);
				str8 FormatStringPeek = FormatString;

				if (Format.Count > 0)
				{
					Str8ParseEat(&Format, 1);
					
					str8 FormatName = Str8ParseNextToken(&FormatStringPeek);
					format_function * FormatFunction = FormatFunctionFromName(FormatName);
					if (FormatFunction)
					{
						FormatFunction(Buffer, FormatString, RefVarArgs(FormatArguments));
					}
					else
					{
						Str8WriteStr8(Buffer, Str8Lit("(Format Error)"));
					}
				}
				else
				{
					Str8WriteStr8(Buffer, Str8Lit("(Format Error)"));
				}
			}
			else
			{
				if (Format.Data[0] == '%')
				{
					Str8ParseEat(&Format, 1);
				}
				Str8WriteChar8(Buffer, '%');
			}
		}
	}
}

void Str8WriteFmt(memory_buffer * Buffer, char * FormatCStr, ...)
{
	va_list FormatArguments;
	va_start(FormatArguments, FormatCStr);
	Str8WriteFmtCore(Buffer, Str8FromCStr(FormatCStr), FormatArguments);
	va_end(FormatArguments);
}

str8 Str8FmtCore(memory_arena * Arena, str8 Format, va_list FormatArguments)
{
	memory_buffer * Buffer = ScratchBufferStart();
	Str8WriteFmtCore(Buffer, Format, FormatArguments);
	return ScratchBufferEndStr8(Buffer, Arena);
}

str8 Str8Fmt(memory_arena * Arena, char * FormatCStr, ...)
{
	va_list FormatArguments;
	va_start(FormatArguments, FormatCStr);

	str8 Result = Str8FmtCore(Arena, Str8FromCStr(FormatCStr), FormatArguments);

	va_end(FormatArguments);

	return Result;
}