#include "../base/base_include.h"
#include "io_include.h"

void StdOutputBuffer(memory_buffer * Buffer)
{
	str8 String = Str8FromBuffer(Buffer);
	StdOutput(String);
}

void StdOutputFmt(const char * FormatCStr, ...)
{
	memory_buffer * Buffer = ScratchBufferStart();

	va_list FormatArguments;
	va_start(FormatArguments, FormatCStr);

	Str8WriteFmtCore(Buffer, Str8FromCStr((char *)FormatCStr), FormatArguments);

	va_end(FormatArguments);

	StdOutputBuffer(Buffer);

	ScratchBufferRelease(Buffer);
}