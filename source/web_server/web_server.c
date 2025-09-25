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

void InitAssetPage(memory_arena * Arena, hash_table * HashTable, char * FilePath, str8 URL)
{
    asset * Asset = ArenaPush(Arena, asset);
    Asset->Data = FileInputFilename(FilePath, Arena);
    Asset->MimeType = HTTPMimeType_PNG;

    HashTableInsert(HashTable, URL, Asset);
}

hash_table * HashTable;

void InitAssetPages(memory_arena * Arena)
{
    HashTable = HashTableCreate(Arena, 64);

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
            asset * Asset = HashTableGet(HashTable, Request->Path);
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