#include "../reuse/base/base_include.h"
#include "../reuse/io/io_include.h"
#include "../reuse/network/network_include.h"
#include "server.h"

web_server ServerInit(socket_handle Socket)
{
	memory_arena * ServerArena = ArenaCreate(0);
	memory_arena * ResponseArena = ArenaCreate(Megabytes(128));

	web_server Result = (web_server)
	{
		.ParsingWorkspace = BufferCreate(HTTP_REQUEST_TOTAL_SIZE),
		.ServerSocket = Socket,
		.Arena = ServerArena,
		.ResponseArena = ResponseArena,
		.Polling = SocketPollingCreate(ServerArena, WEB_CONNECTION_COUNT)
	};

	for (i32 I = 0; I < WEB_CONNECTION_COUNT; I++)
	{
		memory_arena * Arena = ArenaCreateAdv(Kilobytes(4), 0, 0);
		Result.Connections[I].Index = I;
		Result.Connections[I].Arena = Arena;
		Result.Connections[I].Unparsed = BufferCreate(Kilobytes(1));

		OSZeroMemory(&Result.Connections[I].Value, sizeof(web_connection));
	}

	for (i32 I = 0; I < WEB_REQUEST_COUNT; I++)
	{
		memory_arena * Arena = ArenaCreateAdv(Kilobytes(4), 0, 0);
		Result.Requests[I].Index = I;
		Result.Requests[I].Arena = Arena;

		OSZeroMemory(&Result.Requests[I].Value, sizeof(web_request));
	}

	return Result;
}

