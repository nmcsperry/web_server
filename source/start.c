#include "base/base_include.h"
#include "io/io_include.h"
#include "network/network_include.h"

#include "base/base_include.c"
#include "io/io_include.c"
#include "network/network_include.c"

void EntryHook()
{
	StdOutputFmt("Hello, World!");
}