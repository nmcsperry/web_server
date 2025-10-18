#include "../reuse/base/base_include.h"
#include "../reuse/network/network_include.h"
#include "../reuse/io/io_include.h"

#include "../reuse/base/base_include.c"
#include "../reuse/network/network_include.c"
#include "../reuse/io/io_include.c"

#include "http_server.h"
#include "http_server.c"
#include "html.c"

str8 NotFoundPage(http_server * Server, memory_arena * Arena)
{
    html_writer Writer = HTMLWriterCreate(Arena);

    HTMLTag(&Writer, HTMLTag_head)
    {
        HTMLTag(&Writer, HTMLTag_title)
        {
            HTMLText(&Writer, Str8Lit("404 Not Found"));
        }
    }

    HTMLTag(&Writer, HTMLTag_body)
    {
        HTMLSimpleTagCStr(&Writer, HTMLTag_p, "Requested Page Not Found");
    }

    return Str8FromHTML(Arena, Writer.DocumentRoot);
}

str8 MainPage(http_server * Server, memory_arena * Arena)
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
        for (u32 I = 0; I < ArrayCount(Server->Connections); I++)
        {
            http_connection_slot * Connection = &Server->Connections[I];

            if (Connection->Value.Valid)
            {
                HTMLSimpleTagCStr(&Writer, HTMLTag_h3, "Connection");
                HTMLSimpleTag(&Writer, HTMLTag_p, Str8FromIPAddr(Writer.Arena, Connection->Value.Address));
                datetime Datetime = DatetimeFromUnixTimeSec(Connection->Value.FirstCommunication);
                HTMLSimpleTag(&Writer, HTMLTag_p, Str8FromDatetime(Writer.Arena, Datetime));

                temp_memory_arena Scratch = GetScratchArena(0, 0);
                str8 RequestPathHistoryString = Str8FromStr8LL(Arena, Connection->Value.RequestPathHistory);
                HTMLSimpleTag(&Writer, HTMLTag_p, RequestPathHistoryString);
                ReleaseScratchArena(Scratch);

                HTMLSimpleTagFmt(&Writer, HTMLTag_p, "%{u32}", Connection->Value.RequestsReceived);
            }
        }
    }

    return Str8FromHTML(Arena, Writer.DocumentRoot);
}

typedef struct asset
{
    str8 Data;
    mime_type * MimeType;
} asset;

void InitAssetPage(memory_arena * Arena, hash_table * HashTable, char * FilePath, str8 URL, mime_type * MimeType)
{
    str8 Extension = Str8CutFindFromEnd(URL, Str8Lit(".")).Second;

    asset * Asset = ArenaPush(Arena, asset);
    Asset->Data = FileInputFilename(FilePath, Arena);
    Asset->MimeType = MimeType;

    HashTableInsert(HashTable, URL, Asset);
}

hash_table * AssetHashTable;

void InitAssetPages(memory_arena * Arena)
{
    AssetHashTable = HashTableCreate(Arena, 64);

    InitAssetPage(Arena, AssetHashTable, "my_image.png", Str8Lit("/my_image.png"), &MimeType_PNG);
    InitAssetPage(Arena, AssetHashTable, "favicon.ico", Str8Lit("/favicon.ico"), &MimeType_ICO);
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
            asset * Asset = HashTableGet(AssetHashTable, Request->RequestPath);
            if (Asset)
            {
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseHTTPCode = 200;
                Request->ResponseMimeType = Asset->MimeType;
                Request->ResponseBody = Asset->Data;
            }
            else if (Str8Match(Request->RequestPath, Str8Lit("/"), 0))
            {
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseHTTPCode = 200;
                Request->ResponseBody = MainPage(&Server, Server.ResponseArena);
                Request->ResponseMimeType = &MimeType_HTML;
            }
            else
            {
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseHTTPCode = 404;
                Request->ResponseBody = NotFoundPage(&Server, Server.ResponseArena);
                Request->ResponseMimeType = &MimeType_HTML;
            }
		}
	}
}