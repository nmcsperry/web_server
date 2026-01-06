#include "../reuse/base/base_include.h"
#include "../reuse/network/network_include.h"
#include "../reuse/io/io_include.h"

#include "../reuse/base/base_include.c"
#include "../reuse/network/network_include.c"
#include "../reuse/io/io_include.c"

#include "server.h"
#include "server.c"
#include "html.c"

#define ManagedHTMLSessionCount 16

typedef struct managed_html_session
{
    bool32 Valid;
    u64 SessionId;

    i32 LastArenaIndex;
    i32 CurrentArenaIndex;

    html_node * LastDocument;
    u32 LastDocumentId;

    u64 FirstCommunication; // Unix time in seconds
    u64 LastCommunication;  // Unix time in seconds

    u32 Messages;
} managed_html_session;

typedef struct managed_html_session_pool
{
    managed_html_session Sessions[ManagedHTMLSessionCount];
    memory_arena * Arenas[ManagedHTMLSessionCount * 2];
    bool32 ArenaOccupancy[ManagedHTMLSessionCount * 2];

    random_series Random;
} managed_html_session_pool;

managed_html_session_pool GlobalSessionPool;

typedef struct html_context
{
    bool32 Valid;
    html_writer Writer;
    managed_html_session * Session;
    i32 ClientId;
    u32 PassCount;
    u32 PassCountMax;
} html_context;

void ManagedHTMLSessionsInit()
{
    GlobalSessionPool.Random = RandomSeriesCreate(0);
    for (i32 I = 0; I < ManagedHTMLSessionCount * 2; I++)
    {
        GlobalSessionPool.Arenas[I] = ArenaCreateAdv(Kilobytes(4), 0, 0);
    }
}

