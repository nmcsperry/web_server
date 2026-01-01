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
        Str8WriteFmt(Buffer, " id=\'%{u32}\'", Node->Id);

        for (html_node * Child = Node->UnorderedChildren; Child; Child = Child->Next)
        {
            if (Child->Type->Class != HtmlNodeClass_Attr) continue;
            Str8WriteFmt(Buffer, " %{str8}=\'%{str8}\'", Child->Type->Name, Child->Content);
        }

		bool32 FirstStyle = true;
        for (html_node * Child = Node->UnorderedChildren; Child; Child = Child->Next)
        {
            if (Child->Type->Class != HtmlNodeClass_Style) continue;
            if (FirstStyle)
            {
                Str8WriteFmt(Buffer, " style=\'");
                FirstStyle = false;
			}
            Str8WriteFmt(Buffer, "%{str8}:%{str8};", Child->Type->Name, Child->Content);
        }
        if (!FirstStyle)
        {
            Str8WriteFmt(Buffer, "\'");
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

str8 Str8FromHTMLDiff(memory_arena * Arena, html_diff * Deltas)
{
    memory_buffer * Buffer = ScratchBufferStart();

    html_diff * Delta = Deltas;
    Str8WriteFmt(Buffer, "[");

    bool32 FirstLoop = true;
    while (Delta)
    {
        if (FirstLoop)
        {
            FirstLoop = false;
        }
        else
        {
            Str8WriteFmt(Buffer, ", ");
        }

        Str8WriteFmt(Buffer, " { \"Type\": %{u32}, ", Delta->Type);

        if ((Delta->Type & HTMLDiff_TypeMask) == HTMLDiff_Tag)
        {
            if ((Delta->Type & HTMLDiff_OperationMask) == HTMLDiff_Insert)
            {
                Str8WriteFmt(Buffer, "\"ID\": %{u32}, \"Content\": \"%{str8}\", \"Index\": %{u32} }",
                    Delta->Parent->Id, Str8FromHTML(Arena, Delta->New), Delta->Index);
            }
            else if ((Delta->Type & HTMLDiff_OperationMask) == HTMLDiff_Delete)
            {
                Str8WriteFmt(Buffer, "\"ID\": %{u32} }", Delta->Old->Id);
            }
            else if ((Delta->Type & HTMLDiff_OperationMask) == HTMLDiff_Replace)
            {
                Str8WriteFmt(Buffer, "\"ID\": %{u32}, \"Content\": \"%{str8}\" }",
                    Delta->Old->Id, Str8FromHTML(Arena, Delta->New));
            }
            else if ((Delta->Type & HTMLDiff_OperationMask) == HTMLDiff_Move)
            {
                Str8WriteFmt(Buffer, "\"ID\": %{u32}, \"Index\": %{u32} }",
                    Delta->Old->Id, Delta->Index);
            }
            else if ((Delta->Type & HTMLDiff_OperationMask) == HTMLDiff_ReplaceContent)
            {
                Str8WriteFmt(Buffer, "\"ID\": %{u32}, \"Content\": \"%{str8}\" }",
                    Delta->Old->Id, Delta->New->Content);
            }
        }
        else if ((Delta->Type & HTMLDiff_TypeMask) == HTMLDiff_Attr || (Delta->Type & HTMLDiff_TypeMask) == HTMLDiff_Style)
        {
            if (Delta->New)
            {
                Str8WriteFmt(Buffer, "\"ID\": %{u32}, \"AttrName\": \"%{str8}\", \"Content\": \"%{str8}\" }",
                    Delta->Parent->Id, Delta->New->Type->JavaScriptName, Delta->New->Content);
            }
            else if (Delta->Old)
            {
                Str8WriteFmt(Buffer, "\"ID\": %{u32}, \"AttrName\": \"%{str8}\", \"Content\": null }",
                    Delta->Parent->Id, Delta->Old->Type->JavaScriptName);
            }
        }

        Delta = Delta->Next;
    }
    Str8WriteFmt(Buffer, " ]");

    return ScratchBufferEndStr8(Buffer, Arena);
}

html_writer HTMLWriterCreate(memory_arena * Arena, html_node * DiffRoot, u32 LastId)
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

        Root->DiffNode = DiffRoot;
        DiffRoot->DiffNode = &DiffComplete; // todo: is this a good way to do this?

        // todo: maybe we should check if they are the same type, although we can assume they are...
    }

    Writer.LastId = LastId;

    DiffTerminator.Children = &DiffTerminator;

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

    Child->Index = Parent->ChildCount++;

    Writer->StackIndex++;
    Writer->TagStack[Writer->StackIndex] = Child;

    HTMLDiffOnStartNode(Writer, Child, Parent);

    if (Child->Id == 0)
    {
        Child->Id = ++Writer->LastId;
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

    HTMLDiffOnEndNode(Writer);

    Writer->TagStack[Writer->StackIndex] = 0;
    Writer->StackIndex--;

    if (Writer->StackIndex < Writer->Replacing)
    {
        Writer->Replacing = 0;
    }

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

html_node * HTMLDiffFindKey(html_node * Node, u64 Key)
{
    html_node * Candidate = Node->Children;
    while (Candidate)
    {
        if (Candidate->Key == Key)
        {
            return Candidate;
        }
        Candidate = Candidate->Next;
    }

    return 0;
}

void HTMLDiffPushNode(html_writer * Writer, html_node * Node)
{
    Writer->DiffTagStack[Writer->StackIndex] = Node;
    
    if (Node == &DiffTerminator) return Node;

    while (Writer->DiffTagStack[Writer->StackIndex] && Writer->DiffTagStack[Writer->StackIndex]->Key)
    {
        Writer->DiffTagStack[Writer->StackIndex] = Writer->DiffTagStack[Writer->StackIndex]->Next;
    }
    if (Writer->DiffTagStack[Writer->StackIndex] == 0)
    {
        Writer->DiffTagStack[Writer->StackIndex] = &DiffTerminator;
    }

    return Writer->DiffTagStack[Writer->StackIndex];
}

void HTMLDiffOnStartNode(html_writer * Writer, html_node * Node, html_node * Parent)
{
    if (Writer->DiffRoot)
    {
        html_node * CompareNode = 0;

        if (Node->Key)
        {
            CompareNode = HTMLDiffFindKey(Parent->DiffNode, Node->Key);
        }
        else
        {
            if (Writer->DiffTagStack[Writer->StackIndex] == 0)
            {
                HTMLDiffPushNode(Writer, Parent->DiffNode ? Parent->DiffNode->Children : 0);
            }

            CompareNode = Writer->DiffTagStack[Writer->StackIndex];
        }

        if (CompareNode == &DiffTerminator || CompareNode == 0)
        {
            HTMLDiffInsert(Writer, Parent, Node);
        }
        else if (CompareNode->Type != Node->Type)
        {
            HTMLDiffReplace(Writer, CompareNode, Node);
            Writer->Replacing = Writer->StackIndex;

            CompareNode->DiffNode = &DiffComplete;
        }
        else
        {
            Node->Id = CompareNode->Id;

            Node->DiffNode = CompareNode;
            CompareNode->DiffNode = &DiffComplete; // todo: is this a good way to do this?

            if (Node->Key && Node->Index != CompareNode->DiffNode)
            {
                HTMLDiffMove(Writer, Node, CompareNode);
            }
        }
    }
}

void HTMLDiffOnEndNode(html_writer * Writer)
{
    if (Writer->DiffRoot)
    {
        html_node * Tag = Writer->TagStack[Writer->StackIndex];
        html_node * DiffTag = Tag->DiffNode;

        if (DiffTag != 0 && DiffTag != &DiffTerminator && Tag->Type == DiffTag->Type)
        {
            // delete any unmatched children
            for (html_node * Child = DiffTag->Children; Child; Child = Child->Next)
            {
                if (Child->DiffNode != &DiffComplete)
                {
                    HTMLDiffDelete(Writer, Child);
                }
            }

            // compare content
            if (!Str8Match(Tag->Content, DiffTag->Content, 0))
            {
                HTMLDiffReplaceContent(Writer, DiffTag, Tag);
            }

            // compare unordered items
            html_node * AttrChild = Tag->UnorderedChildren;
            html_node * AttrDiffChild = DiffTag->UnorderedChildren;

            while (AttrChild && AttrDiffChild)
            {
                if (AttrChild->Type > AttrDiffChild->Type)
                {
                    HTMLDiffAttrSet(Writer, Tag, AttrChild);
                    AttrDiffChild = AttrDiffChild->Next;
                }
                else if (AttrChild->Type < AttrDiffChild->Type)
                {
                    HTMLDiffAttrDelete(Writer, Tag, AttrDiffChild);
                    AttrChild = AttrChild->Next;
                }
                else
                {
                    if (!Str8Match(AttrChild->Content, AttrDiffChild->Content, 0))
                    {
                        HTMLDiffAttrSet(Writer, Tag, AttrChild);
                    }
                    AttrChild = AttrChild->Next;
                    AttrDiffChild = AttrDiffChild->Next;
                }
            }

            if (AttrChild)
            {
                HTMLDiffAttrSetRest(Writer, Tag, AttrChild);
            }
            else if (AttrDiffChild)
            {
                HTMLDiffAttrDeleteRest(Writer, Tag, AttrDiffChild);
            }
        }

        HTMLDiffPushNode(Writer,
            Writer->DiffTagStack[Writer->StackIndex] ? Writer->DiffTagStack[Writer->StackIndex]->Next : 0);

        for (i32 I = Writer->StackIndex + 1; I < HtmlMaxTagDepth; I++)
        {
            if (Writer->DiffTagStack[I] != 0)
            {
                Writer->DiffTagStack[I] = 0;
            }
        }
    }
}

html_diff * HTMLDiffAppend(html_writer * Writer, html_diff Diff)
{
    if (!Writer->Replacing)
    {
        html_diff * DiffPtr = ArenaPushAndCopy(Writer->Arena, html_diff, &Diff);
        SLLQueuePush(Writer->Diffs, Writer->DiffsEnd, DiffPtr);
    }
}

html_diff * HTMLDiffDelete(html_writer * Writer, html_node * OldTag)
{
    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_Delete | OldTag->Type->Class,
        .Old = OldTag
    });
    StdOutputFmt("Delete a tag of type %{str8}\r\n", OldTag->Type->Name);
}

