#ifndef web_server_html_h
#define web_server_html_h

#include "../reuse/base/base_include.h"

enum html_node_class
{
    HtmlNodeClass_Tag = 1,
    HtmlNodeClass_Attr = 2,
    HtmlNodeClass_Style = 3,
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
HTMLTagDef(h1);
HTMLTagDef(h2);
HTMLTagDef(h3);
HTMLTagDef(img);
HTMLTagDef(script);
HTMLTagDef(span);

HTMLAttrDef(src);
HTMLAttrDef(type);

HTMLStyleDef(color, color);

#undef HTMLTagDef
#undef HTMLAttrDef
#undef HTMLStyleDef

typedef struct html_node html_node;

struct html_node {
    u64 Key;
    u32 Id;
    u32 Index;

    html_node_type * Type;

    str8 Content;
    u32 ChildCount;

    html_node * UnorderedChildren;
    html_node * Children;
    html_node * Next;

    html_node * DiffNode;
};

#define HtmlMaxTagDepth 16

html_node DiffTerminator;

typedef struct html_diff html_diff;

enum html_diff_type {
    HTMLDiff_Insert = 0x1 << 4,
    HTMLDiff_Delete = 0x2 << 4,
    HTMLDiff_Replace = 0x3 << 4,
    HTMLDiff_Move = 0x7 << 4,
    HTMLDiff_ReplaceContent = 0x8 << 4,

    HTMLDiff_OperationMask = 0xf << 4,

    HTMLDiff_Tag = 0x1,
    HTMLDiff_Attr = 0x2,
    HTMLDiff_Style = 0x3,

    HTMLDiff_TypeMask = 0xf,
};

struct html_diff {
    u32 Type;

    html_node * Old;
    html_node * New;

    html_node * Parent;
    i32 Index;

    html_diff * Next;
};

typedef struct html_writer
{
	memory_arena * Arena;
    html_node * DocumentRoot;

    html_node * TagStack[HtmlMaxTagDepth];
    u32 StackIndex;

    html_node * DiffRoot;
    html_node * DiffTagStack[HtmlMaxTagDepth];
    html_diff * Diffs;
    html_diff * DiffsEnd;

    u32 LastId;
    u32 Replacing;

    bool32 Error;
	str8 ErrorMessage;
} html_writer;

str8 Str8FromHTML(memory_arena * Arena, html_node * Nodes);
str8 Str8FromHTMLDiff(memory_arena * Arena, html_diff * Deltas);

html_writer HTMLWriterCreate(memory_arena * Arena, html_node * DiffRoot, u32 LastId);
html_node * HTMLStartTag(html_writer * Writer, html_node_type * Type);
html_node * HTMLStartTagKey(html_writer * Writer, html_node_type * Type, u64 Key);
html_node * HTMLEndTag(html_writer * Writer);
html_node * HTMLSingleTag(html_writer * Writer, html_node_type * Type);
html_node * HTMLSingleTagKey(html_writer * Writer, html_node_type * Type, u64 Key);
html_node * HTMLText(html_writer * Writer, str8 Text);
html_node * HTMLTextFmt(html_writer * Writer, char * Format, ...);
html_node * HTMLAttr(html_writer * Writer, html_node_type * Attr, str8 Value);
html_node * HTMLStyle(html_writer * Writer, html_node_type * Style, str8 Value);

#define HTMLTag(Writer, Type) DeferLoop(HTMLStartTag(Writer, Type), HTMLEndTag(Writer))
#define HTMLTagKey(Writer, Type, Key) DeferLoop(HTMLStartTagKey(Writer, Type, Key), HTMLEndTag(Writer))

void HTMLDiffOnStartNode(html_writer * Writer, html_node * Node, html_node * Parent);
void HTMLDiffOnEndNode(html_writer * Writer);

html_diff * HTMLDiffDeleteOne(html_writer * Writer, html_node * Parent, html_node * OldTag);
html_diff * HTMLDiffDeleteAll(html_writer * Writer, html_node * Parent, html_node * OldTags);
html_diff * HTMLDiffInsertOne(html_writer * Writer, html_node * Parent, html_node * NewTag);
html_diff * HTMLDiffInsertAll(html_writer * Writer, html_node * Parent, html_node * NewTags);

html_diff * HTMLDiffReplace(html_writer * Writer, html_node * OldTag, html_node * NewTag);
html_diff * HTMLDiffReplaceContent(html_writer * Writer, html_node * OldTag, html_node * NewTag);
html_diff * HTMLDiffMove(html_writer * Writer, html_node * OldTag, html_node * NewTag);

#endif