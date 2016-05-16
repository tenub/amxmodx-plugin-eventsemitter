#if defined _resp_included
	#endinput
#endif
#define _resp_included

static const char StringReply = '+';
static const char ErrorReply = '-';
static const char IntegerReply = ':';
static const char BulkReply = '$';
static const char ArrayReply = '*';

enum RespResult
{
	Completed,
	Incompleted,
	Error
};

enum RespState
{
	Start = 0,

	String,
	StringLF,

	ErrorString,
	ErrorLF,

	Integer,
	IntegerLF,

	BulkLen,
	BulkLenLF,
	Bulk,
	BulkCR,
	BulkLF,

	ArrayLen,
	ArrayLenLF
};

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
	new i, c, bulkLen, error[512], RespState:respState;

	while ((c = data[i]))
	{
		switch (respState)
		{
			case respState[Start]:
			{
				message[0] = 0;

				switch (c)
				{
					case StringReply:
					{
						respState = String;
					}
					case ErrorReply:
					{
						respState = ErrorString;
					}
					case IntegerReply: {
						respState = Integer;
					}
					case BulkReply: {
						respState = BulkLen;
						bulkLen = 0;
					}
					case ArrayReply: {
						respState = ArrayLen;
					}
					default: {
						respState = Start;
						return 0;
					}
				}
			}
			default: {
				respState = Start;
				return 0;
			}
		}

		i++;
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
