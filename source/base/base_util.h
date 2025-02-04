#ifndef base_util_h
#define base_util_h

// compiler context

#if defined(_WIN32)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS
	#endif
#elif __APPLE__
    #define OS_MAC
    #define OS_POSIX
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
    #define OS_POSIX
#elif __wasm__
    #define OS_WASM
#endif

#if defined(_MSC_VER)
    #define COMPILER_MSVC
#elif defined(__clang__)
    #define COMPILER_CLANG
#elif defined(__GNUC__)
    #define COMPILER_GCC
#else
    #define COMPILER_UNKNOWN
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)	
	#if defined(__x86_64__)
		#define ARCH_X64
	#endif
	#if defined(__i386__)
		#define ARCH_X86
	#endif
#endif
#if defined(COMPILER_MSVC)
	#if defined(_WIN64)
		#define ARCH_X64
	#endif
	#if defined(_WIN32)
		#define ARCH_X86
	#endif
#endif

#if defined(COMPILER_MSVC)
    #define thread_var __declspec(thread)
#elif defined(COMPILER_CLANG)
    #define thread_var __thread
#endif

#include <stdarg.h>

// In some compilers you have to dereference va_lists to pass them around.
// In other compilers... you maybe don't?

#if 1
    typedef va_list * va_list_ptr;
    #define RefVarArgs(VarArgs) ((va_list_ptr) (&(VarArgs)))
    #define DerefVarArgs(VarArgs) (*(VarArgs))
#else
    typedef va_list * va_list_ptr;
    #define RefVarArgs(VarArgs) ((va_list_ptr) (VarArgs))
    #define DerefVarArgs(VarArgs) ((VarArgs))
#endif

// assert

#if !(defined(OS_WASM))
    #include <assert.h>
    #define Assert(...) assert(__VA_ARGS__)
#else
    #define Assert(...)
#endif

// general utilities

#define internal_variable static
#define local_persist static

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define switch_case(Value) break; case (Value):
#define through_case(Value) case (Value):
#define switch_default break; default:
#define through_default default:

#define IsEmpty(...) (sizeof("_" # __VA_ARGS__) == 2)

#define loop while (true)

#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) > (b) ? (b) : (a))
#define Clamp(max, min, val) Min(Max(val, min), max)
#define UnitClamp(val) Clamp(1, -1, val)
#define AlignPow2(value, pow2) (((value) + ((pow2) - 1)) & (~((pow2) - 1)))

#define Log2Of10Const (3.32192809489)
#define PiConst (3.14159265358972323846264338327950288)
#define EConst (2.718281828)

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)

#define CreatePointer(Type, Variable, Value) Type _Instant_##Variable = Value, * Variable = &_Instant_##Variable
#define LValue(Type, Value) (*((Type[]) {(Value)}))
#define LValuePtr(Type, Value) ((Type[]) {(Value)})
#define Reinterpret(OldType, NewType, Value) (*(NewType *) ((OldType[]) {(Value)}))

#define MacroEval(MacroA) MacroA
#define Concat(SymbolA, SymbolB) SymbolA ## SymbolB
#define MacroConcat(MacroA, MacroB) Concat(MacroA, MacroB)

#define ConcatLine(Symbol) Concat(__LINE__, Symbol)
#define DeferLoop(CodeA, CodeB) for (bool ConcatLine(_loop) = true; ConcatLine(_loop) ? (CodeA, true) : false; CodeB, ConcatLine(_loop) = false)

#define InvalidCodePath() Assert(false)

#define alignof _Alignof

// linked list macros

#define DLLPushBack_Named(First, Last, Element, Next, Previous) (\
	/*if*/ (First) == 0 ?\
	(\
		(First) = (Last) = (Element),\
		(Element)->Next = (Element)->Previous = 0\
	)\
	/*else*/ :\
	(\
		(Element)->Previous = (Last),\
		(Last)->Next = (Element),\
		(Last) = (Element),\
		(Element)->Next = 0\
	)\
)
#define DLLPushBack(First, Last, Element) DLLPushBack_Named(First, Last, Element, Next, Previous)

#define DLLPushFront(First, Last, Element) DLLPushBack_Named(Last, First, Element, Next, Previous)

#define DLLRemove_Named(First, Last, Element, Next, Previous) (\
	/*if*/ (First) == (Element) ?\
	(\
		/*if*/ (First) == (Last) ?\
		(\
			(First) = (Last) = (0)\
		)\
		/*else*/ :\
		(\
			(First) = (First)->Next,\
			(First)->Previous = 0\
		)\
	)\
	/*else*/ :\
	(\
		/*if*/ (Last) == (Element) ?\
		(\
			(Last) = (Last)->Previous,\
			(Last)->Next = 0\
		)\
		/*else*/:\
		(\
			(Element)->Next->Previous = (Element)->Previous,\
			(Element)->Previous->Next = (Element)->Next\
		)\
	)\
)
#define DLLRemove(First, Last, Element) DLLRemove_Named(First, Last, Element, Next, Previous)

#define SLLQueuePush_Named(First, Last, Element, Next) (\
	/*if*/ (First) == 0 ?\
	(\
		(First) = (Last) = (Element)\
	)\
	/*else*/ :\
	(\
		(Last)->Next = (Element),\
		(Last) = (Element)\
	),\
	(Element)->Next = 0\
)
#define SLLQueuePush(First, Last, Element) SLLQueuePush_Named(First, Last, Element, Next)

#define SLLQueuePushFront_Named(First, Last, Element, Next) (\
	/*if*/ (First) == 0 ?\
	(\
		(First) = (Last) = (Element),\
		(Element)->Next = 0\
	)\
	/*else*/ :\
	(\
		(Element)->Next = (First),\
		(First) = (Element)\
	)\
)
#define SLLQueuePushFront(First, Last, Element) SLLQueuePushFront_Named(First, Last, Element, Next)

#define SLLQueuePop_Named(First, Last, Next) (\
	/*if*/ (First) == (Last) ?\
	(\
		(First) = (Last) = 0\
	)\
	/*else*/ :\
	(\
		(First) = (First)->Next\
	)\
)
#define SLLQueuePop(First, Last) SLLQueuePop_Named(First, Last, Next)

#define SLLStackPush_Named(First, Element, Next) (\
	(Element)->Next = (First),\
	(First) = (Element)\
)
#define SLLStackPush(First, Element) SLLStackPush_Named(First, Element, Next)

#define SLLStackPop_Named(First, Next) (\
	/*if*/ (First) == 0 ?\
	(\
		0\
	)\
	/*else*/ :\
	(\
		(First) = (First)->Next\
	)\
)
#define SLLStackPop(First) SLLStackPop_Named(First, Next)

#endif