html_diff * HTMLDiffInsert(html_writer * Writer, html_node * Parent, html_node * NewTag)
{
    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_Insert | NewTag->Type->Class,
        .New = NewTag,

        .Index = NewTag->Index,
        .Parent = Parent
    });
    StdOutputFmt("Insert a tag of type %{str8}\r\n", NewTag->Type->Name);
}

html_diff * HTMLDiffReplace(html_writer * Writer, html_node * OldTag, html_node * NewTag)
{
    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_Replace | NewTag->Type->Class,
        .New = NewTag,
        .Old = OldTag
    });
    StdOutputFmt("Replace a tag of type %{str8} with a tag of type %{str8}\r\n", OldTag->Type->Name, NewTag->Type->Name);
}

html_diff * HTMLDiffReplaceContent(html_writer * Writer, html_node * OldTag, html_node * NewTag)
{
    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_ReplaceContent | OldTag->Type->Class,
        .New = NewTag,
        .Old = OldTag
    });
    StdOutputFmt("Replace content of a tag of type %{str8}\r\n", NewTag->Type->Name);
}

html_diff * HTMLDiffMove(html_writer * Writer, html_node * OldTag, html_node * NewTag)
{
    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_Move | NewTag->Type->Class,
        .New = NewTag,
        .Old = OldTag,

        .Index = NewTag->Index
    });
    StdOutputFmt("Move a tag of type %{str8}\r\n", NewTag->Type->Name);
}

