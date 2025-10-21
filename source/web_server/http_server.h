#ifndef http_server_h
#define http_server_h

#include "../reuse/network/network_include.h"

#define HTTP_CONNECTION_COUNT 32
#define HTTP_REQUEST_COUNT 64
#define HTTP_REQUEST_PARSE_SIZE Kilobytes(1)
#define HTTP_REQUEST_TOTAL_SIZE Megabytes(2)
#define HTTP_CONNECTION_TIMEOUT 200
#define HTTP_CONNECTION_TIMEOUT_MIDREQUEST 100

typedef struct http_connection
{
	bool32 Valid;
	u32 ProtocolType;

	socket_handle Socket;
	u64 FirstCommunication; // Unix time in seconds
	u64 LastCommunication; // Unix time in seconds
	u32 ResponsesSent;
	u32 RequestsReceived;

	str8 WebSocketPath;

	str8ll RequestPathHistory;

	u32 ParsingRequestIndex;
	bool32 IsParsingRequest;

	ip_addr Address;
} http_connection;

typedef struct http_connection_slot {
	u32 Index;

	memory_arena * Arena;
	memory_buffer Unparsed;

	http_connection Value;
} http_connection_slot;

enum HTTPResponseBehavior
{
	ResponseBehavior_Ignore = 0,
	ResponseBehavior_Respond = 1,
	ResponseBehavior_Close = 2,
	ResponseBehavior_RespondClose = 3
};

typedef str8 mime_type;

enum web_protocol_type
{
	WebProtocol_HTTP,
	WebProtocol_WebSocket
};

enum http_request_status
{
	HTTPRequest_Invalid,
	HTTPRequest_Parsing,
	HTTPRequest_Ready,
	HTTPRequest_Processed
};

typedef struct http_request
{
	u32 Status;

	bool8 RequestMethodComplete;
	bool8 RequestHeadersComplete;
	bool8 RequestContentLengthComplete;
	bool8 RequestBodyComplete;

	u32 RequestHTTPMethod;
	str8 RequestPath;
	str8 RequestBody;
	u32 RequestContentLength;

	u32 RequestProtocolSwitch;
	str8 RequestWebSocketKey;

	u32 ConnectionIndex;

	u16 ResponseBehavior;
	u16 ResponseHTTPCode;
	str8 ResponseBody;
	mime_type * ResponseMimeType;

	str8 ResponseWebSocketAccept;
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

mime_type MimeType_HTML = Str8LitInit("text/html; charset=utf-8");
mime_type MimeType_PNG = Str8LitInit("image/png");
mime_type MimeType_ICO = Str8LitInit("image/x-icon");
mime_type MimeType_Text = Str8LitInit("text/plain");
mime_type MimeType_JS = Str8LitInit("text/javascript");
mime_type MimeType_Unknown = Str8LitInit("application/octet-stream");

http_server ServerInit(socket_handle Socket);
void ServerLoop(http_server * Server);

http_connection_slot * AddConnection(http_server * Server, socket_handle Socket, ip_addr Address);
bool8 CloseConnection(http_server * Server, http_connection_slot * Connection, u16 Reason);

bool8 CloseRequest(http_request_slot * RequestSlot);

void ParseHttpRequest(http_server * Server, http_connection_slot * Connection, str8 * HttpRequest);

http_request_slot * AddRequest(http_server * Server, http_connection_slot * Connection);

str8 HTTPReasonName(u16 Reason);

#endif