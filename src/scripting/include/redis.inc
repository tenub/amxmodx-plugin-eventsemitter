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
	authCommandSize = format_command_resp(authCommand, fmtAuthCommand)

	//client_print(0, print_console, "Authenticate: %s", authCommand)

	redis_send(fmtAuthCommand, authCommandSize)
	redis_receive(authBuffer)

	return equali(authBuffer, "+OK^r^n")
}

public redis_publish(channel[], data[])
{
	new publishCommand[512], fmtPublishCommand[512], publishCommandSize, publishBuffer[512]

	formatex(publishCommand, 511, "PUBLISH %s %s", channel, data)
	publishCommandSize = format_command_resp(publishCommand, fmtPublishCommand)

	//client_print(0, print_console, "Publish: %s", fmtPublishCommand)

	if (redis_connect())
	{
		if (redis_authenticate())
		{
			redis_send(fmtPublishCommand, publishCommandSize)
			redis_receive(publishBuffer)

			return !equali(publishBuffer[0], ":")
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

public redis_send(command[512], commandSize)
{
	socket_send(g_redis_socket, command, commandSize)
}

public redis_disconnect()
{
	socket_close(g_redis_socket)
}

format_command_resp(commandString[512], fmtCommandString[512])
{
	new pos, arg[240], fmtArg[256]
	new i = 0

	while ((pos = argparse(commandString, pos, arg, sizeof(arg) - 1)) > -1)
	{
		i++
		formatex(fmtArg, sizeof(fmtArg) - 1, "$%d^r^n%s^r^n", strlen(arg), arg)
		add(fmtCommandString, sizeof(fmtCommandString) - 1, fmtArg, strlen(fmtArg))

		client_print(0, print_console, "fmtArg: %s", fmtArg)
	}

	formatex(fmtCommandString, sizeof(fmtCommandString) - 1, "*%d^r^n%s", i, commandString)

	return strlen(fmtCommandString)
}

/*format_command_resp(commandString[512])
{
	new pos, arg[240], commandArray[4][256], commandArraySize
	new i = 0

	while ((pos = argparse(commandString, pos, arg, sizeof(arg) - 1)) > -1)
	{
		formatex(commandArray[i], 255, "$%d^r^n%s", strlen(arg), arg)
		client_print(0, print_console, "arg: %s", arg)
		i++
	}

	implode_strings(commandArray, commandArraySize, "^r^n", commandString, 511)
	formatex(commandString, sizeof(commandString) - 1, "*%d^r^n%s^r^n", i, commandString)

	return strlen(commandString)
}*/

/*format_command_resp(commandString[512])
{
	new commandArray[4][256], commandArraySize

	commandArraySize = explode_string(commandString, " ", commandArray, 4, 255)

	for (new i = 0; i < commandArraySize; i++)
	{
		format(commandArray[i], 255, "$%d^r^n%s", strlen(commandArray[i]), commandArray[i])
	}

	implode_strings(commandArray, commandArraySize, "^r^n", commandString, 511)
	format(commandString, 511, "*%d^r^n%s^r^n", commandArraySize, commandString)

	return strlen(commandString)
}*/
