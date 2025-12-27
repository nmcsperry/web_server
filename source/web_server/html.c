#include "../reuse/base/base_include.h"
#include "../reuse/io/io_include.h"
#include "html.h"

str8 Str8FromHTML(memory_arena * Arena, html_node * Nodes)
{
    html_node * TagStack[HtmlMaxTagDepth] = { 0 };
    u32 StackIndex = 0;

    TagStack[StackIndex++] = Nodes;

    memory_buffer * Buffer = ScratchBufferStart();

    while (StackIndex)
    {
        html_node * Node = TagStack[StackIndex - 1];

        Str8WriteFmt(Buffer, "<%{str8}", Node->Type->Name);

        for (html_node * Child = Node->UnorderedChildren; Child; Child = Child->Next)
        {
            if (Child->Type->Class != HtmlNodeClass_Attr) continue;
            Str8WriteFmt(Buffer, " %{str8}=\"%{str8}\"", Child->Type->Name, Child->Content);
        }

		bool32 FirstStyle = true;
        for (html_node * Child = Node->UnorderedChildren; Child; Child = Child->Next)
        {
            if (Child->Type->Class != HtmlNodeClass_Style) continue;
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

            TagStack[StackIndex++] = Node->Children;
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

            while (StackIndex)
            {
                TagStack[StackIndex - 1] = TagStack[StackIndex - 1]->Next;

                if (TagStack[StackIndex - 1])
                {
                    break;
                }
                else if (--StackIndex)
                {
                    Str8WriteFmt(Buffer, "</%{str8}>", TagStack[StackIndex - 1]->Type->Name);
                }
            }
        }
    }

    return ScratchBufferEndStr8(Buffer, Arena);
}

html_writer HTMLWriterCreate(memory_arena * Arena, html_node * DiffRoot)
{
    html_writer Writer = { 0 };
    Writer.Arena = Arena;

	html_node * Root = ArenaPushZero(Arena, html_node);
	Root->Type = HTMLTag_html;
	
    Writer.DocumentRoot = Root;
    Writer.TagStack[0] = Root;

    if (DiffRoot)
    {
        Writer.DiffRoot = DiffRoot;
        Writer.DiffTagStack[0] = DiffRoot;

        // todo: maybe we should check if they are the same type, although we can assume they are...
    }

	return Writer;
}

void HTMLAppendTagOrdered(html_writer * Writer, html_node * Child)
{
    html_node * Parent = Writer->TagStack[Writer->StackIndex];
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

    Writer->StackIndex++;
    Writer->TagStack[Writer->StackIndex] = Child;

    if (Writer->DiffRoot)
    {
        if (Writer->DiffTagStack[Writer->StackIndex] == 0)
        {
            if (Writer->DiffTagStack[Writer->StackIndex - 1] == &DiffTerminator)
            {
                Writer->DiffTagStack[Writer->StackIndex] = &DiffTerminator;
            }
            else
            {
                Writer->DiffTagStack[Writer->StackIndex] = Writer->DiffTagStack[Writer->StackIndex - 1]->Children;
                if (Writer->DiffTagStack[Writer->StackIndex] == 0)
                {
                    Writer->DiffTagStack[Writer->StackIndex] = &DiffTerminator;
                }
            }
        }

        if (Writer->DiffTagStack[Writer->StackIndex] == &DiffTerminator)
        {
            HTMLDiffInsertAll(Writer, Parent, Child);
        }
        else if (Writer->DiffTagStack[Writer->StackIndex]->Type != Child->Type)
        {
            HTMLDiffReplace(Writer, Writer->DiffTagStack[Writer->StackIndex], Child);
        }
    }
}

void HTMLAppendTagUnordered(html_writer * Writer, html_node * Child)
{
    html_node * Parent = Writer->TagStack[Writer->StackIndex];

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

	HTMLAppendTagOrdered(Writer, Node);

    return Node;
}

