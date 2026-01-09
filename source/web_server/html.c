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

html_node_ref HTMLNodeRefFromNode(html_node * Node)
{
    return (html_node_ref)
    {
		.Node = Node,
        .Key = Node->Key,
        .Id = Node->Id, // (id should not actually be populated at this point)
        .Type = Node->Type
	};
}

void HTMLWriterInit(html_writer * Writer)
{
    html_node * Root = ArenaPushZero(Writer->Arena, html_node);
    Root->Type = HTMLTag_html;

    Writer->DocumentRoot = Root;
    Writer->TagStack[0] = HTMLNodeRefFromNode(Root);

    if (Writer->DiffRoot)
    {
        Writer->DiffTagStack[0] = Writer->DiffRoot;
    }
}

// todo: we should try to move away from this, and just have the writer be smarter somehow...
void HTMLWriterReset(html_writer * Writer, u32 LastId)
{
    // Writer->Arena = Writer->Arena;
    Writer->DocumentRoot = 0;
    Writer->StackIndex = 0;
    for (i32 I = 0; I < HtmlMaxTagDepth; I++)
    {
        Writer->TagStack[I] = (html_node_ref) { 0 };
        Writer->DiffTagStack[I] = 0;
    }

    HTMLWriterInit(Writer);

    // Writer->DiffRoot = Writer->DiffRoot;
    Writer->Diffs = 0;
    Writer->DiffsEnd = 0;
    Writer->DiffVersion++;

    Writer->LastId = LastId;
    Writer->Replacing = false;

    Writer->Error = 0;
    Writer->ErrorMessage = Str8Empty();
}

html_writer HTMLWriterCreate(memory_arena * Arena, html_node * DiffRoot, u32 LastId)
{
    html_writer Writer = { 0 };
    Writer.Arena = Arena;
    Writer.DiffRoot = DiffRoot;
    Writer.DiffVersion = 1;

    HTMLWriterInit(&Writer);

    Writer.LastId = LastId;

    HTMLDiffTerminator.Children = &HTMLDiffTerminator;

	return Writer;
}

void HTMLAppendTagOrdered(html_writer * Writer, html_node_ref Child)
{
    html_node_ref * ParentRef = &Writer->TagStack[Writer->StackIndex];
    html_node * LastChild = ParentRef->Node->Children;

    while (LastChild && LastChild->Next)
    {
		LastChild = LastChild->Next;
    }

    if (ParentRef->Node != &HTMLMemorylessPlaceholder)
    {
        html_node * ParentNode = ParentRef->Node;
        html_node * ChildNode = Child.Node;

        if (LastChild)
        {
            LastChild->Next = ChildNode;
        }
        else
        {
            ParentNode->Children = ChildNode;
        }

        ChildNode->Index = ParentNode->ChildCount++; // todo: store something on the writer, so can put the index on the html_node_ref
        ChildNode->Next = 0;
    }

    Writer->StackIndex++;
    Writer->TagStack[Writer->StackIndex] = Child;
    html_node_ref * ChildPtr = &Writer->TagStack[Writer->StackIndex]; // todo: redo this

    HTMLDiffOnStartNode(Writer, ChildPtr, ParentRef);

    if (ChildPtr->Id == 0)
    {
        ChildPtr->Id = ++Writer->LastId;
        if (ChildPtr->Node != &HTMLMemorylessPlaceholder)
        {
            ChildPtr->Node->Id = ChildPtr->Id;
        }
    }
}

void HTMLAppendTagUnordered(html_writer * Writer, html_node * Child)
{
    if (Writer->Memoryless)
    {
        return;
    }

    html_node * Parent = Writer->TagStack[Writer->StackIndex].Node;

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
    html_node_ref Ref;
    if (!Writer->Memoryless)
    {
        html_node * Node = ArenaPushZero(Writer->Arena, html_node);
        Node->Type = Type;
        Node->Key = Key;

        Ref = HTMLNodeRefFromNode(Node);
    }
    else
    {
        Ref = (html_node_ref)
        {
            .Node = &HTMLMemorylessPlaceholder,
            .Type = Type,
            .Key = Key
        };
    }

    HTMLAppendTagOrdered(Writer, Ref);

    return Ref.Node;
}

html_node * HTMLSingleTagKey(html_writer * Writer, html_node_type * Type, u64 Key)
{
    html_node * Result = HTMLStartTagKey(Writer, Type, Key);
    HTMLEndTag(Writer);
    return Result;
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

    Writer->TagStack[Writer->StackIndex] = (html_node_ref) { 0 };
    Writer->StackIndex--;

    if (Writer->StackIndex < Writer->Replacing)
    {
        Writer->Replacing = 0;
    }

	return Writer->TagStack[Writer->StackIndex].Node;
}

html_node * HTMLText(html_writer * Writer, str8 Text)
{
    html_node_ref Ref = Writer->TagStack[Writer->StackIndex];
    if (Ref.Node && Ref.Node != &HTMLMemorylessPlaceholder)
    {
        Ref.Node->Content = Text;
        return Ref.Node;
    }
    return 0;
}

