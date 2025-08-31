#ifndef base_logging_h
#define base_logging_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct log_entry log_entry;
struct log_entry {
	const char * Type;
	u32 Line;
	str8 FileName;
	str8 Message;
	log_entry * Next;
	bool32 Error;
};

typedef struct logging {
	log_entry * Start;
	bool32 Error;
	const char * ErrorType;
	memory_arena * Arena;
} logging;

extern thread_var logging GlobalLog;

bool8 Logging(bool32 Error, u32 Line, str8 FileName, const char * Type, char * FormatString, ...);

#define LogError(Type, ...) Logging(true, __LINE__, Str8Lit(__FILE__), Type, __VA_ARGS__)
#define LogInfo(Type, ...) Logging(false, __LINE__, Str8Lit(__FILE__), Type, __VA_ARGS__)

#define CheckErrorGotoCleanup(NewError, ...) if (!NewError) 

// #define CheckError(ErrorFlag, NewError, ...) if (ErrorFlag) \
// for (bool32 ConcatLine(_Loop) = true; ConcatLine(_Loop); (NewError) && ((ErrorFlag) = !!(NewError)) && LogError(Type, __VA_ARGS__)), ConcatLine(_Loop) = false)
// #define CheckErrorNoLog(ErrorFlag, NewError) if (ErrorFlag) \
// for (bool32 ConcatLine(_Loop) = true; ConcatLine(_Loop); (NewError) && ((ErrorFlag) = !!(NewError)), ConcatLine(_Loop) = false)

#ifdef __cplusplus
}
#endif

#endif