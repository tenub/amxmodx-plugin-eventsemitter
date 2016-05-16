#if defined _redis_included
	#endinput
#endif
#define _redis_included

#include <amxmodx>
#include <sockets_hz>
#include <resp>
#include <.env>

new g_RedisSocket, g_RedisSocketListen;

/**
 * Send authentication command using specified socket
 *
 * @param integer socket
 *
 * @return boolean If the command was successful
 */
stock redis_authenticate(socket)
{
	static sendBuffer[512], sendBufferSize = sizeof(sendBuffer) - 1, sendBufferLen;
	static recvBuffer[512], recvBufferSize = sizeof(recvBuffer) - 1;
	new respType, msgBuffer[512];

	sendBuffer[0] = 0, recvBuffer[0] = 0;

	sendBufferLen = format_command_resp(sendBuffer, sendBufferSize, "AUTH", REDIS_PASS);

	redis_send(socket, sendBuffer, sendBufferLen);
	redis_receive(socket, recvBuffer, recvBufferSize);

	respType = parse_resp_data(recvBuffer, msgBuffer);

	return respType == 2 && equal(msgBuffer, "OK");
}

/**
 * Format and send a RESP command to the redis server
 * Response received into provided buffer
 *
 * @param string msgBuffer
 * @param string arg,...
 *
 * @return integer Type of data of the response
 */
stock redis_send_command(msgBuffer[], any:...)
{
	new numArgs = numargs();

	if (numArgs < 2)
	{
		return false;
	}

	static sendBuffer[512], sendBufferSize = sizeof(sendBuffer) - 1;
	static recvBuffer[512], recvBufferSize = sizeof(recvBuffer) - 1;
	static respType;
	static num[8], arg[256];

	sendBuffer[0] = 0, recvBuffer[0] = 0, msgBuffer[0] = 0;

	num_to_str(numArgs - 1, num, 7);
	add(sendBuffer, sendBufferSize, "*");
	add(sendBuffer, sendBufferSize, num);
	add(sendBuffer, sendBufferSize, CRLF);

	new pos, chr;

	for (new i = 1; i < numArgs; ++i)
	{
		pos = 0, chr = 0;

		while ((chr = getarg(i, pos)))
		{
			arg[pos++] = chr;

			if (pos >= 255)
			{
				break;
			}
		}

		arg[pos++] = 0;
		num_to_str(strlen(arg), num, 7);
		add(sendBuffer, sendBufferSize, "$");
		add(sendBuffer, sendBufferSize, num);
		add(sendBuffer, sendBufferSize, CRLF);
		add(sendBuffer, sendBufferSize, arg);
		add(sendBuffer, sendBufferSize, CRLF);
	}

	if (redis_connect())
	{
		if (redis_authenticate(g_RedisSocket))
		{
			redis_send(g_RedisSocket, sendBuffer, strlen(sendBuffer));
			redis_receive(g_RedisSocket, recvBuffer, recvBufferSize);

			respType = parse_resp_data(recvBuffer, msgBuffer);
		}

		redis_disconnect(g_RedisSocket);
	}

	return respType;
}

/**
 * Open a listening connection and subscribe to supplied redis channel
 * Response received into provided buffer
 *
 * @param string msgBuffer
 * @param string channel
 *
 * @return boolean False on redis error
 */
stock redis_subscribe(msgBuffer[], const channel[])
{
	static sendBuffer[512], sendBufferSize = sizeof(sendBuffer) - 1, sendBufferLen;
	static recvBuffer[512], recvBufferSize = sizeof(recvBuffer) - 1;

	sendBuffer[0] = 0, recvBuffer[0] = 0, msgBuffer[0] = 0;

	sendBufferLen = format_command_resp(sendBuffer, sendBufferSize, "SUBSCRIBE", channel);

	if (redis_listen())
	{
		if (redis_authenticate(g_RedisSocketListen))
		{
			redis_send(g_RedisSocketListen, sendBuffer, sendBufferLen);
			redis_receive(g_RedisSocketListen, recvBuffer, recvBufferSize);
			parse_resp_data(recvBuffer, msgBuffer);
		}
	}

	return g_RedisSocketListen;
}

/**
 * Release the redis listen connection
 * Unsubscribe from all redis channels and close the connection
 */
stock redis_release()
{
	static sendBuffer[512], sendBufferSize = sizeof(sendBuffer) - 1, sendBufferLen;
	static recvBuffer[512], recvBufferSize = sizeof(recvBuffer) - 1;
	new msgBuffer[512];

	sendBuffer[0] = 0, recvBuffer[0] = 0, msgBuffer[0] = 0;

	sendBufferLen = format_command_resp(sendBuffer, sendBufferSize, "UNSUBSCRIBE");

	if (g_RedisSocketListen)
	{
		redis_send(g_RedisSocketListen, sendBuffer, sendBufferLen);
		redis_receive(g_RedisSocketListen, recvBuffer, recvBufferSize);

		if (!parse_resp_data(recvBuffer, msgBuffer))
		{
			server_print("Error unsubscribing");
		}

		redis_disconnect(g_RedisSocketListen);
	}
}

/**
 * Open a TCP connection with a remote redis server
 *
 * @return integer Socket
 */
stock redis_connect()
{
	new error;

	g_RedisSocket = socket_open(REDIS_HOST, REDIS_PORT, SOCKET_TCP, error);

	if (g_RedisSocket <= 0)
	{
		switch (error)
		{
			case 1: { server_print("Error creating socket"); }
			case 2: { server_print("Error resolving remote hostname"); }
			case 3: { server_print("Error connecting socket"); }
		}
	}

	return g_RedisSocket;
}

/**
 * Open a TCP listen connection with a remote redis server
 *
 * @return integer Socket
 */
stock redis_listen()
{
	new error;

	g_RedisSocketListen = socket_listen(REDIS_HOST, REDIS_PORT, SOCKET_TCP, error);

	if (g_RedisSocketListen <= 0)
	{
		switch (error)
		{
			case 1: { server_print("Error creating listen socket"); }
			case 2: { server_print("Error resolving remote hostname for listen socket"); }
			case 3: { server_print("Error connecting listen socket"); }
		}
	}

	return g_RedisSocketListen;
}

/**
 * Send a formatted string over a redis connection through specified socket
 *
 * @param integer socket
 * @param string buffer
 * @param integer commandSize
 */
stock redis_send(socket, buffer[], const bufferSize)
{
	socket_send(socket, buffer, bufferSize);
}

/**
 * Receive a response from the redis server into a string buffer through specified socket
 *
 * @param integer socket
 * @param string buffer
 * @param integer bufferSize
 */
stock redis_receive(socket, buffer[], const bufferSize)
{
	if (socket_change(socket))
	{
		socket_recv(socket, buffer, bufferSize);
	}
}

/**
 * Close a redis connection through specified socket
 *
 * @param integer socket
 */
stock redis_disconnect(socket)
{
	socket_close(socket);
}
