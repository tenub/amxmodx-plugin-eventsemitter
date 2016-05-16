#include <amxmodx>
#include <amxmisc>
#include <redis>

new g_ServerIp[32], g_ServerName[33];

public plugin_init()
{
	register_plugin("Server events emitter", "1.0", "pvab");

	// register event handlers
	register_clcmd("say", "EventSay");
	register_clcmd("say_team", "EventSayTeam");

	// subscribe to web chat channel
	redis_subscribe("webchat");

	// get current game state of server and publish
	get_game_state();
}

public plugin_end()
{
	// unsubscribe from all channels and free subscriber handle
	redis_release();
}

/**
 * This function is called upon join when a client has successfully authenticated with the Steam server
 * Parse and prepare a connect message to publish over redis connection
 *
 * @param integer id Client id
 */
public client_authorized(id)
{
	static authid[32];

	get_user_authid(id, authid, 31);

	// return early if client is a bot
	if (equal(authid, "BOT"))
	{
		return;
	}

	static payload[512], name[33], ip[32], respBuffer[512];
	new isAdmin = is_user_admin(id);

	// get user info to send with connect message
	get_user_name(id, name, 32);
	get_user_ip(id, ip, 31);

	// format values to a single JSON string (payload)
	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"connected^":%i,^"authid^":^"%s^",^"name^":^"%s^",^"ip^":^"%s^",^"admin^":%i}", g_ServerIp, g_ServerName, true, authid, name, ip, isAdmin);

	// publish JSON to redis servers channel
	redis_send_command(respBuffer, "publish", "servers", payload);
}

/**
 * This function is called when a client disconnects from the server
 * Parse and prepare a disconnect message to publish over redis connection
 *
 * @param integer id Client id
 */
public client_disconnected(id)
{
	static authid[32];

	// get user info to send with disconnect message
	get_user_authid(id, authid, 31);

	// return early if client is a bot
	if (equal(authid, "BOT"))
	{
		return;
	}

	static payload[512], respBuffer[512];

	// format values to a single JSON string (payload)
	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"connected^":%i,^"authid^":^"%s^"}", g_ServerIp, g_ServerName, false, authid);

	// publish JSON to redis servers channel
	redis_send_command(respBuffer, "publish", "servers", payload);
}

/**
 * This function is called after the "Top15Check" function is called when a client presses the stop timer
 * Parse and prepare a player record to publish over redis connection
 *
 * @param string table
 * @param string map
 * @param string authid
 * @param string name
 * @param float time
 * @param string date
 * @param string weapon
 * @param integer cp
 * @param integer gc
 */
public EventPlayerRank(table[9], map[32], authid[32], name[33], Float:time, date[20], weapon[7], cp, gc)
{
	static payload[512], respBuffer[512];

	// format values to a single JSON string (payload)
	formatex(payload, 511, "{^"table^":^"%s^",^"map^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"time^":%f,^"date^":^"%s^",^"weapon^":^"%s^",^"cp^":%i,^"gc^":%i}", table, map, authid, name, time, date, weapon, cp, gc);

	// publish JSON to redis records channel
	redis_send_command(respBuffer, "publish", "records", payload);
}

/**
 * This function is called when a client issues a say command (all chat event)
 * Parse and prepare a chat message to publish over redis connection
 *
 * @param integer id Client id
 */
public EventSay(id)
{
	static text[193];

	// read client say command into text buffer
	read_args(text, 192);
	// remove quotes from text (some text is not received with quotes so it must be normalized)
	remove_quotes(text);

	// return early if text is "empty"
	if (!strlen(text))
	{
		return;
	}

	static payload[512], authid[32], name[33], respBuffer[512];
	new isAdmin = is_user_admin(id);

	// get user info to send with chat message
	get_user_name(id, name, 32);
	get_user_authid(id, authid, 31);

	// prepare values to send in JSON string
	sanitize(text, 192);
	sanitize(g_ServerName, 32);
	sanitize(name, 32);

	// format values to a single JSON string (payload)
	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"admin^":%i,^"text^":^"%s^",^"datetime^":%i}", g_ServerIp, g_ServerName, authid, name, isAdmin, text, get_systime());

	if (redis_send_command(respBuffer, "lpush", "chat", payload))
	{
		redis_send_command(respBuffer, "ltrim", "chat", "0", "99");
		redis_send_command(respBuffer, "publish", "chat", payload);
	}
}

/**
 * This function is called when a client issues a say_team command (team chat event)
 * Parse and prepare a team chat message to publish over redis connection
 *
 * @param integer id Client id
 */
public EventSayTeam(id)
{
	static text[193];

	// read client say_team command into text buffer
	read_args(text, 192);
	// remove quotes from say text (some text is not received with quotes so it must be normalized)
	remove_quotes(text);

	// return early if text is "empty"
	if (!strlen(text))
	{
		return;
	}

	static payload[512], authid[32], name[33], team[32], respBuffer[512];
	new isAdmin = is_user_admin(id);

	// get user info to send with chat message
	get_user_name(id, name, 32);
	get_user_authid(id, authid, 31);
	get_user_team(id, team, 31);

	// prepare values to send in JSON string
	sanitize(text, 192);
	sanitize(g_ServerName, 32);
	sanitize(name, 32);

	// format values to a single JSON string (payload)
	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"team^":^"%s^",^"admin^":%i,^"text^":^"%s^",^"datetime^":%i}", g_ServerIp, g_ServerName, authid, name, team, isAdmin, text, get_systime());

	if (redis_send_command(respBuffer, "lpush", "chat", payload))
	{
		redis_send_command(respBuffer, "ltrim", "chat", "0", "99");
		redis_send_command(respBuffer, "publish", "chat", payload);
	}
}

/**
 * This function is called upon server init (when map has changed and plugins load)
 * Parse and prepare a server init message to publish over redis connection
 */
public get_game_state()
{
	static serverKey[40], serverMap[32], serverMaxPlayers[3], respBuffer[512];

	// get server info to send with init message
	get_user_ip(0, g_ServerIp, 31);
	get_user_name(0, g_ServerName, 32);
	get_mapname(serverMap, 31);
	num_to_str(get_maxplayers(), serverMaxPlayers, 2);
	formatex(serverKey, 39, "server:%s", g_ServerIp);

	// publish JSON to redis channel if data was set to database successfully
	if (redis_send_command(respBuffer, "hmset", serverKey, "ip", g_ServerIp, "name", g_ServerName, "map", serverMap, "maxplayers", serverMaxPlayers))
	{
		static payload[512];

		// prepare server info as a JSON payload to publish
		formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"map^":^"%s^",^"maxplayers^":%i}", g_ServerIp, g_ServerName, serverMap, serverMaxPlayers);

		redis_send_command(respBuffer, "publish", "servers", payload);
	}
}

/**
 * Helper function to format a string
 * Remove excess whitespace and escape string to validate as JSON
 *
 * @param string text
 * @param integer textSize
 */
sanitize(text[], textSize)
{
	trim(text);
	replace_all(text, textSize, "\", "\\");
	replace_all(text, textSize, "^"", "\^"");
}
