#include "../reuse/base/base_include.h"
#include "../reuse/io/io_include.h"
#include "../reuse/network/network_include.h"
#include "http_server.h"

http_server ServerInit(socket_handle Socket)
{
	memory_arena * ServerArena = ArenaCreate(0);
	memory_arena * ResponseArena = ArenaCreate(Megabytes(128));

	http_server Result = (http_server)
	{
		.ParsingWorkspace = BufferCreate(HTTP_REQUEST_TOTAL_SIZE),
		.ServerSocket = Socket,
		.Arena = ServerArena,
		.ResponseArena = ResponseArena,
		.Polling = SocketPollingCreate(ServerArena, HTTP_CONNECTION_COUNT)
	};

	for (i32 I = 0; I < HTTP_CONNECTION_COUNT; I++)
	{
		memory_arena * Arena = ArenaCreateAdv(Kilobytes(4), 0, 0);
		Result.Connections[I].Index = I;
		Result.Connections[I].Arena = Arena;
		Result.Connections[I].Unparsed = BufferCreate(Kilobytes(1));

		OSZeroMemory(&Result.Connections[I].Value, sizeof(http_connection));
	}

	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		memory_arena * Arena = ArenaCreateAdv(Kilobytes(4), 0, 0);
		Result.Requests[I].Index = I;
		Result.Requests[I].Arena = Arena;

		OSZeroMemory(&Result.Requests[I].Value, sizeof(http_request));
	}

	return Result;
}

void RespondToRequestWithWebSocketFrame(http_server * Server, http_request_slot * RequestSlot, http_connection_slot * ConnectionSlot)
{
	http_request * Request = &RequestSlot->Value;
	http_connection * Connection = &ConnectionSlot->Value;

	// todo: implement this

	return;
}

