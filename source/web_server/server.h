#ifndef web_server_server_h
#define web_server_server_h

#include "../reuse/network/network_include.h"

#define WEB_CONNECTION_COUNT 32
#define WEB_REQUEST_COUNT 64

#define HTTP_REQUEST_PARSE_SIZE Kilobytes(1)
#define HTTP_REQUEST_TOTAL_SIZE Megabytes(2)
#define HTTP_CONNECTION_TIMEOUT 200
#define HTTP_CONNECTION_TIMEOUT_MIDREQUEST 200

#define WEBSOCKET_BODY_TOTAL_SIZE Kilobytes(1)
#define WEBSOCKET_CONNECTION_PING 10
#define WEBSOCKET_CONNECTION_TIMEOUT 20

typedef struct web_connection
{
	bool32 Valid;
	u32 ProtocolType;

	socket_handle Socket;
	u64 FirstCommunication; // Unix time in seconds
	u64 LastCommunication;  // Unix time in seconds
	u64 LastSent;           // Unix time in seconds 
	u32 ResponsesSent;
	u32 RequestsReceived;

	str8 WebSocketPath;

	u32 ParsingRequestIndex;
	bool32 IsParsingRequest;

	ip_addr Address;
} web_connection;

typedef struct web_connection_slot {
	u32 Index;

	memory_arena * Arena;
	memory_buffer Unparsed;

	web_connection Value;
} web_connection_slot;

enum WebResponseBehavior
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

enum web_request_status
{
	WebRequest_Invalid,
	WebRequest_Parsing,
	WebRequest_Ready,
	WebRequest_Processed
};

typedef struct web_request
{
	u32 Status;
	bool32 Synthetic;

	bool8 FrameHTTPMethodComplete;
	bool8 FrameHeaderComplete;
	bool8 FrameBodyComplete;
	bool8 RequestComplete;

	bool8 HasMoreFrames;
	u32 Mask;

	u32 RequestHTTPMethod;
	str8 RequestPath;
	str8 RequestBody;
	u32 FrameCount;

	u32 RequestHasContentLength;
	u32 RequestContentLength;

	u32 RequestProtocolSwitch;
	str8 RequestWebSocketKey;

	u32 ConnectionIndex;

	u16 ResponseBehavior;
	u16 ResponseCode;
	str8 ResponseBody;
	mime_type * ResponseMimeType;

	memory_buffer BodyBuffer;

	str8 ResponseWebSocketAccept;
} web_request;

typedef struct web_request_slot
{
	u32 Index;

	memory_arena * Arena;

	web_request Value;
} web_request_slot;

typedef struct web_server {
	web_connection_slot Connections[WEB_CONNECTION_COUNT];
	web_request_slot Requests[WEB_REQUEST_COUNT];

	memory_buffer ParsingWorkspace;

	socket_handle ServerSocket;

	memory_arena * Arena;
	memory_arena * ResponseArena;
	socket_polling * Polling;
} web_server;

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

enum websocket_opcode
{
	WebSocket_Continuation = 0,
	WebSocket_Text = 1,
	WebSocket_Binary = 2,
	WebSocket_Close = 8,
	WebSocket_Ping = 9,
	WebSocket_Pong = 10
};

web_server ServerInit(socket_handle Socket);
void ServerLoop(web_server * Server);

void RespondToRequestWithWebSocketFrame(web_server * Server, web_request_slot * RequestSlot, web_connection_slot * ConnectionSlot);
void RespondToRequestWithHTTP(web_server * Server, web_request_slot * RequestSlot, web_connection_slot * ConnectionSlot);
void RespondToRequest(web_server * Server, web_request_slot * RequestSlot, web_connection_slot * ConnectionSlot);

web_request_slot * AddRequest(web_server * Server, web_connection_slot * Connection, bool32 Synthetic);
bool8 CloseRequest(web_request_slot * RequestSlot);

web_connection_slot * AddConnection(web_server * Server, socket_handle Socket, ip_addr Address);
bool8 CloseConnection(web_server * Server, web_connection_slot * Connection);

void ParseHttpRequest(web_server * Server, str8 * RequestData, web_request_slot * RequestSlot);
void ParseWebsocketRequest(web_server * Server, str8 * RequestData, web_request_slot * RequestSlot);

void WebRequestRespondWithError(web_request * Request, u32 Code);

web_request * ServerNextRequest(web_server * Server);

str8 HTTPReasonName(u16 Reason);

str8 WebsocketReasonName(u16 Reason);

#endif