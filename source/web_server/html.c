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

#define HTMLTagDef(Name) html_node_type HTMLTag_##Name##_Value = { Str8LitInit(#Name), Str8LitInit(#Name), HtmlNodeClass_Tag }, * HTMLTag_##Name = &HTMLTag_##Name##_Value
#define HTMLAttrDef(Name) html_node_type HTMLAttr_##Name##_Value = { Str8LitInit(#Name), Str8LitInit(#Name), HtmlNodeClass_Attr }, * HTMLAttr_##Name = &HTMLAttr_##Name##_Value
#define HTMLStyleDef(Name, JSName) html_node_type HTMLStyle_##JSName##_Value = { Str8LitInit(#Name), Str8LitInit(#JSName), HtmlNodeClass_Style }, * HTMLStyle_##Name = &HTMLStyle_##Name##_Value

HTMLTagDef(html);
HTMLTagDef(body);
HTMLTagDef(head);
HTMLTagDef(title);
HTMLTagDef(p);

HTMLStyleDef(color, color);

#undef HTMLTagDef
#undef HTMLAttrDef
#undef HTMLStyleDef

typedef struct html_node {
    u64 Key;
    hash_value Hash;

    html_node_type * Type;

    str8 Content;

    u32 AttrCount;
    u32 StyleCount;
    u32 ChildTagCount;
} html_node;

#define HtmlMaxTagDepth 16

str8 HTMLFromHTMLNodes(memory_arena * Arena, html_node * Nodes, u32 Count)
{
    str8 * TagNameStack[HtmlMaxTagDepth] = { 0 };
    u32 TagCountStack[HtmlMaxTagDepth] = { 0 };
    u32 StackIndex = 0;

    memory_buffer * Buffer = ScratchBufferStart();

    u32 Index = 0;
    while (Index < Count)
    {
        html_node * Node = &Nodes[Index];
        u32 ChildrenStartIndex = Index + 1;

        Str8WriteFmt(Buffer, "<%{str8}", Node->Type->Name);

        Index++;

        if (Node->AttrCount)
        {
            for (; Index < ChildrenStartIndex + Node->AttrCount; Index++)
            {
                html_node * Node2 = &Nodes[Index];
                Str8WriteFmt(Buffer, " %{str8}=\"%{str8}\"", Node2->Type->Name, Node2->Content);
            }
        }

        if (Node->StyleCount)
        {
            Str8WriteFmt(Buffer, " style=\"");

            for (; Index < ChildrenStartIndex + Node->AttrCount + Node->StyleCount; Index++)
            {
                html_node * Node2 = &Nodes[Index];
                Str8WriteFmt(Buffer, "%{str8}:%{str8};", Node2->Type->Name, Node2->Content);
            }

            Str8WriteFmt(Buffer, "\"");
        }

        if (Node->ChildTagCount)
        {
            TagNameStack[StackIndex] = &Node->Type->Name;
            TagCountStack[StackIndex] = Node->ChildTagCount;

            Str8WriteFmt(Buffer, ">");

            StackIndex++;
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

            while (StackIndex && --TagCountStack[StackIndex - 1] == 0)
            {
                Str8WriteFmt(Buffer, "</%{str8}>", TagNameStack[StackIndex - 1]);

                TagNameStack[StackIndex - 1] = 0;
                StackIndex--;
            }
        }
    }

    return ScratchBufferEndStr8(Buffer, Arena);
}

str8 HTMLNodesTest(memory_arena * Arena)
{
    html_node Nodes[] = {
        (html_node) { .Type = HTMLTag_html, .Content = Str8Empty(), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 2 },
        (html_node) { .Type = HTMLTag_head, .Content = Str8Empty(), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 1 },
        (html_node) { .Type = HTMLTag_title, .Content = Str8Lit("HTML Builder"), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 0 },
        (html_node) { .Type = HTMLTag_body, .Content = Str8Empty(), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 2 },
        (html_node) { .Type = HTMLTag_p, .Content = Str8Lit("Red"), .AttrCount = 0, .StyleCount = 1, .ChildTagCount = 0 },
        (html_node) { .Type = HTMLStyle_color, .Content = Str8Lit("red"), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 0 },
        (html_node) { .Type = HTMLTag_p, .Content = Str8Lit("Blue"), .AttrCount = 0, .StyleCount = 1, .ChildTagCount = 0 },
        (html_node) { .Type = HTMLStyle_color, .Content = Str8Lit("blue"), .AttrCount = 0, .StyleCount = 0, .ChildTagCount = 0 },

    };

    return HTMLFromHTMLNodes(Arena, Nodes, 8);
}