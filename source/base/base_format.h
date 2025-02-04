#ifndef base_format_h
#define base_format_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct format_properties
{
	union {
		bool32 BooleanProperties[4];
		struct {
			bool32 LeftJustify;
			bool32 Sign;
			bool32 SignSpacing;
			bool32 ZeroPadding;
		};
	};
	union {
		i32 IntegerProperties[2];
		struct {
			i32 Width;
			i32 Precision;
		};
	};
} format_properties;

typedef void format_function(memory_buffer *, str8, va_list_ptr);

typedef struct format_function_mapping {
	str8 Name;
	format_function * FormatFunction;
	bool32 Valid;
} format_function_mapping;

extern format_function_mapping FormatFunctions[];

void Str8WriteFmtCore(memory_buffer * Buffer, str8 Format, va_list FormatArguments);
str8 Str8Fmt(memory_arena * Arena, char * FormatCStr, ...);

#ifdef __cplusplus
}
#endif

#endif