void ServerLoop(web_server * Server)
{
	// send responses for requests the application code has processed

	for (i32 I = 0; I < WEB_REQUEST_COUNT; I++)
	{
		web_request_slot * RequestSlot = &Server->Requests[I];
		web_request * Request = &RequestSlot->Value;

		if (Request->Status == WebRequest_Invalid)
		{
			continue;
		}

		web_connection_slot * ConnectionSlot = Request->Status ? &Server->Connections[Request->ConnectionIndex] : 0;
		web_connection * Connection = ConnectionSlot ? &ConnectionSlot->Value : 0;

		bool32 ConnectionValid = Connection && Connection->Valid;

		if (ConnectionValid && Request->Status == WebRequest_Processed)
		{
			RespondToRequest(Server, RequestSlot, ConnectionSlot);
		}
		
		if (!ConnectionValid || Request->Status == WebRequest_Processed)
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

	for (i32 ConnectionIndex = 0; ConnectionIndex < WEB_CONNECTION_COUNT; ConnectionIndex++)
	{
		web_connection_slot * Connection = &Server->Connections[ConnectionIndex];
		web_connection * ConnectionInfo = &Connection->Value;

		if (!ConnectionInfo->Valid)
		{
			continue;
		}

		memory_buffer * Unparsed = &Connection->Unparsed;
		bool32 ConnectionHasData = SocketPollingGet(Server->Polling, ConnectionIndex);

		if (!ConnectionHasData)
		{
			u64 CommunicationElapsed = CurrentTime - ConnectionInfo->LastCommunication;
			u64 SentElapsed = CurrentTime - ConnectionInfo->LastSent;
			bool32 MidRequest = !!Unparsed->Count;

			if (Connection->Value.ProtocolType == WebProtocol_HTTP)
			{
				u64 Timeout = MidRequest ? HTTP_CONNECTION_TIMEOUT_MIDREQUEST : HTTP_CONNECTION_TIMEOUT;

				if (CommunicationElapsed >= Timeout)
				{
					web_request_slot * CloseRequestSlot = AddRequest(Server, Connection, true);
					web_request * CloseRequest = &CloseRequestSlot->Value;
					CloseRequest->Status = WebRequest_Processed;
					CloseRequest->ResponseCode = 408;
					CloseRequest->ResponseBehavior = ConnectionInfo->ResponsesSent == 0 ? ResponseBehavior_Close : ResponseBehavior_RespondClose;
				}
			}
			else if (Connection->Value.ProtocolType == WebProtocol_WebSocket)
			{
				if (CommunicationElapsed >= WEBSOCKET_CONNECTION_TIMEOUT)
				{
					web_request_slot * CloseRequestSlot = AddRequest(Server, Connection, true);
					web_request * CloseRequest = &CloseRequestSlot->Value;
					CloseRequest->Status = WebRequest_Processed;
					CloseRequest->ResponseCode = 1001;
					CloseRequest->ResponseBehavior = ResponseBehavior_RespondClose;
				}
				else if (CommunicationElapsed >= WEBSOCKET_CONNECTION_PING && SentElapsed >= WEBSOCKET_CONNECTION_PING)
				{
					web_request_slot * PingRequestSlot = AddRequest(Server, Connection, true);
					web_request * PingRequest = &PingRequestSlot->Value;
					PingRequest->Status = WebRequest_Processed;
					PingRequest->ResponseBody = Str8Lit("Ping");
					PingRequest->ResponseCode = WebSocket_Ping;
					PingRequest->ResponseBehavior = ResponseBehavior_Respond;
				}
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

			Connection->Value.LastCommunication = UnixTimeSec();

			// get request we are creating, or make a new one if needed

			web_request_slot * RequestSlot;
			if (Connection->Value.IsParsingRequest)
			{
				RequestSlot = &Server->Requests[Connection->Value.ParsingRequestIndex];
			}
			else
			{
				RequestSlot = AddRequest(Server, Connection, false);
			}
			web_request * Request = &RequestSlot->Value;

			if (Connection->Value.ProtocolType == WebProtocol_HTTP)
			{
				ParseHttpRequest(Server, &RequestData, RequestSlot);
			}
			else if (Connection->Value.ProtocolType == WebProtocol_WebSocket)
			{
				ParseWebsocketRequest(Server, &RequestData, RequestSlot);
			}

			if (Request->Status == WebRequest_Processed)
			{
				Connection->Value.IsParsingRequest = false;
				Connection->Value.ParsingRequestIndex = 0;
				break;
			}

			if (Request->RequestComplete)
			{
				Request->Status = WebRequest_Ready;
				Connection->Value.IsParsingRequest = false;
				Connection->Value.ParsingRequestIndex = 0;

				if (Connection->Value.ProtocolType == WebProtocol_HTTP)
				{
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
			}

			BufferReset(&Connection->Unparsed);
			if (RequestData.Count)
			{
				Str8WriteStr8(&Connection->Unparsed, RequestData);
			}
		}
	}

	// before we return to user code, we will handle any websocket handshakes and such

	for (i32 I = 0; I < WEB_REQUEST_COUNT; I++)
	{
		web_request_slot * RequestSlot = &Server->Requests[I];
		web_request * Request = RequestSlot ? &RequestSlot->Value : 0;

		if (Request->Status == WebRequest_Invalid)
		{
			continue;
		}

		web_connection_slot * ConnectionSlot = Request ? &Server->Connections[Request->ConnectionIndex] : 0;
		web_connection * Connection = ConnectionSlot ? &ConnectionSlot->Value : 0;

		if (Request->Status == WebRequest_Ready && Request->RequestProtocolSwitch == WebProtocol_WebSocket)
		{
			Request->Status = WebRequest_Processed;
			Request->ResponseCode = 101;
			Request->ResponseBehavior = ResponseBehavior_Respond;
		}
		else if (Request->Status == WebRequest_Ready && Connection->ProtocolType == WebProtocol_WebSocket
			&& Request->RequestHTTPMethod == WebSocket_Close)
		{
			Request->Status = WebRequest_Processed;
			Request->ResponseCode = WebSocket_Close;
			Request->ResponseBehavior = ResponseBehavior_RespondClose;
			Request->ResponseBody = Request->RequestBody;
		}
		else if (Request->Status == WebRequest_Ready && Connection->ProtocolType == WebProtocol_WebSocket
			&& Request->RequestHTTPMethod == WebSocket_Ping)
		{
			Request->Status = WebRequest_Processed;
			Request->ResponseCode = WebSocket_Pong;
			Request->ResponseBehavior = ResponseBehavior_Respond;
			Request->ResponseBody = Request->RequestBody;
		}
		else if (Request->Status == WebRequest_Ready && Connection->ProtocolType == WebProtocol_WebSocket
			&& Request->RequestHTTPMethod == WebSocket_Pong)
		{
			Request->Status = WebRequest_Processed;
			Request->ResponseBehavior = ResponseBehavior_Ignore;
		}
	}
}

void RespondToRequestWithWebSocketFrame(web_server * Server, web_request_slot * RequestSlot, web_connection_slot * ConnectionSlot)
{
	web_request * Request = &RequestSlot->Value;
	web_connection * Connection = &ConnectionSlot->Value;

	u8 Opcode = Request->ResponseCode;
	if (Request->ResponseCode >= 1000)
	{
		Opcode = 8;

		memory_buffer * Buffer = ScratchBufferStart();

		u16 ReasonCode = SwapByteOrderU16(Request->ResponseCode);
		BufferPush(Buffer, &ReasonCode, 2);

		if (Request->ResponseBody.Data == 0)
		{
			Str8WriteStr8(Buffer, WebsocketReasonName(Request->ResponseCode));
		}
		else
		{
			Str8WriteStr8(Buffer, Request->ResponseBody);
		}

		Request->ResponseBody = ScratchBufferEndStr8(Buffer, RequestSlot->Arena);
	}
	u8 FirstByte = 0x80 | Opcode;

	u8 Length = Request->ResponseBody.Count;
	if (Request->ResponseBody.Count > U16Max)
	{
		Length = 127;
	}
	else if (Request->ResponseBody.Count > 125)
	{
		Length = 126;
	}
	u8 SecondByte = Length;

	memory_buffer * Buffer = ScratchBufferStart();

	Str8WriteChar8(Buffer, FirstByte);
	Str8WriteChar8(Buffer, SecondByte);

	if (Length == 126)
	{
		u16 Length = Request->ResponseBody.Count;
		Length = SwapByteOrderU16(Length);
		BufferPush(Buffer, &Length, 2);
	}
	else if (Length == 127)
	{
		u64 Length = Request->ResponseBody.Count;
		Length = SwapByteOrderU64(Length);
		BufferPush(Buffer, &Length, 4);
	}

	Str8WriteStr8(Buffer, Request->ResponseBody);

	str8 ResponseString = Str8FromBuffer(Buffer);
	SocketOutput(Connection->Socket, ResponseString);

	ScratchBufferRelease(Buffer);
}

void RespondToRequestWithHTTP(web_server * Server, web_request_slot * RequestSlot, web_connection_slot * ConnectionSlot)
{
	web_request * Request = &RequestSlot->Value;
	web_connection * Connection = &ConnectionSlot->Value;

	memory_buffer * Buffer = ScratchBufferStart();

	Str8WriteFmt(Buffer, "HTTP/1.1 %{u16} %{str8}\r\n", Request->ResponseCode, HTTPReasonName(Request->ResponseCode));

	if (Request->ResponseBehavior && Request->ResponseCode >= 500)
	{
		Request->ResponseBehavior |= ResponseBehavior_Close;
	}

	if (Request->RequestProtocolSwitch == WebProtocol_WebSocket)
	{
		str8 WebSocketMagicValue = Str8Lit("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
		sha1 SHA1 = CalculateSHA1(Str8Concat(RequestSlot->Arena, Request->RequestWebSocketKey, WebSocketMagicValue));

		// todo: something is backwards possibly that this needed?
		SHA1.E[0] = SwapByteOrderU32(SHA1.E[0]);
		SHA1.E[1] = SwapByteOrderU32(SHA1.E[1]);
		SHA1.E[2] = SwapByteOrderU32(SHA1.E[2]);
		SHA1.E[3] = SwapByteOrderU32(SHA1.E[3]);
		SHA1.E[4] = SwapByteOrderU32(SHA1.E[4]);

		Str8WriteFmt(Buffer, "Upgrade: websocket\r\n");
		Str8WriteFmt(Buffer, "Connection: Upgrade\r\n");
		Str8WriteFmt(Buffer, "Sec-WebSocket-Accept: %{base64}\r\n", Blob(SHA1), '=');
	}
	else if (!(Request->ResponseBehavior & ResponseBehavior_Close))
	{
		Str8WriteFmt(Buffer, "Connection: Keep-Alive\r\n");
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

	ScratchBufferRelease(Buffer);
}

void RespondToRequest(web_server * Server, web_request_slot * RequestSlot, web_connection_slot * ConnectionSlot)
{
	web_request * Request = &RequestSlot->Value;
	web_connection * Connection = &ConnectionSlot->Value;

	if (Request->ResponseBehavior & ResponseBehavior_Respond)
	{
		if (Connection->ProtocolType == WebProtocol_WebSocket)
		{
			if (Connection->ResponsesSent == 0) // websocket handshake is basically http
			{
				RespondToRequestWithHTTP(Server, RequestSlot, ConnectionSlot);
			}
			else
			{
				RespondToRequestWithWebSocketFrame(Server, RequestSlot, ConnectionSlot);
			}
		}
		else
		{
			RespondToRequestWithHTTP(Server, RequestSlot, ConnectionSlot);
		}

		Connection->ResponsesSent++;
		Connection->LastSent = UnixTimeSec();
	}

	if (Request->ResponseBehavior & ResponseBehavior_Close)
	{
		CloseConnection(Server, ConnectionSlot);
	}
}

web_request_slot * AddRequest(web_server * Server, web_connection_slot * Connection, bool32 Synthetic)
{
	web_request_slot * Request = 0;

	// find connection
	for (i32 I = 0; I < WEB_REQUEST_COUNT; I++)
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
	Request->Value.Status = WebRequest_Parsing;
	Request->Value.Synthetic = Synthetic;

	Connection->Value.RequestsReceived++;

	return Request;
}

bool8 CloseRequest(web_request_slot * RequestSlot)
{
	ArenaZero(RequestSlot->Arena);
	ArenaReset(RequestSlot->Arena);

	OSZeroMemory(&RequestSlot->Value, sizeof(web_request));

	return true;
}

web_connection_slot * AddConnection(web_server * Server, socket_handle Socket, ip_addr Address)
{
	web_connection_slot * Connection = 0;

	// find connection
	for (i32 I = 0; I < WEB_CONNECTION_COUNT; I++)
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
	Connection->Value.LastSent = 0;
	Connection->Value.Address = Address;
	Connection->Value.ProtocolType = WebProtocol_HTTP;

	SocketPollingAdd(Server->Polling, Socket, Connection->Index);

	return Connection;
}

bool8 CloseConnection(web_server * Server, web_connection_slot * Connection)
{
	StdOutputFmt("Closing something...\n\n");

	SocketClose(Connection->Value.Socket);
	SocketPollingRemove(Server->Polling, Connection->Index);

	OSZeroMemory(&Connection->Value, sizeof(web_connection));
	OSZeroMemory(&Connection->Value, sizeof(web_connection));

	BufferZero(&Connection->Unparsed);

	return true;
}

void ParseHttpRequest(web_server * Server, str8 * RequestData, web_request_slot * RequestSlot)
{
	memory_arena * Arena = RequestSlot->Arena;
	web_request * Request = &RequestSlot->Value;

	if (Request->BodyBuffer.Data == 0)
	{
		Request->BodyBuffer = BufferCreateOnArena(HTTP_REQUEST_PARSE_SIZE, Arena);
	}

	if (!Request->FrameHTTPMethodComplete)
	{
		// get next header line
		str8_bool32 HeaderLine = Str8ParseEatUntilStr8Match(RequestData, Str8Lit("\r\n"));
		if (HeaderLine.Bool == false)
		{
			return;
		}
		if (HeaderLine.String.Count > HTTP_REQUEST_PARSE_SIZE)
		{
			WebRequestRespondWithError(Request, 431);
			return;
		}

		i32 HTTPMethod = Str8ParseExpectAny(&HeaderLine.String, (str8[]) { Str8Lit("GET"), Str8Lit("POST") }, 2, MatchFlag_IgnoreCase);
		if (HTTPMethod == -1 || !Str8ParseExpect(&HeaderLine.String, Str8Lit(" "), MatchFlag_Normal))
		{
			WebRequestRespondWithError(Request, 400);
			return;
		}
		Request->RequestHTTPMethod = HTTPMethod;

		str8 Path = Str8ParseEatUntilChar(&HeaderLine.String, ' ');
		if (Path.Count == 0 || !Str8ParseExpect(&HeaderLine.String, Str8Lit(" "), MatchFlag_Normal))
		{
			WebRequestRespondWithError(Request, 400);
			return;
		}
		Request->RequestPath = ArenaPushStr8(Arena, Path);

		if (!Str8ParseExpect(&HeaderLine.String, Str8Lit("HTTP/1.1"), MatchFlag_Normal) && HeaderLine.String.Count == 0)
		{
			WebRequestRespondWithError(Request, 400);
			return;
		}

		Request->FrameHTTPMethodComplete = true;
	}

	if (!Request->FrameHeaderComplete)
	{
		while (true)
		{
			str8_bool32 HeaderLine = Str8ParseEatUntilStr8Match(RequestData, Str8Lit("\r\n"));
			if (HeaderLine.String.Count > HTTP_REQUEST_PARSE_SIZE)
			{
				WebRequestRespondWithError(Request, 431);
				return;
			}
			if (HeaderLine.Bool == false)
			{
				return;
			}
			if (HeaderLine.String.Count == 0)
			{
				Request->FrameHeaderComplete = true;
				break;
			}

			str8_bool32 HeaderKey = Str8ParseEatUntilStr8Match(&HeaderLine.String, Str8Lit(":"));
			if (HeaderKey.Bool == false)
			{
				WebRequestRespondWithError(Request, 400);
				return;
			}
			if (HeaderKey.String.Count > HTTP_REQUEST_PARSE_SIZE)
			{
				WebRequestRespondWithError(Request, 431);
				return;
			}

			str8 HeaderValue = Str8Trim(HeaderLine.String);

			if (Str8Match(HeaderKey.String, Str8Lit("Content-Length"), MatchFlag_IgnoreCase))
			{
				void * Extra = 0;
				Request->RequestContentLength = IntFromStr8(HeaderValue, Extra);
				Request->RequestHasContentLength = true;
				if (Extra)
				{
					WebRequestRespondWithError(Request, 400);
					return;
				}
				if (Request->RequestContentLength > HTTP_REQUEST_PARSE_SIZE)
				{
					WebRequestRespondWithError(Request, 413);
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
					WebRequestRespondWithError(Request, 426);
					return;
				}
			}

			if (Str8Match(HeaderKey.String, Str8Lit("Sec-Websocket-Key"), MatchFlag_IgnoreCase))
			{
				Request->RequestWebSocketKey = ArenaPushStr8(Arena, HeaderValue);
			}
		}
	}

	if (Request->RequestHTTPMethod == 1 && !Request->RequestHasContentLength)
	{
		WebRequestRespondWithError(Request, 411);
		return;
	}
	if (Request->RequestProtocolSwitch && Request->RequestWebSocketKey.Count == 0)
	{
		WebRequestRespondWithError(Request, 400);
		return;
	}
	if (Request->RequestContentLength == 0)
	{
		Request->FrameBodyComplete = true;
	}

	if (!Request->FrameBodyComplete)
	{
		if (RequestData->Count >= Request->RequestContentLength)
		{
			Str8WriteStr8(&Request->BodyBuffer, Str8ParseEat(RequestData, Request->RequestContentLength));
			Request->RequestBody = Str8FromBuffer(&Request->BodyBuffer);
			Request->FrameBodyComplete = true;
		}
		else
		{
			return;
		}
	}

	Request->FrameCount++;
	Request->RequestComplete = true;
}

void ParseWebsocketRequest(web_server * Server, str8 * RequestData, web_request_slot * RequestSlot)
{
	memory_arena * Arena = RequestSlot->Arena;
	web_request * Request = &RequestSlot->Value;

	if (Request->BodyBuffer.Data == 0)
	{
		Request->BodyBuffer = BufferCreateOnArena(WEBSOCKET_BODY_TOTAL_SIZE, Arena);
	}

	while (!Request->RequestComplete)
	{
		Request->FrameHTTPMethodComplete = true;

		if (!Request->FrameHeaderComplete)
		{
			if (RequestData->Count < 2)
			{
				return;
			}

			u64 Cursor = 0;

			bool32 FinalFrame = RequestData->Data[Cursor] >> 7;
			u32 Opcode = RequestData->Data[Cursor] & 0xf;

			Cursor++;

			bool32 Masked = RequestData->Data[Cursor] >> 7;
			u64 Length = RequestData->Data[Cursor] & ~0x80;

			Cursor++;

			if (Length == 126)
			{
				if (RequestData->Count < Cursor + 2)
				{
					return;
				}

				Length = (RequestData->Data[Cursor] << 8) | RequestData->Data[Cursor + 1];
				Cursor += 2;
			}
			else if (Length == 127)
			{
				if (RequestData->Count < Cursor + 4)
				{
					return;
				}

				Length =
					(RequestData->Data[Cursor] << 24) | (RequestData->Data[Cursor + 1] << 16) |
					(RequestData->Data[Cursor + 2] << 8) | RequestData->Data[Cursor + 3];
				Cursor += 4;
			}

			if (Length > WEBSOCKET_BODY_TOTAL_SIZE)
			{
				WebRequestRespondWithError(Request, 431);
			}

			u32 Mask = 0;
			if (Masked)
			{
				if (RequestData->Count < Cursor + 4)
				{
					return;
				}
				Mask = *((u32 *) &RequestData->Data[Cursor]);

				Cursor += 4;
			}

			Request->FrameHeaderComplete = true;

			Str8ParseEat(RequestData, Cursor);

			Request->Mask = Mask;
			Request->HasMoreFrames = !FinalFrame;
			Request->RequestContentLength = Length;
			Request->RequestHasContentLength = Length != 0;
			if (Request->FrameCount == 0)
			{
				Request->RequestHTTPMethod = Opcode;
			}
		}

		if (!Request->FrameBodyComplete)
		{
			if (RequestData->Count >= Request->RequestContentLength)
			{
				char8 * StartIndex = BufferAt(&Request->BodyBuffer);
				Str8WriteStr8(&Request->BodyBuffer, Str8ParseEat(RequestData, Request->RequestContentLength));
				char8 * EndIndex = BufferAt(&Request->BodyBuffer);

				for (char8 * Char = StartIndex; Char <= EndIndex; Char++)
				{
					u8 MaskPosition = ((Char - StartIndex) % 4) * 8;
					*Char ^= (Request->Mask & (0xff << MaskPosition)) >> MaskPosition;
				}

				Request->FrameBodyComplete = true;
			}
			else
			{
				return;
			}
		}

		Request->FrameCount++;

		if (Request->HasMoreFrames)
		{
			Request->RequestContentLength = 0;
			Request->RequestHasContentLength = false;
			Request->Mask = 0;
			Request->HasMoreFrames = 0;
			Request->FrameHTTPMethodComplete = false;
			Request->FrameHeaderComplete = false;
			Request->FrameBodyComplete = false;
			continue;
		}
		else
		{
			Request->RequestBody = Str8FromBuffer(&Request->BodyBuffer);
		}

		Request->RequestComplete = true;
	}
}

void WebRequestRespondWithError(web_request * Request, u32 Code)
{
	Request->ResponseCode = Code;
	Request->ResponseBehavior = ResponseBehavior_RespondClose;
	Request->Status = WebRequest_Processed;
}

web_request * ServerNextRequest(web_server * Server)
{
	for (i32 I = 0; I < WEB_REQUEST_COUNT; I++)
	{
		web_request_slot * Request = &Server->Requests[I];

		if (Request->Value.Status == WebRequest_Ready)
		{
			Request->Value.Status = WebRequest_Processed;

			web_connection_slot * Connection = &Server->Connections[Request->Value.ConnectionIndex];

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

str8 WebsocketReasonName(u16 Reason)
{
	switch (Reason)
	{
	case 1000: return Str8Lit("Closing");
	case 1001: return Str8Lit("Going Away");
	case 1002: return Str8Lit("Protcol Error");
	case 1003: return Str8Lit("Unsupported Data");

	case 1007: return Str8Lit("Invalid Payload");
	case 1008: return Str8Lit("Policy Violated");
	case 1009: return Str8Lit("Message Too Big");
	case 1010: return Str8Lit("Unsupported extension");
	case 1011: return Str8Lit("Internal Server Error");

	default: return Str8Lit("Unknown");
	}
}