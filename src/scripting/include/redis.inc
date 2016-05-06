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
	new authCommand[512], fmtAuthCommand[512], authCommandSize, authBuffer[512]

	formatex(authCommand, 511, "AUTH %s", REDIS_PASS)
	fmtAuthCommandSize = format_command_resp(authCommand, strlen(authCommand), fmtAuthCommand, sizeof(fmtAuthCommand) - 1)

	//client_print(0, print_console, "Authenticate: %s", authCommand)

	redis_send(fmtAuthCommand, fmtAuthCommandSize)
	redis_receive(authBuffer)

	return equal(authBuffer, "+OK^r^n")
}

public redis_publish(channel[], data[])
{
	new publishCommand[512], fmtPublishCommand[512], publishCommandSize, publishBuffer[512]

	formatex(publishCommand, 511, "PUBLISH %s %s", channel, data)
	fmtPublishCommandSize = format_command_resp(publishCommand, strlen(publishCommand), fmtPublishCommand, sizeof(fmtPublishCommand) - 1)

	//client_print(0, print_console, "Publish: %s", fmtPublishCommand)

	if (redis_connect())
	{
		if (redis_authenticate())
		{
			redis_send(fmtPublishCommand, fmtPublishCommandSize)
			redis_receive(publishBuffer)

			return !equal(publishBuffer[0], ":")
		}

		redis_disconnect()
	}

	return false
}

public redis_receive(buffer[])
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

format_command_resp(commandString[], commandStringLen, commandBuffer[], commandBufferSize)
{
	new pos, arg[240], fmtArg[256]
	new i = 0

	while ((pos = argparse(commandString, pos, arg, sizeof(arg) - 1)) > -1)
	{
		i++
		formatex(fmtArg, sizeof(fmtArg) - 1, "$%d^r^n%s^r^n", strlen(arg), arg)
		add(commandBuffer, commandBufferSize, fmtArg, strlen(fmtArg))
	}

	formatex(commandBuffer, commandBufferSize, "*%d^r^n%s", i, commandBuffer)

	return strlen(commandBuffer)
}
