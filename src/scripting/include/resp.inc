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
 * @param string data
 * @param integer dataSize
 * @param array buffer
 * @param integer bufferSize

 *
 * @return string
 */
bool:parse_resp_data(data[], const dataSize, buffer[][], bufferCount, bufferSize) {
	static success, error[64]

	switch (data[0])
	{
		// + simple string
		case 43: {
			success = parse_resp_simple_string(data[1], dataSize - 1, buffer, bufferCount, bufferSize, error)
		}
		// $ bulk string
		case 36: {
			success = parse_resp_bulk_string(data[1], dataSize - 1, buffer, bufferCount, bufferSize, error)
		}
		// * array
		case 42: {
			success = parse_resp_array(data[1], dataSize - 1, buffer, bufferCount, bufferSize, error)
		}
		// : integer
		case 58: {
			success = parse_resp_integer(data[1], dataSize - 1, buffer, bufferCount, bufferSize, error)
		}
		// - error
		case 45: {
			success = parse_resp_error(data[1], dataSize - 1, buffer, bufferCount, bufferSize, error)
			server_print(buffer)
		}
	}

	if (!success)
	{
		server_print(error)
	}

	return success
}

bool:parse_resp_simple_string(data[], const dataSize, buffer, const bufferCount, const bufferSize, error[])
{
	new i, arg[511]

	while (data[i++])
	{
		// buffer is terminated by CRLF, so "arg" is now fully parsed
		if (data[i] == 13 && data[i + 1] == 10)
		{
			break
		}

		// add each character to "arg" string if not at end of data
		add(arg, 510, data[i], 1)
	}

	copy(buffer[0], bufferSize, arg)

	return true
}

bool:parse_resp_bulk_string(data[], const dataSize, buffer, const bufferCount, const bufferSize, error[]) {}

bool:parse_resp_array(data[], const dataSize, buffer, const bufferCount, const bufferSize, error[]) {}

bool:parse_resp_integer(data[], const dataSize, buffer, const bufferCount, const bufferSize, error[])
{
	new i, arg[511]

	if (bufferCount != 1)
	{
		error = "Integer response expected"

		return false
	}

	while (data[i++])
	{
		// buffer is terminated by CRLF, so "arg" is now fully parsed
		if (data[i] == 13 && data[i + 1] == 10)
		{
			break
		}

		// throw error if data contains a non-integer value and reset "arg" value
		if (data[i] < 48 || data[i] > 57)
		{
			arg[0] = 0
			error = "Data is not an integer"

			return false
		}

		// add each character to "arg" string if not at end of data
		add(arg, 510, data[i], 1)
	}

	str_to_num(arg)
	copy(buffer[0], bufferSize, arg)

	// convert "arg" to integer since it contains only digits and return success/failure
	return true
}

bool:parse_resp_error(data[], const dataSize, buffer, const bufferCount, const bufferSize, error[])
{
	return parse_resp_simple_string(data, dataSize, buffer, bufferSize, error))
}
