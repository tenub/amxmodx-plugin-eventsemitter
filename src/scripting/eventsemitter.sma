#include <amxmodx>
#include <amxmisc>

#include "include/redis.inc"

static g_ServerIp[32], g_ServerName[33], g_ServerMap[32]

public plugin_init()
{
	register_plugin("Server events emitter", "1.0", "pvab")
	register_clcmd("say", "EventSay")
	register_clcmd("say_team", "EventSayTeam")
	get_game_state()
}

public get_game_state()
{
	static payload[512]

	get_mapname(g_ServerMap, 31)
	get_user_ip(0, g_ServerIp, 31)
	//get_players(players, playersCount, "c")

	formatex(payload, 511, "{^"server^":^"%s^",^"map^":^"%s^"}", g_ServerIp, g_ServerMap)

	redis_publish("servers", payload)
}

public client_authorized(id)
{
	static payload[512], authid[32], name[33], ip[32]
	new isAdmin = is_user_admin(id)

	get_user_name(id, name, 32)
	get_user_authid(id, authid, 31)
	get_user_ip(id, ip, 31)

	formatex(payload, 511, "{^"server^":^"%s^",^"player^":{^"connected^":%i,^"authid^":^"%s^",^"name^":^"%s^",^"ip^":^"%s^",^"admin^":%i}}", g_ServerIp, true, authid, name, ip, isAdmin)

	redis_publish("servers", payload)
}

public client_disconnected(id)
{
	static payload[512], authid[32]

	get_user_authid(id, authid, 31)

	formatex(payload, 511, "{^"server^":^"%s^",^"player^":{^"connected^":%i,^"authid^":^"%s^"}}", g_ServerIp, false, authid)

	redis_publish("servers", payload)
}

public EventPlayerRank(table[9], map[32], authid[32], name[33], Float:time, date[20], weapon[7], cp, gc)
{
	static payload[512]

	formatex(payload, 511, "{^"table^":^"%s^",^"map^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"time^":%f,^"date^":^"%s^",^"weapon^":^"%s^",^"cp^":%i,^"gc^":%i}", table, map, authid, name, time, date, weapon, cp, gc)

	redis_publish("records", payload)
}

public EventSay(id)
{
	static text[193], payload[512], authid[32], name[33]
	new isAdmin = is_user_admin(id)

	read_args(text, 192)
	remove_quotes(text)

	if (!strlen(text))
	{
		return
	}

	get_user_name(0, g_ServerName, 32) // get server hostname
	get_user_name(id, name, 32) // get player name
	get_user_authid(id, authid, 31) // get player authid

	sanitize(text, 192)
	sanitize(g_ServerName, 32)
	sanitize(name, 32)

	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"admin^":%i,^"text^":^"%s^",^"datetime^":%i}", g_ServerIp, g_ServerName, authid, name, isAdmin, text, get_systime())

	redis_publish("chat", payload)
}

public EventSayTeam(id)
{
	static text[193], payload[512], authid[32], name[33], team[32]
	new isAdmin = is_user_admin(id)

	read_args(text, 192)
	remove_quotes(text)

	if (!strlen(text))
	{
		return
	}

	get_user_name(0, g_ServerName, 32) // get server hostname
	get_user_name(id, name, 32) // get player name
	get_user_authid(id, authid, 31) // get player authid
	get_user_team(id, team, 31) // get player team

	sanitize(text, 192)
	sanitize(g_ServerName, 32)
	sanitize(name, 32)

	formatex(payload, 511, "{^"serverip^":^"%s^",^"servername^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"team^":^"%s^",^"admin^":%i,^"text^":^"%s^",^"datetime^":%i}", g_ServerIp, g_ServerName, authid, name, team, isAdmin, text, get_systime())

	redis_publish("chat", payload)
}

sanitize(text[], textSize)
{
	trim(text)
	replace_all(text, textSize, "\", "\\")
	replace_all(text, textSize, "^"", "\^"")
}
