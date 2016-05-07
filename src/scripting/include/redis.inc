#if defined _redis_included
	#endinput
#endif
#define _redis_included

#include <amxmodx>
#include <sockets_hz>

#include ".env.inc"

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
	static args[4][256], fmtBuffer[2048], recvBuffer[512]
	new fmtBufferLen

	copy(args[0], 255, REDIS_PASS)

	fmtBufferLen = format_command_resp("AUTH", args, fmtBuffer)

	//server_print(0, print_console, "Authenticate: %s", fmtBuffer)

	redis_send(fmtBuffer, fmtBufferLen)
	redis_receive(recvBuffer)

	return equal(recvBuffer, "+OK^r^n")
}

public redis_publish(channel[64], data[512])
{
	static args[4][256], fmtBuffer[2048], recvBuffer[512]
	new fmtBufferLen

	copy(args[0], 255, channel)
	copy(args[1], 255, data)

	fmtBufferLen = format_command_resp("PUBLISH", args, fmtBuffer)

	//server_print(0, print_console, "Publish: %s", fmtBuffer)

	if (redis_connect())
	{
		if (redis_authenticate())
		{
			redis_send(fmtBuffer, fmtBufferLen)
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

	//server_print(0, print_console, "Received: %s", buffer)
}

public redis_send(command[2048], commandLen)
{
	socket_send(g_redis_socket, command, commandLen)
}

public redis_disconnect()
{
	socket_close(g_redis_socket)
}

format_command_resp(command[], args[4][256], fmtBuffer[2048])
{
	new fmtArg[320], tmpBuffer[2048], nArgs = 1

	formatex(tmpBuffer, 2047, "$%d^r^n%s^r^n", strlen(command), command)

	for (new i = 0; i < 4; i++)
	{
		if (strlen(args[i]))
		{
			nArgs++
			formatex(fmtArg, 319, "$%d^r^n%s^r^n", strlen(args[i]), args[i])
			add(tmpBuffer, 2047, fmtArg, strlen(fmtArg))
		}
	}

	formatex(fmtBuffer, 2047, "*%i^r^n%s", nArgs, tmpBuffer)

	return strlen(fmtBuffer)
}
