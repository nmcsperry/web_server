#ifndef http_server_h
#define http_server_h

#include "../reuse/network/network_include.h"

#define HTTP_CONNECTION_COUNT 32
#define HTTP_REQUEST_COUNT 64
#define HTTP_REQUEST_PARSE_SIZE Kilobytes(1)
#define HTTP_REQUEST_TOTAL_SIZE Megabytes(2)
#define HTTP_CONNECTION_TIMEOUT 200
#define HTTP_CONNECTION_TIMEOUT_MIDREQUEST 100

typedef struct http_request_parser
{
	bool8 MethodComplete;
	bool8 HeadersComplete;
	bool8 ContentLengthComplete;
	bool8 BodyComplete;

	u32 HTTPError; // 400 Bad Request, 411 Length Required, 413 Content Too Large, 431 Request Header Fields Too Large, 408 Request Timeout
	u32 ContentLength;
	// bool32 CloseConnection;

	u16 HTTPMethod;
	str8 Path;
	str8 Body;
} http_request_parser;

typedef struct http_connection
{
	bool32 Valid;

	socket_handle Socket;
	u64 LastCommunication; // Unix time in seconds
	u64 FirstCommunication; // Unix time in seconds
	u64 ResponsesSent;

	ip_addr Address;

	http_request_parser RequestParser;
} http_connection;

typedef struct http_connection_slot {
	u32 Index;

	memory_arena * ParsingArena;
	memory_buffer Unparsed;
	temp_memory_arena Initial;

	http_connection Value;
} http_connection_slot;

enum HTTPResponseBehavior
{
	ResponseBehavior_Ignore = 0,
	ResponseBehavior_Respond = 1,
	ResponseBehavior_Close = 2,
	ResponseBehavior_RespondClose = 3
};

enum HTTPMimeType
{
	HTTPMimeType_Unknown = 0,
	HTTPMimeType_HTML = 1,
	HTTPMimeType_PNG = 2
};

typedef struct http_request
{
	bool32 Valid;
	bool32 Processed;
	u32 ConnectionIndex;

	u16 ResponseBehavior;
	u16 ResponseHTTPCode;
	str8 ResponseBody;
	u16 ResponseMimeType;

	u16 HTTPMethod;
	str8 Path;
	str8 Body;
} http_request;

typedef struct http_request_slot
{
	u32 Index;

	memory_arena * Arena;

	http_request Value;
} http_request_slot;

typedef struct http_server {
	http_connection_slot Connections[HTTP_CONNECTION_COUNT];
	http_request_slot Requests[HTTP_REQUEST_COUNT];

	memory_buffer ParsingWorkspace;

	socket_handle ServerSocket;

	memory_arena * Arena;
	memory_arena * ResponseArena;
	socket_polling * Polling;
} http_server;

enum http_method
{
	HTTP_GET,
	HTTP_POST
};

http_server ServerInit(socket_handle Socket);
void ServerLoop(http_server * Server);

http_connection_slot * AddConnection(http_server * Server, socket_handle Socket, ip_addr Address);
bool8 CloseConnection(http_server * Server, http_connection_slot * Connection, u16 Reason);

bool8 CloseRequest(http_request_slot * RequestSlot);

void ParseHttpRequest(http_server * Server, http_connection_slot * Connection, str8 * HttpRequest);

http_request_slot * AddRequest(http_server * Server, http_connection_slot * Connection);

str8 HTTPReasonName(u16 Reason);

#endif