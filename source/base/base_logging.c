#include "base_include.h"

thread_var logging GlobalLogging;

const char * LogError_Generic = "Error";
const char * LogInfo_Generic = "Info";

void Logging(bool32 Error, u32 Line, str8 FileName, const char * Type, char * FormatString, ...)
{
	if (GlobalLogging.Arena == 0)
	{
		GlobalLogging.Arena = ArenaCreate(0);
	}
	memory_arena * Arena = GlobalLogging.Arena;

	log_entry * Result = ArenaPush(Arena, log_entry);

	Result->Error = Error;
	Result->Line = Line;
	Result->FileName = FileName;

	if (Type == 0 && Error)
	{
		Type = LogError_Generic;
	}
	else if (Type == 0 && !Error)
	{
		Type = LogInfo_Generic;
	}
	Result->Type = Type;

	memory_buffer * Buffer = ScratchBufferStart();
	
	va_list FormatArguments;
	va_start(FormatArguments, FormatString);
	Str8WriteFmtCore(Buffer, Str8FromCStr(FormatString), FormatArguments);
	va_end(FormatArguments);

	Result->Message = ScratchBufferEndStr8(Buffer, Arena);

	if (Error)
	{
		GlobalLogging.Error = true;
		GlobalLogging.ErrorType = Type;
	}

	SLLStackPush(GlobalLogging.Start, Result);
}