html_node * HTMLSingleTagKey(html_writer * Writer, html_node_type * Type, u64 Key)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
    Node->Type = Type;
    Node->Key = Key;

    HTMLAppendTagOrdered(Writer, Node);
    HTMLEndTag(Writer);

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
    if (Writer->StackIndex == 0)
    {
        return 0;
    }

    if (Writer->DiffRoot && Writer->DiffTagStack[Writer->StackIndex] != &DiffTerminator)
    {
        html_node * Tag = Writer->TagStack[Writer->StackIndex];
        html_node * DiffTag = Writer->DiffTagStack[Writer->StackIndex];

        if (Tag->Type == DiffTag->Type)
        {
            // compare content
            if (!Str8Match(Tag->Content, DiffTag->Content, 0))
            {
                HTMLDiffReplaceContent(Writer, DiffTag, Tag);
            }

            // compare unordered items
            html_node * Child = Tag->UnorderedChildren;
            html_node * DiffChild = DiffTag->UnorderedChildren;

            while (Child && DiffChild)
            {
                if (Child->Type > DiffChild->Type)
                {
                    HTMLDiffInsertOne(Writer, Tag, Child);
                    DiffChild = DiffChild->Next;
                }
                else if (Child->Type < DiffChild->Type)
                {
                    HTMLDiffDeleteOne(Writer, DiffChild);
                    Child = Child->Next;
                }
                else
                {
                    if (!Str8Match(Child->Content, DiffChild->Content, 0))
                    {
                        HTMLDiffReplaceContent(Writer, DiffChild, Child);
                    }
                    Child = Child->Next;
                    DiffChild = DiffChild->Next;
                }
            }

            if (Child)
            {
                HTMLDiffInsertAll(Writer, Tag, Child);
            }
            else if (DiffChild)
            {
                HTMLDiffDeleteAll(Writer, DiffChild);
            }
        }
    }

    Writer->TagStack[Writer->StackIndex] = 0;
    
    if (Writer->DiffRoot)
    {
        Writer->DiffTagStack[Writer->StackIndex] = Writer->DiffTagStack[Writer->StackIndex]->Next;
        if (Writer->DiffTagStack[Writer->StackIndex] == 0)
        {
            Writer->DiffTagStack[Writer->StackIndex] = &DiffTerminator;
        }

        for (i32 I = Writer->StackIndex + 1; I < HtmlMaxTagDepth; I++)
        {
            if (Writer->DiffTagStack[I] != 0)
            {
                if (Writer->DiffTagStack[I] != &DiffTerminator)
                {
                    HTMLDiffDeleteAll(Writer, Writer->DiffTagStack[I]);
                }
                Writer->DiffTagStack[I] = 0;
            }
        }
    }
    
    Writer->StackIndex--;

	return Writer->TagStack[Writer->StackIndex];
}

html_node * HTMLText(html_writer * Writer, str8 Text)
{
    Writer->TagStack[Writer->StackIndex]->Content = Text;
}

html_node * HTMLTextCStr(html_writer * Writer, char * CStr)
{
    Writer->TagStack[Writer->StackIndex]->Content = Str8FromCStr(CStr);
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

    HTMLAppendTagUnordered(Writer, Node);
}

html_node * HTMLStyle(html_writer * Writer, html_node_type * Style, str8 Value)
{
    html_node * Node = ArenaPushZero(Writer->Arena, html_node);
    Node->Type = Style;
    Node->Content = Value;

    HTMLAppendTagUnordered(Writer, Node);
}

html_diff * HTMLDiffDeleteOne(html_writer * Writer, html_node * OldTag)
{
    StdOutputFmt("Delete a tag of type %{str8}\r\n", OldTag->Type->Name);
}

html_diff * HTMLDiffDeleteAll(html_writer * Writer, html_node * OldTags)
{
    do {
        HTMLDiffDeleteOne(Writer, OldTags);
    } while (OldTags = OldTags->Next);
}

html_diff * HTMLDiffInsertOne(html_writer * Writer, html_node * Parent, html_node * NewTag)
{
    StdOutputFmt("Insert a tag of type %{str8}\r\n", NewTag->Type->Name);
}

html_diff * HTMLDiffInsertAll(html_writer * Writer, html_node * Parent, html_node * NewTags)
{
    do {
        HTMLDiffInsertOne(Writer, Parent, NewTags);
    } while (NewTags = NewTags->Next);
}

html_diff * HTMLDiffReplace(html_writer * Writer, html_node * OldTag, html_node * NewTag)
{
    StdOutputFmt("Replace a tag of type %{str8} with a tag of type %{str8}\r\n", OldTag->Type->Name, NewTag->Type->Name);
}

html_diff * HTMLDiffReplaceContent(html_writer * Writer, html_node * OldTag, html_node * NewTag)
{
    StdOutputFmt("Replace content of a tag of type %{str8}\r\n", NewTag->Type->Name);
}