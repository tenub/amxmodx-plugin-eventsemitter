#if defined _redis_included
	#endinput
#endif
#define _redis_included

#include <amxmodx>
#include "sockets_hz"

#include ".env.inc"

new const CRLF[] = "^r^n"
new g_redis_socket

public redis_connect()
{
	new error

	g_redis_socket = socket_open(REDIS_HOST, REDIS_PORT, SOCKET_TCP, error)

	if (g_redis_socket <= 0)
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

	return g_redis_socket
}

public redis_authenticate()
{
	static command[512], commandSize, recvBuffer[512]
	commandSize = format_command_resp(command, 511, "AUTH", REDIS_PASS)

	//client_print(0, print_console, "Authenticate: %s", authCommand)

	redis_send(command, commandSize)
	redis_receive(recvBuffer)

	return equal(recvBuffer, "+OK^r^n")
}

public redis_publish(channel[], data[])
{
	static command[512], commandSize, recvBuffer[512]
	commandSize = format_command_resp(command, 511, "PUBLISH", channel, data)

	//client_print(0, print_console, "Publish: %s", command)

	if (redis_connect())
	{
		if (redis_authenticate())
		{
			redis_send(command, commandSize)
			redis_receive(recvBuffer)

			return !equal(recvBuffer[0], ":")
		}

		redis_disconnect()
	}

	return false
}

public redis_receive(buffer[512])
{
	if (socket_change(g_redis_socket))
	{
		socket_recv(g_redis_socket, buffer, sizeof(buffer) - 1)
	}

	//client_print(0, print_console, "Received: %s", buffer)
}

public redis_send(command[], commandSize)
{
	socket_send(g_redis_socket, command, commandSize)
}

public redis_disconnect()
{
	socket_close(g_redis_socket)
}

format_command_resp(buffer[], const bufferSize, ...)
{
	static arg[256], num[8]
	new numArgs = numargs()

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
