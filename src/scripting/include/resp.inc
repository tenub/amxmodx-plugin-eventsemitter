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
format_command_resp(buffer[], const bufferSize, Array:args, numArgs)
{
	if (!numArgs)
	{
		return false;
	}

	static num[8], arg[256];

	buffer[0] = 0;
	num_to_str(numArgs, num, 7);

	add(buffer, bufferSize, "*");
	add(buffer, bufferSize, num);
	add(buffer, bufferSize, CRLF);

	for (new i = 0; i < numArgs; ++i)
	{
		ArrayGetString(args, i, arg, 255);
		num_to_str(strlen(arg), num, 7);

		add(buffer, bufferSize, "$");
		add(buffer, bufferSize, num);
		add(buffer, bufferSize, CRLF);
		add(buffer, bufferSize, arg);
		add(buffer, bufferSize, CRLF);
	}

	ArrayDestroy(args);

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
 * @param string respData
 *
 * @return integer
 */
parse_resp_data(data[], respData[])
{
	new respType, error[512];

	switch (data[0])
	{
		case '+': {
			respType = parse_resp_simple_string(data[1], respData);
		}
		case '$': {
			respType = parse_resp_bulk_string(data[1], respData, error, 511);
		}
		case '*': {
			respType = parse_resp_array(data[1], respData, error, 511);
		}
		case ':': {
			respType = parse_resp_integer(data[1], respData, error, 511);
		}
		case '-': {
			respType = parse_resp_error(data[1], respData, error, 511);
		}
		default: {
			server_print(error);
		}
	}

	return respType;
}

stock parse_resp_integer(data[], respData[], error[], errorSize)
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
			respData[0] = 0;
			copy(error, errorSize, "Non-integer response");

			return 0;
		}

		add(respData, 510, data[i], 1);
		i++;
	}

	return 1;
}

stock parse_resp_simple_string(data[], respData[])
{
	new i;

	while (data[i])
	{
		if (data[i] == 13 && data[i + 1] == 10)
		{
			break;
		}

		add(respData, 510, data[i], 1);
		i++;
	}

	return 2;
}

stock parse_resp_bulk_string(data[], respData[], error[], errorSize) {}

stock parse_resp_array(data[], respData[], error[], errorSize) {}

stock parse_resp_error(data[], respData[], error[], errorSize)
{
	parse_resp_simple_string(data, respData);
	copy(error, errorSize, respData);

	return 0;
}
