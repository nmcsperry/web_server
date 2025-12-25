#include "../reuse/base/base_include.h"
#include "html.h"

str8 Str8FromHTML(memory_arena * Arena, html_node * Nodes)
{
    html_node * TagStack[HtmlMaxTagDepth] = { 0 };
    html_node * TagChildrenStack[HtmlMaxTagDepth] = { 0 };
    u32 StackIndex = 0;

    memory_buffer * Buffer = ScratchBufferStart();

    html_node * Node = Nodes;
    while (Node)
    {
        Str8WriteFmt(Buffer, "<%{str8}", Node->Type->Name);

        for (html_node * Child = Node->UnorderedChildren; Child; Child = Child->Next)
        {
            if (Child->Type->Class != HtmlNodeClass_Attr)
            {
                continue;
            }
            Str8WriteFmt(Buffer, " %{str8}=\"%{str8}\"", Child->Type->Name, Child->Content);
        }

		bool32 FirstStyle = true;
        for (html_node * Child = Node->UnorderedChildren; Child; Child = Child->Next)
        {
            if (Child->Type->Class != HtmlNodeClass_Style)
            {
                continue;
            }
            if (FirstStyle)
            {
                Str8WriteFmt(Buffer, " style=\"");
                FirstStyle = false;
			}
            Str8WriteFmt(Buffer, "%{str8}:%{str8};", Child->Type->Name, Child->Content);
        }
        if (!FirstStyle)
        {
            Str8WriteFmt(Buffer, "\"");
        }

        if (Node->Children)
        {
            Str8WriteFmt(Buffer, ">");

            TagStack[StackIndex] = Node;
            TagChildrenStack[StackIndex] = Node->Children;
            StackIndex++;

            Node = Node->Children;
        }
        else
        {
            if (Node->Content.Count)
            {
                Str8WriteFmt(Buffer, ">%{str8}</%{str8}>", Node->Content, Node->Type->Name);
            }
            else
            {
                Str8WriteFmt(Buffer, " />");
            }

            // todo: this works but is sort of weird
            while (StackIndex)
            {
				TagChildrenStack[StackIndex - 1] = TagChildrenStack[StackIndex - 1]->Next;
                if (TagChildrenStack[StackIndex - 1] != 0)
                {
					Node = TagChildrenStack[StackIndex - 1];
                    break;
                }

                Str8WriteFmt(Buffer, "</%{str8}>", TagStack[StackIndex - 1]->Type->Name);

                TagStack[StackIndex - 1] = 0;
                StackIndex--;
                Node = 0;
            }
        }
    }

    return ScratchBufferEndStr8(Buffer, Arena);
}

html_writer HTMLWriterCreate(memory_arena * Arena)
{
    html_writer Writer = { 0 };
    Writer.Arena = Arena;

	html_node * Root = ArenaPushZero(Arena, html_node);
	Root->Type = HTMLTag_html;
	
    Writer.DocumentRoot = Root;
    Writer.CurrentTag = Root;

	return Writer;
}

void HTMLAppendTagOrdered(html_node * Parent, html_node * Child)
{
    Child->Next = 0;

    html_node * LastChild = Parent->Children;
    while (LastChild && LastChild->Next)
    {
		LastChild = LastChild->Next;
    }

    if (LastChild)
    {
        LastChild->Next = Child;
    }
    else
    {
		Parent->Children = Child;
    }
}

void HTMLAppendTagUnordered(html_node * Parent, html_node * Child)
{
    html_node * PrevSibling = 0;
    html_node * NextSibling = Parent->UnorderedChildren;
    while (NextSibling && NextSibling->Type < Child->Type)
    {
		PrevSibling = NextSibling;
        NextSibling = NextSibling->Next;
    }

	Child->Next = NextSibling;
    if (PrevSibling)
    {
        PrevSibling->Next = Child;
    }
    else
    {
        Parent->UnorderedChildren = Child;
	}
}

html_node * HTMLStartTagKey(html_writer * Writer, html_node_type * Type, u64 Key)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
	Node->Type = Type;
	Node->Key = Key;

	HTMLAppendTagOrdered(Writer->CurrentTag, Node);
	Writer->CurrentTag = Node;
	Writer->TagStack[Writer->StackIndex++] = Node;

    return Node;
}

html_node * HTMLSingleTagKey(html_writer * Writer, html_node_type * Type, u64 Key)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
    Node->Type = Type;
    Node->Key = Key;

    HTMLAppendTagOrdered(Writer->CurrentTag, Node);

    return Node;
}

html_node * HTMLStartTag(html_writer * Writer, html_node_type * Type)
{
	HTMLStartTagKey(Writer, Type, 0);
}

html_node * HTMLSingleTag(html_writer * Writer, html_node_type * Type)
{
    HTMLSingleTagKey(Writer, Type, 0);
}

html_node * HTMLEndTag(html_writer * Writer)
{
    if (Writer->StackIndex <= 1)
    {
		Writer->CurrentTag = Writer->DocumentRoot;
        return 0;
    }

    Writer->TagStack[Writer->StackIndex--] = 0;
    Writer->CurrentTag = Writer->TagStack[Writer->StackIndex - 1];
	return Writer->CurrentTag;
}

html_node * HTMLText(html_writer * Writer, str8 Text)
{
    Writer->CurrentTag->Content = Text;
}

html_node * HTMLTextCStr(html_writer * Writer, char * CStr)
{
    Writer->CurrentTag->Content = Str8FromCStr(CStr);
}

html_node * HTMLSimpleTag(html_writer * Writer, html_node_type * Type, str8 String)
{
    HTMLTag(Writer, Type)
    {
        HTMLText(Writer, String);
    }
}

html_node * HTMLSimpleTagCStr(html_writer * Writer, html_node_type * Type, char * CStr)
{
    HTMLTag(Writer, Type)
    {
        HTMLTextCStr(Writer, CStr);
    }
}

html_node * HTMLSimpleTagFmt(html_writer * Writer, html_node_type * Type, char * FormatCStr, ...)
{
    va_list FormatArguments;
    va_start(FormatArguments, FormatCStr);

    str8 Result = Str8FmtCore(Writer->Arena, Str8FromCStr(FormatCStr), FormatArguments);

    va_end(FormatArguments);

    HTMLTag(Writer, Type)
    {
        HTMLText(Writer, Result);
    }
}

html_node * HTMLTextFmt(html_writer * Writer, char * FormatCStr, ...)
{
    va_list FormatArguments;
    va_start(FormatArguments, FormatCStr);

    str8 Result = Str8FmtCore(Writer->Arena, Str8FromCStr(FormatCStr), FormatArguments);

    va_end(FormatArguments);

	HTMLText(Writer, Result);
}

html_node * HTMLAttr(html_writer * Writer, html_node_type * Attr, str8 Value)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
    Node->Type = Attr;
    Node->Content = Value;

    HTMLAppendTagUnordered(Writer->CurrentTag, Node);
}

html_node * HTMLStyle(html_writer * Writer, html_node_type * Style, str8 Value)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
    Node->Type = Style;
    Node->Content = Value;

    // todo: verify attributes directly follow the open tag, an attribute or another style

    HTMLAppendTagUnordered(Writer->CurrentTag, Node);
}