html_node * HTMLTextCStr(html_writer * Writer, char * CStr)
{
    HTMLText(Writer, Str8FromCStr(CStr));
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
    
    if (Node == &HTMLDiffTerminator) return Node;

    while (Writer->DiffTagStack[Writer->StackIndex] && Writer->DiffTagStack[Writer->StackIndex]->Key)
    {
        Writer->DiffTagStack[Writer->StackIndex] = Writer->DiffTagStack[Writer->StackIndex]->Next;
    }
    if (Writer->DiffTagStack[Writer->StackIndex] == 0)
    {
        Writer->DiffTagStack[Writer->StackIndex] = &HTMLDiffTerminator;
    }

    return Writer->DiffTagStack[Writer->StackIndex];
}

html_node * HTMLDiffGetOffset(html_writer * Writer, i32 Offset)
{
    i32 Cursor = Offset - 1;
    html_node_ref CurrentNodeRef;
    do
    {
        Cursor++;
        CurrentNodeRef = Writer->TagStack[Writer->StackIndex - Cursor];
    } while (CurrentNodeRef.Key);

    html_node * DiffNode = Writer->DiffTagStack[Writer->StackIndex - Cursor];
    Cursor--;
    while (Cursor > 0)
    {
        u64 Key = Writer->TagStack[Writer->StackIndex - Cursor].Key;
        DiffNode = HTMLDiffFindKey(DiffNode, Key);
        Cursor--;
    }

    return DiffNode;
}

void HTMLDiffOnStartNode(html_writer * Writer, html_node_ref * Node, html_node_ref * Parent)
{
    if (Writer->DiffRoot)
    {
        html_node * ParentDiffNode = HTMLDiffGetOffset(Writer, 1);
        html_node * DiffNode = 0;

        if (Node->Key)
        {
            DiffNode = HTMLDiffFindKey(ParentDiffNode, Node->Key);
            if (DiffNode)
            {
                DiffNode->UsedInDiff = Writer->DiffVersion;
            }
        }
        else
        {
            if (Writer->DiffTagStack[Writer->StackIndex] == 0)
            {
                HTMLDiffPushNode(Writer, ParentDiffNode ? ParentDiffNode->Children : 0);
            }
            DiffNode = Writer->DiffTagStack[Writer->StackIndex];
        }

        if (DiffNode == &HTMLDiffTerminator || DiffNode == 0)
        {
            HTMLDiffInsert(Writer, Parent->Node, Node->Node);
        }
        else if (DiffNode->Type != Node->Type)
        {
            HTMLDiffReplace(Writer, DiffNode, Node->Node);
            Writer->Replacing = Writer->StackIndex;
        }
        else
        {
            Node->Id = DiffNode->Id;
            if (Node->Node != &HTMLMemorylessPlaceholder)
            {
                Node->Node->Id = Node->Id;
            }

            if (Node->Key && Node->Node->Index != DiffNode->Index)
            {
                HTMLDiffMove(Writer, Node->Node, DiffNode);
            }
        }
    }
}

void HTMLDiffOnEndNode(html_writer * Writer)
{
    if (Writer->DiffRoot)
    {
        html_node_ref TagRef = Writer->TagStack[Writer->StackIndex];
        html_node * DiffTagNode = HTMLDiffGetOffset(Writer, 0);

        if (DiffTagNode != 0 && DiffTagNode != &HTMLDiffTerminator && TagRef.Type == DiffTagNode->Type)
        {
            // delete any children from the old tag that are not on the new one
            html_node * DiffChild = DiffTagNode->Children;
            html_node * DiffChildEnd = Writer->DiffTagStack[Writer->StackIndex + 1];

            for (DiffChild; DiffChild && DiffChild != DiffChildEnd; DiffChild = DiffChild->Next)
            {
                if (DiffChild->Key && DiffChild->UsedInDiff != Writer->DiffVersion)
                {
                    HTMLDiffDelete(Writer, DiffChild);
                }
            }
            for (DiffChild; DiffChild; DiffChild = DiffChild->Next)
            {
                if (!DiffChild->Key || DiffChild->UsedInDiff != Writer->DiffVersion)
                {
                    HTMLDiffDelete(Writer, DiffChild);
                }
            }

            if (TagRef.Node != &HTMLMemorylessPlaceholder)
            {
                html_node * TagNode = TagRef.Node;

                // compare content
                if (!Str8Match(TagNode->Content, DiffTagNode->Content, 0))
                {
                    HTMLDiffReplaceContent(Writer, DiffTagNode, TagNode);
                }

                // compare unordered items
                html_node * AttrChild = TagNode->UnorderedChildren;
                html_node * AttrDiffChild = DiffTagNode->UnorderedChildren;

                while (AttrChild && AttrDiffChild)
                {
                    if (AttrChild->Type > AttrDiffChild->Type)
                    {
                        HTMLDiffAttrSet(Writer, TagRef.Node, AttrChild);
                        AttrDiffChild = AttrDiffChild->Next;
                    }
                    else if (AttrChild->Type < AttrDiffChild->Type)
                    {
                        HTMLDiffAttrDelete(Writer, TagRef.Node, AttrDiffChild);
                        AttrChild = AttrChild->Next;
                    }
                    else
                    {
                        if (!Str8Match(AttrChild->Content, AttrDiffChild->Content, 0))
                        {
                            HTMLDiffAttrSet(Writer, TagRef.Node, AttrChild);
                        }
                        AttrChild = AttrChild->Next;
                        AttrDiffChild = AttrDiffChild->Next;
                    }
                }

                if (AttrChild)
                {
                    HTMLDiffAttrSetRest(Writer, TagRef.Node, AttrChild);
                }
                else if (AttrDiffChild)
                {
                    HTMLDiffAttrDeleteRest(Writer, TagRef.Node, AttrDiffChild);
                }
            }
        }

        HTMLDiffPushNode(Writer,
            Writer->DiffTagStack[Writer->StackIndex] ? Writer->DiffTagStack[Writer->StackIndex]->Next : 0);

        html_node * LastChild = Writer->DiffTagStack[Writer->StackIndex - 1];
        Writer->DiffTagStack[Writer->StackIndex + 1] = 0;
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
    if (OldTag == &HTMLMemorylessPlaceholder)
    {
        return;
    }

    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_Delete | OldTag->Type->Class,
        .Old = OldTag
    });
    StdOutputFmt("Delete a tag of type %{str8}\r\n", OldTag->Type->Name);
}

