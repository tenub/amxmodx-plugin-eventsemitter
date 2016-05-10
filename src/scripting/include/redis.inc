#if defined _redis_included
	#endinput
#endif
#define _redis_included

#include <amxmodx>
#include "sockets_hz"

#include ".env.inc"

new const CRLF[] = "^r^n"
new g_RedisSocket, g_RedisSocketListen

/**
 * Called by "redis_publish" to open a TCP connection with a remote server
 *
 * @return integer Socket
 */
public redis_connect()
{
	new error

	g_RedisSocket = socket_open(REDIS_HOST, REDIS_PORT, SOCKET_TCP, error)

	if (g_RedisSocket <= 0)
	{
		switch (error)
		{
			case 1: {
				server_print("Error creating socket")
			}
			case 2: {
				server_print("Error resolving remote hostname")
			}
			case 3: {
				server_print("Error connecting socket")
			}
		}
	}

	return g_RedisSocket
}

/**
 * Called by "redis_subscribe" to open a listening TCP connection with a remote server
 *
 * @return integer Socket
 */
public redis_listen()
{
	new error

	g_RedisSocketListen = socket_open(REDIS_HOST, REDIS_PORT, SOCKET_TCP, error)

	if (g_RedisSocketListen <= 0)
	{
		switch (error)
		{
			case 1: {
				server_print("Error creating listen socket")
			}
			case 2: {
				server_print("Error resolving remote hostname for listen socket")
			}
			case 3: {
				server_print("Error connecting listen socket")
			}
		}
	}

	return g_RedisSocketListen
}

/**
 * Called by "redis_publish" to authenticate with the redis server
 *
 * @return boolean Success or failure from authentication attempt
 */
public redis_authenticate()
{
	static command[512], commandSize, recvBuffer[512]

	commandSize = format_command_resp(command, 511, "AUTH", REDIS_PASS)

	redis_send(g_RedisSocket, command, commandSize)
	redis_receive(g_RedisSocket, recvBuffer, 511)

	// check if redis server responds with success message
	return equal(recvBuffer, "+OK^r^n")
}

/**
 * Send a publish command to the redis server with supplied channel and message data
 *
 * @param string channel
 * @param string data
 *
 * @return boolean False on redis error
 */
public redis_publish(channel[], data[])
{
	static command[512], commandSize, recvBuffer[512]

	commandSize = format_command_resp(command, 511, "PUBLISH", channel, data)

	if (redis_connect())
	{
		if (redis_authenticate())
		{
			redis_send(g_RedisSocket, command, commandSize)
			redis_receive(g_RedisSocket, recvBuffer, 511)

			// return false if response is an error, true otherwise
			return recvBuffer[0] != '-')
		}

		redis_disconnect(g_RedisSocket)
	}

	return false
}

/**
 * Open a listening connection and subscribe to supplied redis channel
 *
 * @param string channel
 *
 * @return boolean False on redis error
 */
public redis_subscribe(channel[])
{
	static listenBuffer[512], msgType[32], msgChannel[64], msgText[512]

	if (redis_listen())
	{
		if (redis_authenticate())
		{
			redis_receive(g_RedisSocketListen, listenBuffer, 511)
			parse_resp_data(listenBuffer, 511, msgType, msgChannel, msgText)

			// do something with received data
		}
	}

	return g_RedisSocketListen
}

/**
 * Release a redis connection
 * Unsubscribe from all redis channels and close the connection
 *
 * @param integer socket
 *
 * @return boolean False on redis error
 */
public redis_release(socket)
{
	static command[512], commandSize, recvBuffer[512]

	commandSize = format_command_resp(command, 511, "UNSUBSCRIBE")

	if (g_RedisSocketListen > 0)
	{
		redis_send(g_RedisSocketListen, command, commandSize)
		redis_receive(g_RedisSocketListen, recvBuffer, 511)

		if (recvBuffer[0] == '-')
		{
			server_print("Error unsubscribing")
		}

		redis_disconnect(g_RedisSocketListen)
	}
}

/**
 * Receive a response from the redis server into a string buffer through specified socket
 *
 * @param integer socket
 * @param string buffer
 * @param integer bufferSize
 */
public redis_receive(socket, buffer[], const bufferSize)
{
	if (socket_change(socket))
	{
		socket_recv(socket, buffer, bufferSize)
	}
}

/**
 * Send a formatted string over a redis connection through specified socket
 *
 * @param integer socket
 * @param string buffer
 * @param integer commandSize
 */
public redis_send(socket, buffer[], const bufferSize)
{
	socket_send(socket, buffer, bufferSize)
}

/**
 * Close a redis connection through specified socket
 *
 * @param integer socket
 */
public redis_disconnect(socket)
{
	socket_close(socket)
}

/**
 * Format a variable number of arguments into a string buffer (for sending over redis connection)
 * Prepare arguments into a single string using the RESP format
 *
 * @param string buffer
 * @param integer bufferSize
 * @param string arg,...
 *
 * @return integer Length of string buffer
 */
format_command_resp(buffer[], const bufferSize, ...)
{
	new numArgs = numargs()

	// do not format unless there are arguments
	if (numArgs <= 2)
	{
		return false
	}

	static num[8], arg[256]

	buffer[0] = 0
	num_to_str(numArgs - 2, num, 7)
	add(buffer, bufferSize, "*")
	add(buffer, bufferSize, num)
	add(buffer, bufferSize, CRLF)

	new pos, chr

	for (new i = 2; i < numArgs; ++i)
	{
		pos = 0, chr = 0

		while ((chr = getarg(i, pos)))
		{
			arg[pos++] = chr

			if (pos >= 255)
			{
				break
			}
		}

		arg[pos++] = 0
		num_to_str(strlen(arg), num, 7)
		add(buffer, bufferSize, "$")
		add(buffer, bufferSize, num)
		add(buffer, bufferSize, CRLF)
		add(buffer, bufferSize, arg)
		add(buffer, bufferSize, CRLF)
	}

	return strlen(buffer)
}

/**
 * Read and parse data in RESP format
 *
 * @param string buffer
 * @param integer bufferSize

 *
 * @return string
 */
parse_resp_data(buffer[], const bufferSize, type[], channel[], text[]) {
	static data[512]
	new arg[512]

	switch (buffer[0])
	{
		case '+': {
			arg = trim(buffer[1])
		}
		case '-': {
			arg = trim(buffer[1])
		}
		case ':': {
			arg = str_to_num(trim(buffer[1]))
		}
		case '$': {
			//
		}
		case '*': {
			for (new i = 1; i < bufferSize; i++)
			{
				if (equal(arg, CRLF))
				{

					arg[0] = 0
				}

				else
				{
					arg[i] = buffer[i]
				}
			}
		}
		default: return false
	}
}
