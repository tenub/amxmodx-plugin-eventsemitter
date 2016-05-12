#pragma semicolon 1

#if defined _resp_included
	#endinput
#endif
#define _resp_included

new const CRLF[] = "^r^n";

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
	new numArgs = numargs();

	// do not format unless there are arguments
	if (numArgs <= 2)
	{
		return false;
	}

	static num[8], arg[256];

	buffer[0] = 0;
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
 * Read and parse data in RESP format
 *
 * @param string data
 * @param integer dataSize
 * @param array buffer
 * @param integer bufferSize

 *
 * @return string
 */
parse_resp_data(data[], respData[])
{
	new respType, error[512];

	switch (data[0])
	{
		// + simple string
		case 43: {
			respType = parse_resp_simple_string(data[1], respData);
		}
		// $ bulk string
		/*case 36: {
			respType = parse_resp_bulk_string(data[1], respData, error, 511);
		}
		// * array
		case 42: {
			respType = parse_resp_array(data[1], respData, error, 511);
		}*/
		// : integer
		case 58: {
			respType = parse_resp_integer(data[1], respData, error, 511);
		}
		// - error
		case 45: {
			respType = parse_resp_error(data[1], respData, error, 511);
		}
	}

	if (!respType)
	{
		server_print(error);
	}

	return respType;
}

parse_resp_integer(data[], respData[], error[], errorSize)
{
	new i;

	while (data[i])
	{
		if (data[i] < 48 || data[i] > 57)
		{
			respData[0] = 0;
			copy(error, errorSize, "Non-integer response");

			return 0;
		}

		if (data[i] == 13 && data[i + 1] == 10)
		{
			break;
		}

		add(respData, 510, data[i], 1);
		i++;
	}

	return 1;
}

parse_resp_simple_string(data[], respData[])
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

/*parse_resp_bulk_string(data[], respData[], error[], errorSize) {}

parse_resp_array(data[], respData[], error[], errorSize) {}*/

parse_resp_error(data[], respData[], error[], errorSize)
{
	parse_resp_simple_string(data, respData);
	copy(error, errorSize, respData);

	return 0;
}
