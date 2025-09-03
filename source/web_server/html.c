#include "../reuse/base/base_include.h"

enum html_node_class
{
    HtmlNodeClass_Tag = 1,
    HtmlNodeClass_Attr,
    HtmlNodeClass_Style,
    HtmlNodeClass_Fragment
};

typedef struct html_node_type
{
    str8 Name;
    str8 JavaScriptName;

    u32 Class;
} html_node_type;

#define HTMLTagDef(Name) \
    html_node_type HTMLTag_##Name##_Value = { Str8LitInit(#Name), Str8LitInit(#Name), HtmlNodeClass_Tag }, \
    * HTMLTag_##Name = &HTMLTag_##Name##_Value
#define HTMLAttrDef(Name) \
    html_node_type HTMLAttr_##Name##_Value = { Str8LitInit(#Name), Str8LitInit(#Name), HtmlNodeClass_Attr }, \
    * HTMLAttr_##Name = &HTMLAttr_##Name##_Value
#define HTMLStyleDef(Name, JSName) \
    html_node_type HTMLStyle_##JSName##_Value = { Str8LitInit(#Name), Str8LitInit(#JSName), HtmlNodeClass_Style }, \
    * HTMLStyle_##Name = &HTMLStyle_##Name##_Value

HTMLTagDef(html);
HTMLTagDef(body);
HTMLTagDef(head);
HTMLTagDef(title);
HTMLTagDef(p);

HTMLStyleDef(color, color);

#undef HTMLTagDef
#undef HTMLAttrDef
#undef HTMLStyleDef

typedef struct html_node html_node;

struct html_node {
    u64 Key;
    hash_value Hash;

    html_node_type * Type;

    str8 Content;

    u32 AttrCount;
    u32 StyleCount;
    u32 ChildTagCount;

	html_node * Next;
};

#define HtmlMaxTagDepth 16

str8 HTMLFromHTMLNodes(memory_arena * Arena, html_node * Nodes)
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

str8 HTMLNodesTest(memory_arena * Arena)
{
    html_node HTML = { .Type = HTMLTag_html, .Content = Str8Empty(), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 2 };
    html_node Head = { .Type = HTMLTag_head, .Content = Str8Empty(), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 1 };
    html_node Title = { .Type = HTMLTag_title, .Content = Str8Lit("HTML Builder"), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 0 };
    html_node Body = { .Type = HTMLTag_body, .Content = Str8Empty(), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 2 };
    html_node P1 = { .Type = HTMLTag_p, .Content = Str8Lit("Red"), .AttrCount = 0, .StyleCount = 1, .ChildTagCount = 0 };
    html_node Color1 = { .Type = HTMLStyle_color, .Content = Str8Lit("red"), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 0 };
    html_node P2 = { .Type = HTMLTag_p, .Content = Str8Lit("Blue"), .AttrCount = 0, .StyleCount = 1, .ChildTagCount = 0 };
    html_node Color2 = { .Type = HTMLStyle_color, .Content = Str8Lit("blue"), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 0 };

	HTML.Next = &Head;
	Head.Next = &Title;
	Title.Next = &Body;
	Body.Next = &P1;
	P1.Next = &Color1;
	Color1.Next = &P2;
	P2.Next = &Color2;
	Color2.Next = 0;

    return HTMLFromHTMLNodes(Arena, &HTML);
}