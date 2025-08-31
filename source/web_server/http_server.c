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
		Result.Connections[I].ParsingArena = Arena;
		Result.Connections[I].Unparsed = BufferCreateOnArena(Kilobytes(1), Arena);
		Result.Connections[I].Initial = TempArenaCreate(Arena);

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

void ServerLoop(http_server * Server)
{
	// send responses for requests the application code has processed

	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		http_request_slot * Slot = &Server->Requests[I];
		http_request * Request = &Slot->Value;

		http_connection * Connection = Request->Valid ? &Server->Connections[Request->ConnectionIndex].Value : 0;
		if (!Request->Valid || !Connection->Valid)
		{
			CloseRequest(Slot);
			continue;
		}

		if (Request->Processed)
		{
			if (Request->ResponseBehavior & ResponseBehavior_Respond)
			{
				memory_buffer * Buffer = ScratchBufferStart();

				Str8WriteFmt(Buffer, "HTTP/1.1 %{u16} %{str8}\r\n", Request->ResponseHTTPCode, HTTPReasonName(Request->ResponseHTTPCode));

				if (Request->ResponseBehavior && Request->ResponseHTTPCode >= 400)
				{
					Request->ResponseBehavior |= ResponseBehavior_Close;
				}

				if (!(Request->ResponseBehavior & ResponseBehavior_Close))
				{
					Str8WriteFmt(Buffer, "Connection: Keep-Alive\r\n");
				}

				if (Request->ResponseBody.Data)
				{
					Str8WriteFmt(Buffer, "Content-Length: %{u32}\r\n\r\n%{str8}", Request->ResponseBody.Count, Request->ResponseBody);
				}
				else
				{
					Str8WriteFmt(Buffer, "\r\n");
				}

				SocketOutput(Connection->Socket, Str8FromBuffer(Buffer));

				ScratchBufferRelease(Buffer);
			}

			if (Request->ResponseBehavior & ResponseBehavior_Close)
			{
				CloseConnection(Server, Connection, 0);
			}
			else if (Request->ResponseBehavior & ResponseBehavior_Respond)
			{
				Connection->ResponsesSent++;
			}

			CloseRequest(Slot);
		}
	}

	ArenaReset(Server->ResponseArena);
	
	// accept new connections

	socket_handle NewConnectionSocket = SocketGetInvalid();

	for (i32 I = 0; I < 16; I++)
	{
		if (SocketAccept(Server->ServerSocket, &NewConnectionSocket, 0))
		{
			if (!AddConnection(Server, NewConnectionSocket))
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
			memory_buffer * Workspace = &Server->ParsingWorkspace;
			BufferReset(Workspace);

			Str8WriteStr8(Workspace, Str8FromBuffer(Unparsed));
			SocketInputToBuffer(ConnectionInfo->Socket, Workspace);

			http_request_parser * Parser = &ConnectionInfo->RequestParser;

			str8 HTTPRequest = Str8FromBuffer(Workspace);

			while (true)
			{
				ParseHttpRequest(Server, Connection, &HTTPRequest);

				if (Parser->HTTPError)
				{
					break;
				}

				if (!Parser->BodyComplete)
				{
					break;
				}
				else
				{
					AddRequest(Server, Connection);
					OSZeroMemory(Parser, sizeof(http_request_parser));
				}
			}

			if (Parser->HTTPError)
			{
				CloseConnection(Server, Connection, Parser->HTTPError);
			}
		}
	}
}

http_request_slot * AddRequest(http_server * Server, http_connection_slot * Connection)
{
	http_request_slot * Request = 0;

	// find connection
	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		if (!Server->Requests[I].Value.Valid)
		{
			Request = &Server->Requests[I];
			break;
		}
	}

	if (Request == 0)
	{
		return 0;
	}

	http_request_parser * Parser = &Connection->Value.RequestParser;

	Request->Value.Valid = true;
	Request->Value.ConnectionIndex = Connection->Index;
	Request->Value.HTTPMethod = Parser->HTTPMethod;
	Request->Value.Path = ArenaPushStr8(Request->Arena, Parser->Path);
	Request->Value.Body = ArenaPushStr8(Request->Arena, Parser->Body);
	Request->Value.ResponseBehavior = ResponseBehavior_Ignore;

	return Request;
}

str8 HTTPReasonName(u16 Reason)
{
	switch (Reason)
	{
	case 200: return Str8Lit("Ok");
	case 204: return Str8Lit("No Content");

	case 400: return Str8Lit("Bad Request");
	case 404: return Str8Lit("Length Required");
	case 408: return Str8Lit("Content Too Large");
	case 411: return Str8Lit("Request Header Fields Too Large");
	case 413: return Str8Lit("Request Timeout");
	case 431: return Str8Lit("Not Found");

	case 500: return Str8Lit("Internal Server Error");

	default: return Str8Lit("Unknown");
	}
}

http_connection_slot * AddConnection(http_server * Server, socket_handle Socket)
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
	Connection->Value.RequestParser = (http_request_parser) { 0 };
	Connection->Value.LastCommunication = UnixTimeSec();

	SocketPollingAdd(Server->Polling, Socket, Connection->Index);

	return Connection;
}