void RespondToRequestWithHTTP(http_server * Server, http_request_slot * RequestSlot, http_connection_slot * ConnectionSlot)
{
	http_request * Request = &RequestSlot->Value;
	http_connection * Connection = &ConnectionSlot->Value;

	memory_buffer * Buffer = ScratchBufferStart();

	Str8WriteFmt(Buffer, "HTTP/1.1 %{u16} %{str8}\r\n", Request->ResponseHTTPCode, HTTPReasonName(Request->ResponseHTTPCode));

	if (Request->ResponseBehavior && Request->ResponseHTTPCode >= 500)
	{
		Request->ResponseBehavior |= ResponseBehavior_Close;
	}

	if (!(Request->ResponseBehavior & ResponseBehavior_Close))
	{
		Str8WriteFmt(Buffer, "Connection: Keep-Alive\r\n");
	}

	if (Request->RequestProtocolSwitch == WebProtocol_WebSocket)
	{
		str8 WebSocketMagicValue = Str8Lit("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
		sha1 SHA1 = CalculateSHA1(Str8Concat(RequestSlot->Arena, Request->RequestWebSocketKey, WebSocketMagicValue));

		Str8WriteFmt(Buffer, "Sec-WebSocket-Accept: %{base64}\r\n", Blob(SHA1));
	}

	if (Request->ResponseBody.Data)
	{
		if (Request->ResponseMimeType)
		{
			Str8WriteFmt(Buffer, "Content-Type: %{str8}\r\n", *Request->ResponseMimeType);
		}

		Str8WriteFmt(Buffer, "Content-Length: %{u32}\r\n\r\n%{str8}", Request->ResponseBody.Count, Request->ResponseBody);
	}
	else
	{
		Str8WriteFmt(Buffer, "\r\n");
	}

	str8 ResponseString = Str8FromBuffer(Buffer);
	SocketOutput(Connection->Socket, ResponseString);

	StdOutput(ResponseString);
	StdOutput(Str8Lit("END"));

	ScratchBufferRelease(Buffer);
}

void RespondToRequest(http_server * Server, http_request_slot * RequestSlot, http_connection_slot * ConnectionSlot)
{
	http_request * Request = &RequestSlot->Value;
	http_connection * Connection = &ConnectionSlot->Value;

	if (Request->ResponseBehavior & ResponseBehavior_Respond)
	{
		if (Connection->ProtocolType == WebProtocol_WebSocket && Connection->ResponsesSent != 0)
		{
			RespondToRequestWithWebSocketFrame(Server, RequestSlot, ConnectionSlot);
		}
		else
		{
			RespondToRequestWithHTTP(Server, RequestSlot, ConnectionSlot);
		}
	
		Connection->ResponsesSent++;
	}

	if (Request->ResponseBehavior & ResponseBehavior_Close)
	{
		CloseConnection(Server, ConnectionSlot, 0);
	}
}

void ServerLoop(http_server * Server)
{
	// send responses for requests the application code has processed

	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		http_request_slot * RequestSlot = &Server->Requests[I];
		http_request * Request = &RequestSlot->Value;

		http_connection_slot * ConnectionSlot = Request->Status ? &Server->Connections[Request->ConnectionIndex] : 0;
		http_connection * Connection = ConnectionSlot ? &ConnectionSlot->Value : 0;

		bool32 ConnectionValid = Connection && Connection->Valid;

		if (ConnectionValid && Request->Status == HTTPRequest_Processed)
		{
			RespondToRequest(Server, RequestSlot, ConnectionSlot);
		}
		
		if (!ConnectionValid || Request->Status == HTTPRequest_Processed)
		{
			CloseRequest(RequestSlot);
		}
	}

	ArenaReset(Server->ResponseArena);
	
	// accept new connections

	socket_handle NewConnectionSocket = SocketGetInvalid();

	for (i32 I = 0; I < 16; I++)
	{
		ip_addr Address;
		if (SocketAccept(Server->ServerSocket, &NewConnectionSocket, &Address))
		{
			if (!AddConnection(Server, NewConnectionSocket, Address))
			{
				SocketClose(NewConnectionSocket);
			}
		}
		else
		{
			break;
		}
	}

	// poll existing connections

	SocketPollingPoll(Server->Polling, 200);

	u64 CurrentTime = UnixTimeSec();

	for (i32 ConnectionIndex = 0; ConnectionIndex < HTTP_CONNECTION_COUNT; ConnectionIndex++)
	{
		http_connection_slot * Connection = &Server->Connections[ConnectionIndex];
		http_connection * ConnectionInfo = &Connection->Value;

		if (!ConnectionInfo->Valid)
		{
			continue;
		}

		memory_buffer * Unparsed = &Connection->Unparsed;
		bool32 ConnectionHasData = SocketPollingGet(Server->Polling, ConnectionIndex);

		if (!ConnectionHasData)
		{
			u64 Elapsed = CurrentTime - ConnectionInfo->LastCommunication;
			bool32 MidRequest = !!Unparsed->Count;
			u64 Timeout = MidRequest ? HTTP_CONNECTION_TIMEOUT_MIDREQUEST : HTTP_CONNECTION_TIMEOUT;

			if (Elapsed >= Timeout)
			{
				CloseConnection(Server, Connection, ConnectionInfo->ResponsesSent == 0 ? 0 : 408);
			}
		}
		else
		{
			// get input from socket
			memory_buffer * Workspace = &Server->ParsingWorkspace;
			BufferReset(Workspace);

			Str8WriteStr8(Workspace, Str8FromBuffer(Unparsed));
			SocketInputToBuffer(ConnectionInfo->Socket, Workspace);
			str8 RequestData = Str8FromBuffer(Workspace);

			// get request we are creating, or make a new one if needed

			http_request_slot * RequestSlot;
			if (Connection->Value.IsParsingRequest)
			{
				RequestSlot = &Server->Requests[Connection->Value.ParsingRequestIndex];
			}
			else
			{
				RequestSlot = AddRequest(Server, Connection);
			}
			http_request * Request = &RequestSlot->Value;

			if (Connection->Value.ProtocolType == WebProtocol_HTTP)
			{
				ParseHttpRequest(Server, &RequestData, RequestSlot);

				if (Request->Status == HTTPRequest_Processed)
				{
					Connection->Value.IsParsingRequest = false;
					Connection->Value.ParsingRequestIndex = 0;
					break;
				}

				if (Request->RequestBodyComplete)
				{
					Request->Status = HTTPRequest_Ready;
					Connection->Value.IsParsingRequest = false;
					Connection->Value.ParsingRequestIndex = 0;

					if (Request->RequestProtocolSwitch == WebProtocol_WebSocket)
					{
						Connection->Value.ProtocolType = WebProtocol_WebSocket;
						Connection->Value.WebSocketPath = ArenaPushStr8(Connection->Arena, Request->RequestPath);
					}
					else
					{
						Connection->Value.ProtocolType = WebProtocol_HTTP;
					}
				}

				BufferReset(&Connection->Unparsed);
				if (RequestData.Count)
				{
					Str8WriteStr8(&Connection->Unparsed, RequestData);
				}
			}
		}
	}

	// before we return to user code, we will handle any websocket handshakes

	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		http_request_slot * RequestSlot = &Server->Requests[I];
		http_request * Request = &RequestSlot->Value;

		if (Request->Status == HTTPRequest_Ready && Request->RequestProtocolSwitch == WebProtocol_WebSocket)
		{
			Request->Status = HTTPRequest_Processed;
			Request->ResponseHTTPCode = 101;
			Request->ResponseBehavior = ResponseBehavior_Respond;
			continue;
		}
	}
}

