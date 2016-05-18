#include <amxmodx>
#include <amxmisc>
#include <redis>

new g_ServerIp[32], g_ServerName[33];

public plugin_init()
{
	register_plugin("Server events emitter", "1.0", "pvab");

	register_clcmd("say", "EventSay");
	register_clcmd("say_team", "EventSayTeam");

	redis_subscribe("webchat");

	get_game_state();
}

public plugin_end()
{
	redis_release();
}

/**
 * This function is called upon join when a client has successfully authenticated with the Steam server
 * Prepare and send a connect message to publish over the redis connection
 *
 * @param integer id Client id
 */
public client_authorized(id)
{
	static authid[32];

	get_user_authid(id, authid, 31);

	if (equal(authid, "BOT"))
	{
		return;
	}

	static serverKey[40], name[33], ip[32];
	new isAdmin = is_user_admin(id);

	get_user_name(id, name, 32);
	get_user_ip(id, ip, 31);
	formatex(serverKey, 39, "players", g_ServerIp);

	if (redis_send_command("sadd", serverKey, authid) && redis_send_command("hmset", authid, "authid", authid, "name", name, "ip", ip))
	{
		static payload[512];

		formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"connected^":%d,^"authid^":^"%s^",^"name^":^"%s^",^"ip^":^"%s^",^"admin^":%d}", g_ServerIp, g_ServerName, true, authid, name, ip, isAdmin);

		redis_send_command("publish", serverKey, payload);
	}
}

/**
 * This function is called when a client disconnects from the server
 * Prepare and send a disconnect message to publish over the redis connection
 *
 * @param integer id Client id
 */
public client_disconnected(id)
{
	static authid[32];

	get_user_authid(id, authid, 31);

	if (equal(authid, "BOT"))
	{
		return;
	}

	static serverKey[40];

	formatex(serverKey, 39, "players", g_ServerIp);

	if (redis_send_command("srem", serverKey, authid))
	{
		static payload[512];

		formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"connected^":%d,^"authid^":^"%s^"}", g_ServerIp, g_ServerName, false, authid);

		redis_send_command("publish", serverKey, payload);
	}
}

/**
 * This function is called after the "Top15Check" function successfully inserts/updates a record in the SQL database when a client presses the stop timer
 * Prepare and send a player record to publish over the redis connection
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
	static payload[512];

	formatex(payload, 511, "{^"table^":^"%s^",^"map^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"time^":%f,^"date^":^"%s^",^"weapon^":^"%s^",^"cp^":%d,^"gc^":%d}", table, map, authid, name, time, date, weapon, cp, gc);

	redis_send_command("publish", "records", payload);
}

/**
 * This function is called when a client issues a say command (all chat event)
 * Prepare and send a chat message to publish over the redis connection
 *
 * @param integer id Client id
 */
public EventSay(id)
{
	static text[193];

	read_args(text, 192);
	remove_quotes(text);

	if (!strlen(text))
	{
		return;
	}

	static payload[512], authid[32], name[33];
	new isAdmin = is_user_admin(id);

	get_user_name(id, name, 32);
	get_user_authid(id, authid, 31);

	sanitize(text, 192);
	sanitize(g_ServerName, 32);
	sanitize(name, 32);

	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"admin^":%d,^"text^":^"%s^",^"datetime^":%d}", g_ServerIp, g_ServerName, authid, name, isAdmin, text, get_systime());

	if (redis_send_command("lpush", "chat", payload))
	{
		redis_send_command("ltrim", "chat", "0", "99");
		redis_send_command("publish", "chat", payload);
	}
}

/**
 * This function is called when a client issues a say_team command (team chat event)
 * Prepare and send a team chat message to publish over the redis connection
 *
 * @param integer id Client id
 */
public EventSayTeam(id)
{
	static text[193];

	read_args(text, 192);
	remove_quotes(text);

	if (!strlen(text))
	{
		return;
	}

	static payload[512], authid[32], name[33], team[32];
	new isAdmin = is_user_admin(id);

	get_user_name(id, name, 32);
	get_user_authid(id, authid, 31);
	get_user_team(id, team, 31);

	sanitize(text, 192);
	sanitize(g_ServerName, 32);
	sanitize(name, 32);

	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"team^":^"%s^",^"admin^":%d,^"text^":^"%s^",^"datetime^":%d}", g_ServerIp, g_ServerName, authid, name, team, isAdmin, text, get_systime());

	if (redis_send_command("lpush", "chat", payload))
	{
		redis_send_command("ltrim", "chat", "0", "99");
		redis_send_command("publish", "chat", payload);
	}
}

/**
 * This function is called upon server init (when map has changed and plugins load)
 * Prepare and send a server init message to publish over the redis connection
 */
public get_game_state()
{
	static serverKey[40], serverMap[32], serverMaxPlayers[3];

	get_user_ip(0, g_ServerIp, 31);
	get_user_name(0, g_ServerName, 32);
	get_mapname(serverMap, 31);
	num_to_str(get_maxplayers(), serverMaxPlayers, 2);
	formatex(serverKey, 39, "server:%s", g_ServerIp);

	if (redis_send_command("hmset", serverKey, "ip", g_ServerIp, "name", g_ServerName, "map", serverMap, "maxplayers", serverMaxPlayers))
	{
		static payload[512];

		formatex(payload, 511, "{^"ip^":^"%s^",^"name^":^"%s^",^"map^":^"%s^",^"maxplayers^":%d}", g_ServerIp, g_ServerName, serverMap, serverMaxPlayers);

		redis_send_command("publish", "servers", payload);
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
