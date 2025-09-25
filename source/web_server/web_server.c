#include "../reuse/base/base_include.h"
#include "../reuse/network/network_include.h"
#include "../reuse/io/io_include.h"

#include "../reuse/base/base_include.c"
#include "../reuse/network/network_include.c"
#include "../reuse/io/io_include.c"

#include "http_server.h"
#include "http_server.c"
#include "html.c"

str8 MainPage(memory_arena * Arena)
{
    html_writer Writer = HTMLWriterCreate(Arena);

    HTMLTag(&Writer, HTMLTag_head)
    {
        HTMLTag(&Writer, HTMLTag_title)
        {
            HTMLText(&Writer, Str8Lit("HTML Builder"));
        }
    }

    HTMLTag(&Writer, HTMLTag_body)
    {
        HTMLTag(&Writer, HTMLTag_p)
        {
            HTMLStyle(&Writer, HTMLStyle_color, Str8Lit("red"));
            HTMLText(&Writer, Str8Lit("Red"));
        }

        HTMLTag(&Writer, HTMLTag_p)
        {
            HTMLStyle(&Writer, HTMLStyle_color, Str8Lit("blue"));
            HTMLText(&Writer, Str8Lit("Blue"));
        }

        HTMLTag(&Writer, HTMLTag_img)
        {
            HTMLAttr(&Writer, HTMLAttr_src, Str8Lit("my_image.png"));
        }

        HTMLTag(&Writer, HTMLTag_img)
        {
            HTMLAttr(&Writer, HTMLAttr_src, Str8Lit("cool_s.png"));
        }
    }

    return Str8FromHTML(Arena, Writer.DocumentRoot);
}

typedef struct asset
{
    str8 Data;
    u32 MimeType;
} asset;

typedef struct asset_node asset_node;

struct asset_node
{
    str8 Key;
    hash_value Hash;

    asset Asset;
    asset_node * Next;
};

typedef struct asset_table {
    asset_node ** Data;
    u32 TableSize;
} asset_table;

asset * AssetPageGet(asset_table * HashTable, str8 URL)
{
    hash_value Hash = HashStr8(URL);
    u32 Slot = Hash % HashTable->TableSize;

    asset_node * AssetNode = HashTable->Data[Slot];
    for (AssetNode; AssetNode; AssetNode = AssetNode->Next)
    {
        if (AssetNode->Hash == Hash && Str8Match(AssetNode->Key, URL, 0))
        {
            return &AssetNode->Asset;
        }
    }

    return 0;
}

void InitAssetPage(memory_arena * Arena, asset_table * HashTable, char * FilePath, str8 URL)
{
    hash_value Hash = HashStr8(URL);
    u32 Slot = Hash % HashTable->TableSize;

    asset_node * AssetNode = HashTable->Data[Slot];

    if (AssetNode == 0)
    {
        AssetNode = ArenaPush(Arena, asset_node);
        HashTable->Data[Slot] = AssetNode;
        AssetNode->Next = 0;
    }
    else
    {
        asset_node * CurrentNode;
        for (CurrentNode = AssetNode; CurrentNode; CurrentNode = CurrentNode->Next)
        {
            if (CurrentNode->Hash == Hash && Str8Match(CurrentNode->Key, URL, 0))
            {
                break;
            }
        }

        if (CurrentNode == 0)
        {
            asset_node * NewNode = ArenaPush(Arena, asset_node);
            SLLStackPush(AssetNode, NewNode);
            AssetNode = NewNode;
        }
        else
        {
            // error
            AssetNode = 0;
        }
    }

    str8 Data = FileInputFilename(FilePath, Arena);
    AssetNode->Asset.Data = Data;
    AssetNode->Asset.MimeType = HTTPMimeType_PNG;
    AssetNode->Hash = Hash;
    AssetNode->Key = URL;
}

asset_table * HashTable = 0;

void InitAssetPages(memory_arena * Arena)
{
    HashTable = ArenaPush(Arena, asset_table);
    HashTable->TableSize = 64;
    HashTable->Data = ArenaPushArray(Arena, asset_node *, HashTable->TableSize);

    InitAssetPage(Arena, HashTable, "my_image.png", Str8Lit("/my_image.png"));
    InitAssetPage(Arena, HashTable, "cool_s.png", Str8Lit("/cool_s.png"));
}

void EntryHook()
{
	SocketInit();

    memory_arena * AssetArena = ArenaCreate(Megabytes(64));
    InitAssetPages(AssetArena);

	socket_handle Socket = SocketCreateServer(80, 50);
	http_server Server = ServerInit(Socket);

	while (true)
	{
		ServerLoop(&Server);

		http_request * Request = 0;
		while (Request = ServerNextRequest(&Server))
		{
            asset * Asset = AssetPageGet(HashTable, Request->Path);
            if (Asset)
            {
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseHTTPCode = 200;
                Request->ResponseMimeType = Asset->MimeType;
                Request->ResponseBody = Asset->Data;
            }
            else
            {
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseHTTPCode = 200;
                Request->ResponseMimeType = HTTPMimeType_HTML;
                Request->ResponseBody = MainPage(Server.ResponseArena);
            }
		}
	}
}