http_request_slot * AddRequest(http_server * Server, http_connection_slot * Connection)
{
	http_request_slot * Request = 0;

	// find connection
	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		if (!Server->Requests[I].Value.Status)
		{
			Request = &Server->Requests[I];
			break;
		}
	}

	if (Request == 0)
	{
		return 0;
	}

	Request->Value.ConnectionIndex = Connection->Index;
	Request->Value.Status = HTTPRequest_Parsing;

	// todo: this can be removed, it's just for testing
	Connection->Value.RequestsReceived++;
	str8 PathSafe = ArenaPushStr8(Server->Arena, Request->Value.RequestPath);
	Str8LLPush(Server->Arena, &Connection->Value.RequestPathHistory, Str8Lit(" - "));
	Str8LLPush(Server->Arena, &Connection->Value.RequestPathHistory, PathSafe);

	return Request;
}

str8 HTTPReasonName(u16 Reason)
{
	switch (Reason)
	{
		case 101: return Str8Lit("Switching Protocols");

		case 200: return Str8Lit("Ok");
		case 204: return Str8Lit("No Content");

		case 400: return Str8Lit("Bad Request");
		case 404: return Str8Lit("Not Found");
		case 408: return Str8Lit("Content Too Large");
		case 411: return Str8Lit("Request Header Fields Too Large");
		case 413: return Str8Lit("Request Timeout");
		case 431: return Str8Lit("Length Required");

		case 500: return Str8Lit("Internal Server Error");

		default: return Str8Lit("Unknown");
	}
}

http_connection_slot * AddConnection(http_server * Server, socket_handle Socket, ip_addr Address)
{
	http_connection_slot * Connection = 0;

	// find connection
	for (i32 I = 0; I < HTTP_CONNECTION_COUNT; I++)
	{
		if (!Server->Connections[I].Value.Valid)
		{
			Connection = &Server->Connections[I];
			break;
		}
	}

	if (Connection == 0)
	{
		return 0;
	}

	Connection->Value.Valid = true;
	Connection->Value.Socket = Socket;
	Connection->Value.LastCommunication = UnixTimeSec();
	Connection->Value.FirstCommunication = UnixTimeSec();
	Connection->Value.Address = Address;
	Connection->Value.ProtocolType = WebProtocol_HTTP;

	SocketPollingAdd(Server->Polling, Socket, Connection->Index);

	return Connection;
}

bool8 CloseConnection(http_server * Server, http_connection_slot * Connection, u16 Reason)
{
	// todo: isn't this normally not correct to send because we've already sent something like this???
	if (Reason)
	{
		str8 ReasonName = HTTPReasonName(Reason);
		SocketOutputFmt(Connection->Value.Socket, "HTTP/1.1 %{u16} %{str8}\r\nConnection: close\r\n\r\n", Reason, ReasonName);
	}

	SocketClose(Connection->Value.Socket);
	SocketPollingRemove(Server->Polling, Connection->Index);

	OSZeroMemory(&Connection->Value, sizeof(http_connection));
	OSZeroMemory(&Connection->Value, sizeof(http_connection));

	BufferZero(&Connection->Unparsed);

	return true;
}

bool8 CloseRequest(http_request_slot * RequestSlot)
{
	ArenaZero(RequestSlot->Arena);
	ArenaReset(RequestSlot->Arena);

	OSZeroMemory(&RequestSlot->Value, sizeof(http_request));

	return true;
}

void HttpRequestRespondWithError(http_request * Request, u32 Code)
{
	Request->ResponseHTTPCode = Code;
	Request->ResponseBehavior = ResponseBehavior_RespondClose;
	Request->Status = HTTPRequest_Processed;
}

