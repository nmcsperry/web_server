#ifndef web_server_html_h
#define web_server_html_h

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
HTMLTagDef(img);

HTMLAttrDef(src);

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

typedef struct html_writer
{
	memory_arena * Arena;
    html_node * DocumentRoot;
    html_node * CurrentTag;
	html_node * LastNode;

    html_node * TagStack[HtmlMaxTagDepth];
    u32 StackIndex;

    bool32 Error;
	str8 ErrorMessage;
} html_writer;

str8 Str8FromHTML(memory_arena * Arena, html_node * Nodes);

html_writer HTMLWriterCreate(memory_arena * Arena);
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

#endif