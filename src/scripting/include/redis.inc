#if defined _redis_included
	#endinput
#endif
#define _redis_included

#include <amxmodx>
#include <sockets_hz>

#include ".env.inc"
#include "resp.inc"

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

	g_RedisSocketListen = socket_listen(REDIS_HOST, REDIS_PORT, SOCKET_TCP, error)

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
	static command[512], commandSize, recvBuffer[512], response[1][512], responseSuccess

	commandSize = format_command_resp(command, 511, "AUTH", REDIS_PASS)

	redis_send(g_RedisSocket, command, commandSize)
	redis_receive(g_RedisSocket, recvBuffer, 511)
	responseSuccess = parse_resp_data(recvBuffer, 511, response, 1, 511)

	// check if redis server responds with success message
	return responseSuccess && equal(response, "OK")
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
	static command[512], commandSize, recvBuffer[512], response[1][512], responseSuccess

	commandSize = format_command_resp(command, 511, "PUBLISH", channel, data)

	if (redis_connect())
	{
		if (redis_authenticate())
		{
			redis_send(g_RedisSocket, command, commandSize)
			redis_receive(g_RedisSocket, recvBuffer, 511)
			responseSuccess = parse_resp_data(recvBuffer, 511, response, 1, 511)
		}

		redis_disconnect(g_RedisSocket)
	}

	return responseSuccess
}

/**
 * Open a listening connection and subscribe to supplied redis channel
 * Dump data to listenBuffer
 *
 * @param string channel
 * @param string msgType
 * @param string msgChannel
 * @param string msgText
 *
 * @return boolean False on redis error
 */
/*public redis_subscribe(channel[], msgType[], msgChannel[], msgText[])
{
	static command[512], commandSize, listenBuffer[512]

	commandSize = format_command_resp(command, 511, "SUBSCRIBE", channel)

	if (redis_listen())
	{
		if (redis_authenticate())
		{
			redis_send(g_RedisSocketListen, command, commandSize)
			redis_receive(g_RedisSocketListen, listenBuffer, 511)
			parse_resp_data(listenBuffer, 511, msgType, msgChannel, msgText)
		}
	}

	return g_RedisSocketListen
}*/

/**
 * Release a redis connection
 * Unsubscribe from all redis channels and close the connection
 *
 * @param integer socket
 *
 * @return boolean False on redis error
 */
/*public redis_release(socket)
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
}*/

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