void ParseHttpRequest(http_server * Server, str8 * RequestData, http_request_slot * RequestSlot)
{
	memory_arena * Arena = RequestSlot->Arena;
	http_request * Request = &RequestSlot->Value;

	if (!Request->RequestMethodComplete)
	{
		// get next header line
		str8_bool32 HeaderLine = Str8ParseEatUntilStr8Match(RequestData, Str8Lit("\r\n"));
		if (HeaderLine.Bool == false)
		{
			return;
		}
		if (HeaderLine.String.Count > HTTP_REQUEST_PARSE_SIZE)
		{
			HttpRequestRespondWithError(Request, 431);
			return;
		}

		i32 HTTPMethod = Str8ParseExpectAny(&HeaderLine.String, (str8[]) { Str8Lit("GET"), Str8Lit("POST") }, 2, MatchFlag_IgnoreCase);
		if (HTTPMethod == -1 || !Str8ParseExpect(&HeaderLine.String, Str8Lit(" "), MatchFlag_Normal))
		{
			HttpRequestRespondWithError(Request, 400);
			return;
		}
		Request->RequestHTTPMethod = HTTPMethod;

		str8 Path = Str8ParseEatUntilChar(&HeaderLine.String, ' ');
		if (Path.Count == 0 || !Str8ParseExpect(&HeaderLine.String, Str8Lit(" "), MatchFlag_Normal))
		{
			HttpRequestRespondWithError(Request, 400);
			return;
		}
		Request->RequestPath = ArenaPushStr8(Arena, Path);

		if (!Str8ParseExpect(&HeaderLine.String, Str8Lit("HTTP/1.1"), MatchFlag_Normal) && HeaderLine.String.Count == 0)
		{
			HttpRequestRespondWithError(Request, 400);
			return;
		}

		Request->RequestMethodComplete = true;
	}

	if (!Request->RequestHeadersComplete)
	{
		while (true)
		{
			str8_bool32 HeaderLine = Str8ParseEatUntilStr8Match(RequestData, Str8Lit("\r\n"));
			if (HeaderLine.String.Count > HTTP_REQUEST_PARSE_SIZE)
			{
				HttpRequestRespondWithError(Request, 431);
				return;
			}
			if (HeaderLine.Bool == false)
			{
				return;
			}
			if (HeaderLine.String.Count == 0)
			{
				Request->RequestHeadersComplete = true;
				break;
			}

			str8_bool32 HeaderKey = Str8ParseEatUntilStr8Match(&HeaderLine.String, Str8Lit(":"));
			if (HeaderKey.Bool == false)
			{
				HttpRequestRespondWithError(Request, 400);
				return;
			}
			if (HeaderKey.String.Count > HTTP_REQUEST_PARSE_SIZE)
			{
				HttpRequestRespondWithError(Request, 431);
				return;
			}

			str8 HeaderValue = Str8Trim(HeaderLine.String);

			if (Str8Match(HeaderKey.String, Str8Lit("Content-Length"), MatchFlag_IgnoreCase))
			{
				void * Extra = 0;
				Request->RequestContentLength = IntFromStr8(HeaderValue, Extra);
				Request->RequestContentLengthComplete = true;
				if (Extra)
				{
					HttpRequestRespondWithError(Request, 400);
					return;
				}
				if (Request->RequestContentLength > HTTP_REQUEST_PARSE_SIZE)
				{
					HttpRequestRespondWithError(Request, 413);
					return;
				}
			}

			if (Str8Match(HeaderKey.String, Str8Lit("Upgrade"), MatchFlag_IgnoreCase))
			{
				if (Str8Match(HeaderValue, Str8Lit("websocket"), MatchFlag_IgnoreCase))
				{
					Request->RequestProtocolSwitch = WebProtocol_WebSocket;
				}
				else
				{
					HttpRequestRespondWithError(Request, 426);
					return;
				}
			}

			if (Str8Match(HeaderKey.String, Str8Lit("Sec-Websocket-Key"), MatchFlag_IgnoreCase))
			{
				Request->RequestWebSocketKey = ArenaPushStr8(Arena, HeaderValue);
			}
		}
	}

	if (Request->RequestHTTPMethod == 1 && !Request->RequestContentLengthComplete)
	{
		HttpRequestRespondWithError(Request, 411);
		return;
	}
	if (Request->RequestProtocolSwitch && Request->RequestWebSocketKey.Count == 0)
	{
		HttpRequestRespondWithError(Request, 400);
		return;
	}
	if (Request->RequestContentLength == 0)
	{
		Request->RequestBodyComplete = true;
	}

	if (!Request->RequestBodyComplete)
	{
		if (RequestData->Count < Request->RequestContentLength)
		{
			return;
		}
		else
		{
			str8 Body = Str8ParseEat(RequestData, Request->RequestContentLength);
			Request->RequestBody = ArenaPushStr8(Arena, Body);
			Request->RequestBodyComplete = true;
			return;
		}
	}
}

http_request * ServerNextRequest(http_server * Server)
{
	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		http_request_slot * Request = &Server->Requests[I];

		if (Request->Value.Status == HTTPRequest_Ready)
		{
			Request->Value.Status = HTTPRequest_Processed;

			http_connection_slot * Connection = &Server->Connections[Request->Value.ConnectionIndex];

			if (Connection->Value.Valid)
			{
				return &Request->Value;
			}
			else
			{
				Request->Value.ResponseBehavior = ResponseBehavior_Close;
			}
		}
	}

	return 0;
}