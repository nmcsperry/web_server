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

// sha 1

typedef struct sha1
{
    u32 SHA1[5];
} sha1;

u32 LeftRotate(u32 Value, u32 Amount)
{
    return Value << Amount | Value >> (32 - Amount);
}

u32 LeftRotateSwapped(u32 Value, u32 Amount)
{
    return SwapByteOrderU32(LeftRotate(SwapByteOrderU32(Value), Amount));
}

#if 1
#define LR(A, B) LeftRotate(A, B)
#define SWAP_A(A) (A)
#define SWAP_B(A) (A)
#else
#define LR(A, B) LeftRotate(A, B)
#define SWAP_A(A) (A)
#define SWAP_B(A) SwapByteOrderU32(A)
#endif

void CalculateSHA1Core(u32 * Result, u32 * Chunk)
{
    u32 W[80] = { 0 };
    // OSCopyMemory(W, Chunk, sizeof(u32) * 16);

    u32 A = Result[0];
    u32 B = Result[1];
    u32 C = Result[2];
    u32 D = Result[3];
    u32 E = Result[4];

    for (i32 I = 0; I < 16; I++)
    {
        W[I] = SwapByteOrderU32(Chunk[I]);
    }

    for (i32 I = 16; I < 80; I++)
    {
        W[I] = LeftRotate(W[I - 3] ^ W[I - 8] ^ W[I - 14] ^ W[I - 16], 1);
    }

    for (i32 I = 0; I < 80; I++)
    {
        u32 F = 0, K = 0;
        if (I < 20)
        {
            F = (B & C) | ((~B) & D);
            K = 0x5A827999;
        }
        else if (I < 40)
        {
            F = B ^ C ^ D;
            K = 0x6ED9EBA1;
        }
        else if (I < 60)
        {
            F = B & C | B & D | C & D;
            K = 0x8F1BBCDC;
        }
        else if (I < 80)
        {
            F = B ^ C ^ D;
            K = 0xCA62C1D6;
        }

        u32 Temp = LeftRotate(A, 5) + F + E + K + W[I];
        E = D;
        D = C;
        C = LeftRotate(B, 30);
        B = A;
        A = Temp;
    }

    Result[0] += A;
    Result[1] += B;
    Result[2] += C;
    Result[3] += D;
    Result[4] += E;
}

sha1 CalculateSHA1(blob Message)
{
    u64 MessageLength = Message.Count * 8;

    u32 Hash[5] = {
        0x67452301,
        0xEFCDAB89,
        0x98BADCFE,
        0x10325476,
        0xC3D2E1F0
    };

    while (Message.Count >= 64)
    {
        CalculateSHA1Core(Hash, Message.Data);

        Message.Count -= 64;
        Message.Data += 64;
    }

    u32 ExtraChunk[16] = { 0 };
    u8 * ExtraChunkU8 = &ExtraChunk[0];
    u64 * ExtraChunkU64 = &ExtraChunk[0];

    OSCopyMemory(ExtraChunkU8, Message.Data, Message.Count);
    ExtraChunkU8[Message.Count] = 0x80;
    if (Message.Count >= 56)
    {
        CalculateSHA1Core(Hash, ExtraChunk);
        OSZeroMemory(ExtraChunk, sizeof(ExtraChunk));
    }

    ExtraChunkU64[7] = SwapByteOrderU64(MessageLength);
    CalculateSHA1Core(Hash, ExtraChunk);

    sha1 SHA1 = { 0 };
    OSCopyMemory(&SHA1, Hash, sizeof(SHA1));
    return SHA1;
}

void EntryHook()
{
    sha1 test = CalculateSHA1(Str8Lit("abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc"));
    StdOutputFmt("%{hex32} : %{hex32} : %{hex32} : %{hex32} : %{hex32}", test.SHA1[0], test.SHA1[1], test.SHA1[2], test.SHA1[3], test.SHA1[4]);

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
            asset * Asset = HashTableGet(AssetHashTable, Request->Path);
            if (Asset)
            {
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseHTTPCode = 200;
                Request->ResponseMimeType = Asset->MimeType;
                Request->ResponseBody = Asset->Data;
            }
            else if (Str8Match(Request->Path, Str8Lit("/"), 0))
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