html_diff * HTMLDiffAttrDelete(html_writer * Writer, html_node * Parent, html_node * OldTag)
{
    HTMLDiffAppend(Writer, (html_diff) {
        .Type = OldTag->Type->Class,
        .Old = OldTag,
        .Parent = Parent
    });
    StdOutputFmt("Delete an attribute of type %{str8}\r\n", OldTag->Type->Name);
}

html_diff * HTMLDiffAttrDeleteRest(html_writer * Writer, html_node * Parent, html_node * OldTags)
{
    do {
        HTMLDiffAttrDelete(Writer, Parent, OldTags);
    } while (OldTags = OldTags->Next);
}

html_diff * HTMLDiffAttrSet(html_writer * Writer, html_node * Parent, html_node * NewTag)
{
    HTMLDiffAppend(Writer, (html_diff) {
        .Type = NewTag->Type->Class,
        .New = NewTag,
        .Parent = Parent
    });
    StdOutputFmt("Insert an attribute of type %{str8}\r\n", NewTag->Type->Name);
}

html_diff * HTMLDiffAttrSetRest(html_writer * Writer, html_node * Parent, html_node * NewTags)
{
    do {
        HTMLDiffAttrSet(Writer, Parent, NewTags);
    } while (NewTags = NewTags->Next);
}