#define _ignore_html_context
#define ManagedHTMLSession(ContextDeclaration, Server, Request) \
    for ( \
        ContextDeclaration = HTMLStartManaged(Server, Request); \
        _ignore_ ## ContextDeclaration.PassCount > _ignore_ ## ContextDeclaration.PassCountMax; \
        HTMLFulfillRequest(Server, & _ignore_ ## ContextDeclaration, Request))

managed_html_session * CreateManagedHTMLSession()
{
    for (i32 I = 0; I < ManagedHTMLSessionCount; I++)
    {
        managed_html_session * Session = &GlobalSessionPool.Sessions[I];
        if (!Session->Valid)
        {
            OSZeroMemory(Session, sizeof(managed_html_session));

            Session->Valid = true;
            Session->SessionId = RandomU64(&GlobalSessionPool.Random);

            Session->LastArenaIndex = -1;
            Session->CurrentArenaIndex = -1;

            Session->FirstCommunication = UnixTimeSec();

            return Session;
        }
    }

    return 0;
}

managed_html_session * FindManagedHTMLSession(u64 SessionId)
{
    for (i32 I = 0; I < ManagedHTMLSessionCount; I++)
    {
        managed_html_session * Session = &GlobalSessionPool.Sessions[I];
        if (Session->Valid && Session->SessionId == SessionId)
        {
            return Session;
        }
    }

    return 0;
}

html_context HTMLStartManaged(web_server * Server, web_request * Request)
{
    managed_html_session * Session = 0;
    if (Request->ProtocolType == WebProtocol_HTTP)
    {
        Session = CreateManagedHTMLSession();
    }
    else if (Request->ProtocolType == WebProtocol_WebSocket && Request->RequestSessionCookie)
    {
        Session = FindManagedHTMLSession(Request->RequestSessionCookie);
    }

    if (Session == 0)
    {
        return (html_context) { 0 };
    }

    memory_arena * Arena = 0;
    for (i32 I = 0; I < ManagedHTMLSessionCount * 2; I++)
    {
        if (!GlobalSessionPool.ArenaOccupancy[I])
        {
            Arena = GlobalSessionPool.Arenas[I];
            GlobalSessionPool.ArenaOccupancy[I] = true;
            Session->CurrentArenaIndex = I;
            break;
        }
    }
    if (Arena == 0)
    {
        return (html_context) { 0 };
    }

    if (Session->Messages == 0)
    {
        return (html_context) {
            .Valid = true,
            .Writer = HTMLWriterCreate(Arena, 0, 0),
            .Session = Session,
            .PassCount = 0,
            .PassCountMax = 0
        };
    }
    else
    {
        return (html_context) {
            .Valid = true,
            .Writer = HTMLWriterCreate(Arena, Session->LastDocument, Session->LastDocumentId),
            .Session = Session,
            .ClientId = I32FromStr8(Request->RequestBody, 0),
            .PassCount = 0,
            .PassCountMax = 1
        };
    }
}

html_context HTMLStartPlain(web_server * Server, web_request * Request)
{
    return (html_context) {
        .Valid = true,
        .Writer = HTMLWriterCreate(Server->ResponseArena, 0, 0),
        .Session = 0
    };
}

void HTMLFulfillRequest(web_server * Server, html_context * Context, web_request * Request)
{
    if (!Context->Valid)
    {
        Request->ResponseBehavior = ResponseBehavior_Respond;
        Request->ResponseCode = 500;
        Request->ResponseMimeType = &MimeType_Text;
        Request->ResponseBody = Str8Lit("Error");

        return;
    }

    Context->PassCount++;

    if (Context->Session)
    {
        if (Context->Session->Messages == 0)
        {
            Request->ResponseBehavior = ResponseBehavior_Respond;
            Request->ResponseCode = 200;
            Request->ResponseMimeType = &MimeType_HTML;
            Request->ResponseBody = Str8FromHTML(Server->ResponseArena, Context->Writer.DocumentRoot);
            Request->ResponseSessionCookie = Context->Session->SessionId;
        }
        else
        {
            if (Context->PassCount == 1)
            {
                Context->ClientId = 0;
                HTMLWriterReset(&Context->Writer, Context->Session->LastDocumentId);
                return;
            }

            Request->ResponseBehavior = ResponseBehavior_Respond;
            Request->ResponseCode = 1;
            Request->ResponseBody = Str8FromHTMLDiff(Server->ResponseArena, Context->Writer.Diffs);

            ArenaReset(GlobalSessionPool.Arenas[Context->Session->LastArenaIndex]);
            GlobalSessionPool.ArenaOccupancy[Context->Session->LastArenaIndex] = false;
        }

        Context->Session->Messages++;
        Context->Session->LastCommunication = UnixTimeSec();

        Context->Session->LastArenaIndex = Context->Session->CurrentArenaIndex;
        Context->Session->CurrentArenaIndex = -1;

        Context->Session->LastDocument = Context->Writer.DocumentRoot;
        Context->Session->LastDocumentId = Context->Writer.LastId;
    }
    else
    {
        Request->ResponseBehavior = ResponseBehavior_Respond;
        Request->ResponseCode = 200;
        Request->ResponseMimeType = &MimeType_HTML;
        Request->ResponseBody = Str8FromHTML(Server->ResponseArena, Context->Writer.DocumentRoot);
    }
}

str8 NotFoundPage(web_server * Server, memory_arena * Arena)
{
    html_writer Writer = HTMLWriterCreate(Arena, 0, 0);

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

bool32 HTMLElementWasClicked(html_context * Context)
{
    return Context->ClientId == Context->Writer.TagStack[Context->Writer.StackIndex]->Id;
}

bool32 HTMLButton(html_context * Context, str8 Text)
{
    bool32 Result;
    HTMLTag(&Context->Writer, HTMLTag_button)
    {
        HTMLAttr(&Context->Writer, HTMLAttr_onclick, Str8Lit("Interact(this)"));
        HTMLText(&Context->Writer, Text);
        Result = HTMLElementWasClicked(Context);
    }
    return Result;
}

i32 GlobalCounter = 0;

void MainPage(web_server * Server, web_request * Request)
{
    for (html_context Ctx = HTMLStartManaged(Server, Request); Ctx.PassCount <= Ctx.PassCountMax; HTMLFulfillRequest(Server, &Ctx, Request))
    {
        HTMLTag(&Ctx.Writer, HTMLTag_head)
        {
            HTMLTag(&Ctx.Writer, HTMLTag_title)
            {
                HTMLText(&Ctx.Writer, Str8Lit("HTML Builder"));
            }
        }

        HTMLTag(&Ctx.Writer, HTMLTag_body)
        {
            HTMLTag(&Ctx.Writer, HTMLTag_img)
            {
                HTMLAttr(&Ctx.Writer, HTMLAttr_src, Str8Lit("/my_image.png"));
            }

            HTMLSimpleTagFmt(&Ctx.Writer, HTMLTag_div, "%{i32}", GlobalCounter);

            if (HTMLButton(&Ctx, Str8Lit("Increment!")))
            {
                GlobalCounter++;
            }

            HTMLSimpleTagFmt(&Ctx.Writer, HTMLTag_div, "%{i32}", GlobalCounter);

            HTMLTag(&Ctx.Writer, HTMLTag_p)
            {
                HTMLTag(&Ctx.Writer, HTMLTag_span)
                {
                    HTMLStyle(&Ctx.Writer, HTMLStyle_color, Str8Lit("red"));
                    HTMLText(&Ctx.Writer, Str8Lit("This is a red paragraph."));
                }
            }

            HTMLTagKey(&Ctx.Writer, HTMLTag_script, 7)
            {
                HTMLAttr(&Ctx.Writer, HTMLAttr_type, Str8Lit("text/javascript"));
                HTMLAttr(&Ctx.Writer, HTMLAttr_src, Str8Lit("script.js"));
                HTMLTextCStr(&Ctx.Writer, "var test;");
            }
        }
    }
}

/*

for (u32 I = 0; I < ArrayCount(Server->Connections); I++)
{
    web_connection_slot * Connection = &Server->Connections[I];

    if (Connection->Value.Valid)
    {
        HTMLSimpleTagCStr(&Writer, HTMLTag_h3, "Connection");
        HTMLSimpleTagFmt(&Writer, HTMLTag_p, "IP Address: %{str8}", Str8FromIPAddr(Writer.Arena, Connection->Value.Address));

        datetime Datetime = DatetimeFromUnixTimeSec(Connection->Value.FirstCommunication);
        HTMLSimpleTagFmt(&Writer, HTMLTag_p, "First Communication: %{str8}", Str8FromDatetime(Writer.Arena, Datetime));

        temp_memory_arena Scratch = GetScratchArena(0, 0);
        ReleaseScratchArena(Scratch);

        HTMLSimpleTagFmt(&Writer, HTMLTag_p, "Total Requests: %{u32}", Connection->Value.RequestsReceived);
    }
}

*/

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
    InitAssetPage(Arena, AssetHashTable, "script.js", Str8Lit("/script.js"), &MimeType_JS);
}

void EntryHook()
{
	SocketInit();
    ManagedHTMLSessionsInit();

    memory_arena * AssetArena = ArenaCreate(Megabytes(64));
    InitAssetPages(AssetArena);

	socket_handle Socket = SocketCreateServer(80, 50);
	web_server Server = ServerInit(Socket);

	while (true)
	{
		ServerLoop(&Server);

		web_request * Request = 0;
		while (Request = ServerNextRequest(&Server))
		{
            //if (Request->ProtocolType == WebProtocol_WebSocket)
            //{
            //    StdOutputFmt("Got a websocket request\n");
            //    Request->ResponseBehavior = ResponseBehavior_Respond;
            //    Request->ResponseCode = 1;
            //    Request->ResponseBody = MainPageDiff(&Server, Server.ResponseArena);
            //    continue;
            //}

            asset * Asset = HashTableGet(AssetHashTable, Request->RequestPath);
            if (Asset)
            {
                StdOutputFmt("Got an asset request\n\n");
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseCode = 200;
                Request->ResponseMimeType = Asset->MimeType;
                Request->ResponseBody = Asset->Data;
            }
            else if (Str8Match(Request->RequestPath, Str8Lit("/"), 0))
            {
                StdOutputFmt("Got root request\n\n");

                MainPage(&Server, Request);
            }
            else
            {
                StdOutputFmt("Unknown request, responding with 404\n\n");
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseCode = 404;
                Request->ResponseBody = NotFoundPage(&Server, Server.ResponseArena);
                Request->ResponseMimeType = &MimeType_HTML;
            }
		}
	}
}