html_diff * HTMLDiffInsert(html_writer * Writer, html_node * Parent, html_node * NewTag)
{
    if (Parent == &HTMLMemorylessPlaceholder || NewTag == &HTMLMemorylessPlaceholder)
    {
        return;
    }

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
    if (OldTag == &HTMLMemorylessPlaceholder || NewTag == &HTMLMemorylessPlaceholder)
    {
        return;
    }

    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_Replace | NewTag->Type->Class,
        .New = NewTag,
        .Old = OldTag
    });
    StdOutputFmt("Replace a tag of type %{str8} with a tag of type %{str8}\r\n", OldTag->Type->Name, NewTag->Type->Name);
}

html_diff * HTMLDiffReplaceContent(html_writer * Writer, html_node * OldTag, html_node * NewTag)
{
    if (OldTag == &HTMLMemorylessPlaceholder || NewTag == &HTMLMemorylessPlaceholder)
    {
        return;
    }

    HTMLDiffAppend(Writer, (html_diff) {
        .Type = HTMLDiff_ReplaceContent | OldTag->Type->Class,
        .New = NewTag,
        .Old = OldTag
    });
    StdOutputFmt("Replace content of a tag of type %{str8}\r\n", NewTag->Type->Name);
}

html_diff * HTMLDiffMove(html_writer * Writer, html_node * OldTag, html_node * NewTag)
{
    if (OldTag == &HTMLMemorylessPlaceholder || NewTag == &HTMLMemorylessPlaceholder)
    {
        return;
    }

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
    if (Parent == &HTMLMemorylessPlaceholder || OldTag == &HTMLMemorylessPlaceholder)
    {
        return;
    }

    HTMLDiffAppend(Writer, (html_diff) {
        .Type = OldTag->Type->Class,
        .Old = OldTag,
        .Parent = Parent
    });
    StdOutputFmt("Delete an attribute of type %{str8}\r\n", OldTag->Type->Name);
}

html_diff * HTMLDiffAttrDeleteRest(html_writer * Writer, html_node * Parent, html_node * OldTags)
{
    if (Parent == &HTMLMemorylessPlaceholder || OldTags == &HTMLMemorylessPlaceholder)
    {
        return;
    }

    do {
        HTMLDiffAttrDelete(Writer, Parent, OldTags);
    } while (OldTags = OldTags->Next);
}

html_diff * HTMLDiffAttrSet(html_writer * Writer, html_node * Parent, html_node * NewTag)
{
    if (Parent == &HTMLMemorylessPlaceholder || NewTag == &HTMLMemorylessPlaceholder)
    {
        return;
    }

    HTMLDiffAppend(Writer, (html_diff) {
        .Type = NewTag->Type->Class,
        .New = NewTag,
        .Parent = Parent
    });
    StdOutputFmt("Insert an attribute of type %{str8}\r\n", NewTag->Type->Name);
}

html_diff * HTMLDiffAttrSetRest(html_writer * Writer, html_node * Parent, html_node * NewTags)
{
    if (Parent == &HTMLMemorylessPlaceholder || NewTags == &HTMLMemorylessPlaceholder)
    {
        return;
    }

    do {
        HTMLDiffAttrSet(Writer, Parent, NewTags);
    } while (NewTags = NewTags->Next);
}
