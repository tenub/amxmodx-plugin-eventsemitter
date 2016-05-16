#if defined _resp_included
	#endinput
#endif
#define _resp_included

new const CRLF[] = "^r^n";

/**
 * Format a variable number of arguments into a string buffer
 * for sending over redis connection using the RESP format
 *
 * @param string buffer
 * @param integer bufferSize
 * @param string arg,...
 *
 * @return integer Length of string buffer
 */
format_command_resp(buffer[], const bufferSize, ...)
{
	new numArgs = numargs();

	// do not format unless there are arguments
	if (numArgs <= 2)
	{
		return false;
	}

	static num[8], arg[256];

	num_to_str(numArgs - 2, num, 7);
	add(buffer, bufferSize, "*");
	add(buffer, bufferSize, num);
	add(buffer, bufferSize, CRLF);

	new pos, chr;

	for (new i = 2; i < numArgs; ++i)
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
		add(buffer, bufferSize, "$");
		add(buffer, bufferSize, num);
		add(buffer, bufferSize, CRLF);
		add(buffer, bufferSize, arg);
		add(buffer, bufferSize, CRLF);
	}

	return strlen(buffer);
}

/**
 * Read and parse data from RESP format
 * Returns data type:
 * 0 - error
 * 1 - integer
 * 2 - simple string
 * 3 - bulk string
 * 4 - array
 *
 * @param string data
 * @param string message
 *
 * @return integer
 */
stock parse_resp_data(data[], message[])
{
	new respType, error[512], errorSize = sizeof(error);

	switch (data[0])
	{
		case '+': { respType = parse_resp_simple_string(data[1], message); }
		//case '$': { respType = parse_resp_bulk_string(data[1], message, error, errorSize); }
		//case '*': { respType = parse_resp_array(data[1], message, error, errorSize); }
		case ':': { respType = parse_resp_integer(data[1], message, error, errorSize); }
		case '-': { respType = parse_resp_error(data[1], message, error, errorSize); }
	}

	if (respType == 0)
	{
		server_print(error);
	}

	return respType;
}

stock parse_resp_simple_string(data[], message[])
{
	new i;

	while (data[i])
	{
		if (data[i] == 13 && data[i + 1] == 10)
		{
			break;
		}

		add(message, 510, data[i], 1);
		i++;
	}

	return 2;
}

// 6\r\nstring\r\n
stock parse_resp_bulk_string(data[], message[], error[], errorSize) {}

stock parse_resp_array(data[], message[], error[], errorSize) {}

stock parse_resp_integer(data[], message[], error[], errorSize)
{
	new i = 0;

	while (data[i])
	{
		if (data[i] == 13 && data[i + 1] == 10)
		{
			break;
		}
		else if (data[i] < 48 || data[i] > 57)
		{
			message[0] = 0;
			copy(error, errorSize, "Non-integer response");

			return 0;
		}

		add(message, 510, data[i], 1);
		i++;
	}

	return 1;
}

stock parse_resp_error(data[], message[], error[], errorSize)
{
	parse_resp_simple_string(data, message);
	copy(error, errorSize, message);

	return 0;
}
