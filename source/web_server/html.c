#include "../reuse/base/base_include.h"
#include "html.h"

str8 Str8FromHTML(memory_arena * Arena, html_node * Nodes)
{
    html_node * TagStack[HtmlMaxTagDepth] = { 0 };
    u32 TagChildCountStack[HtmlMaxTagDepth] = { 0 };
    u32 StackIndex = 0;

    memory_buffer * Buffer = ScratchBufferStart();

    html_node * Node = Nodes;
    while (Node)
    {
		html_node * TagNode = Node;

        Str8WriteFmt(Buffer, "<%{str8}", TagNode->Type->Name);

        if (TagNode->AttrCount)
        {
            for (u32 Index = 0; Index < TagNode->AttrCount; Index++)
            {
                Node = Node->Next;
                Str8WriteFmt(Buffer, " %{str8}=\"%{str8}\"", Node->Type->Name, Node->Content);
            }
        }

        if (TagNode->StyleCount)
        {
            Str8WriteFmt(Buffer, " style=\"");

            for (u32 Index = 0; Index < TagNode->StyleCount; Index++)
            {
                Node = Node->Next;
                Str8WriteFmt(Buffer, "%{str8}:%{str8};", Node->Type->Name, Node->Content);
            }

            Str8WriteFmt(Buffer, "\"");
        }

        Node = Node->Next;

        if (TagNode->ChildTagCount)
        {
            TagStack[StackIndex] = TagNode;
            TagChildCountStack[StackIndex] = TagNode->ChildTagCount;

            Str8WriteFmt(Buffer, ">");

            StackIndex++;
        }
        else
        {
            if (TagNode->Content.Count)
            {
                Str8WriteFmt(Buffer, ">%{str8}</%{str8}>", TagNode->Content, TagNode->Type->Name);
            }
            else
            {
                Str8WriteFmt(Buffer, " />");
            }

            while (StackIndex && --TagChildCountStack[StackIndex - 1] == 0)
            {
                Str8WriteFmt(Buffer, "</%{str8}>", TagStack[StackIndex - 1]->Type->Name);

                TagStack[StackIndex - 1] = 0;
                StackIndex--;
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
    Writer.LastNode = Root;

	return Writer;
}

html_node * HTMLStartTagKey(html_writer * Writer, html_node_type * Type, u64 Key)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
	Node->Type = Type;
	Node->Key = Key;

	Writer->CurrentTag->ChildTagCount++;
	Writer->CurrentTag = Node;
    Writer->LastNode->Next = Node;
    Writer->LastNode = Node;
	Writer->TagStack[Writer->StackIndex++] = Node;

    return Node;
}

html_node * HTMLSingleTagKey(html_writer * Writer, html_node_type * Type, u64 Key)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
    Node->Type = Type;
    Node->Key = Key;

    Writer->CurrentTag->ChildTagCount++;
	Writer->LastNode->Next = Node;
    Writer->LastNode = Node;

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

	// todo: verify attributes directly follow the open tag or another attribute

    Writer->CurrentTag->AttrCount++;
    Writer->LastNode->Next = Node;
    Writer->LastNode = Node;
}

html_node * HTMLStyle(html_writer * Writer, html_node_type * Style, str8 Value)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
    Node->Type = Style;
    Node->Content = Value;

    // todo: verify attributes directly follow the open tag, an attribute or another style

    Writer->CurrentTag->StyleCount++;
    Writer->LastNode->Next = Node;
    Writer->LastNode = Node;
}