bool8 CloseConnection(http_server * Server, http_connection_slot * Connection, u16 Reason)
{
	if (Reason)
	{
		str8 ReasonName = HTTPReasonName(Reason);
		SocketOutputFmt(Connection->Value.Socket, "HTTP/1.1 %{u16} %{str8}\r\nConnection: close\r\n\r\n", Reason, ReasonName);
	}

	SocketClose(Connection->Value.Socket);

	OSZeroMemory(&Connection->Value, sizeof(http_connection));
	ArenaZero(Connection->ParsingArena);
	ArenaReset(Connection->ParsingArena);

	SocketPollingRemove(Server->Polling, Connection->Index);

	return true;
}

bool8 CloseRequest(http_request_slot * RequestSlot)
{
	ArenaZero(RequestSlot->Arena);
	ArenaReset(RequestSlot->Arena);

	OSZeroMemory(&RequestSlot->Value, sizeof(http_request));

	return true;
}

void ParseHttpRequest(http_server * Server, http_connection_slot * Connection, str8 * HttpRequest)
{
	http_request_parser * Parser = &Connection->Value.RequestParser;
	memory_arena * Arena = Connection->ParsingArena;

	if (!Parser->MethodComplete)
	{
		// get next header line
		str8_bool32 HeaderLine = Str8ParseEatUntilStr8Match(HttpRequest, Str8Lit("\r\n"));
		if (HeaderLine.Bool == false)
		{
			return;
		}
		if (HeaderLine.String.Count > HTTP_REQUEST_PARSE_SIZE)
		{
			Parser->HTTPError = 431;
			return;
		}

		i32 HTTPMethod = Str8ParseExpectAny(&HeaderLine.String, (str8[]) { Str8Lit("GET"), Str8Lit("POST") }, 2, MatchFlag_IgnoreCase);
		if (HTTPMethod == -1 || !Str8ParseExpect(&HeaderLine.String, Str8Lit(" "), MatchFlag_Normal))
		{
			Parser->HTTPError = 400;
			return;
		}
		Parser->HTTPMethod = HTTPMethod;

		str8 Path = Str8ParseEatUntilChar(&HeaderLine.String, ' ');
		if (Path.Count == 0 || !Str8ParseExpect(&HeaderLine.String, Str8Lit(" "), MatchFlag_Normal))
		{
			Parser->HTTPError = 400;
			return;
		}
		Parser->Path = ArenaPushStr8(Arena, Path);

		if (!Str8ParseExpect(&HeaderLine.String, Str8Lit("HTTP/1.1"), MatchFlag_Normal) && HeaderLine.String.Count == 0)
		{
			Parser->HTTPError = 400;
			return;
		}

		Parser->MethodComplete = true;
	}

	if (!Parser->HeadersComplete)
	{
		while (true)
		{
			str8_bool32 HeaderLine = Str8ParseEatUntilStr8Match(HttpRequest, Str8Lit("\r\n"));
			if (HeaderLine.Bool == false)
			{
				return;
			}
			if (HeaderLine.String.Count > HTTP_REQUEST_PARSE_SIZE)
			{
				Parser->HTTPError = 431;
				return;
			}
			if (HeaderLine.String.Count == 0)
			{
				Parser->HeadersComplete = true;
				break;
			}

			str8_bool32 HeaderKey = Str8ParseEatUntilStr8Match(&HeaderLine.String, Str8Lit(":"));
			if (HeaderKey.Bool == false)
			{
				Parser->HTTPError = 400;
				return;
			}
			if (HeaderKey.String.Count > HTTP_REQUEST_PARSE_SIZE)
			{
				Parser->HTTPError = 413;
				return;
			}

			if (Str8Match(HeaderKey.String, Str8Lit("Content-Length"), MatchFlag_IgnoreCase))
			{
				void * Extra = 0;
				Parser->ContentLength = IntFromStr8(HeaderLine.String, Extra);
				Parser->ContentLengthComplete = true;
				if (Extra)
				{
					Parser->HTTPError = 400;
					return;
				}
				if (Parser->ContentLength > HTTP_REQUEST_PARSE_SIZE)
				{
					Parser->HTTPError = 413;
					return;
				}
			}
		}
	}

	if (Parser->HTTPMethod == 1 && !Parser->ContentLengthComplete)
	{
		Parser->HTTPError = 411;
		return;
	}
	if (Parser->ContentLength == 0)
	{
		Parser->BodyComplete = true;
	}

	if (!Parser->BodyComplete)
	{
		if (HttpRequest->Count < Parser->ContentLength)
		{
			return;
		}
		else
		{
			str8 Body = Str8ParseEat(HttpRequest, Parser->ContentLength);
			Parser->Body = ArenaPushStr8(Arena, Body);
			Parser->BodyComplete = true;
			return;
		}
	}
}

http_request * ServerNextRequest(http_server * Server)
{
	for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
	{
		http_request_slot * Request = &Server->Requests[I];

		if (Request->Value.Valid && !Request->Value.Processed)
		{
			Request->Value.Processed = true;

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

/*
typedef struct hollyoak_state {
	u64 Timestamp;

	u8 Hands[45];
	u8 CurrentTrick[5];

	u8 Scores[3]; // Scores[0] is P1 and P2, Scores[1] is P3 and P4, Scores[2] is P5

	u8 DealingPlayerAndActivePlayer;
	u8 RoundAndTrick;
	u8 CurrentSeason;
} hollyoak_state;

typedef struct hollyoak_history {
	hollyoak_state History[128];
} hollyoak_history;
*/
