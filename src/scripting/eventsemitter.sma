#include <amxmodx>
#include <amxmisc>

#include "include/redis.inc"

new g_ServerIp[32], g_ServerKey[64], g_ServerMap[32]

public plugin_init()
{
	register_plugin("Server events emitter", "1.0", "pvab")
	register_clcmd("say", "EventSay")
	get_game_state()
}

public get_game_state()
{
	new payload[512]

	get_mapname(g_ServerMap, 31)
	get_user_ip(0, g_ServerIp, 31)

	formatex(payload, 511, "{^"map^":^"%s^"}", g_ServerMap)
	formatex(g_ServerKey, 63, "servers:%s", g_ServerIp)

	redis_publish(g_ServerKey, payload)
}

public client_authorized(id)
{
	new payload[512], authid[32], name[33], ip[32]
	new bool:isAdmin = is_user_admin(id)

	get_user_name(id, name, 32)
	get_user_authid(id, authid, 31)
	get_user_ip(id, ip, 31)

	formatex(payload, 511, "{^"connected^":%i,^"authid^":^"%s^",^"name^":^"%s^",^"ip^":^"%s^",^"admin^":%i}", true, authid, name, ip, isAdmin)

	redis_publish(g_ServerKey, payload)
}

public client_disconnect(id)
{
	new payload[512], authid[32]

	get_user_authid(id, authid, 31)

	formatex(payload, 511, "{^"connected^":%i,^"authid^":^"%s^"}", false, authid)

	redis_publish(g_ServerKey, payload)
}

public EventPlayerRank(table[9], map[32], authid[32], name[33], Float:time, date[20], weapon[7], cp, gc)
{
	new payload[512]

	formatex(payload, 511, "{^"table^":^"%s^",^"map^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"time^":%f,^"date^":^"%s^",^"weapon^":^"%s^",^"cp^":%i,^"gc^":%i}", table, map, authid, name, time, date, weapon, cp, gc)

	redis_publish("records", payload)
}

public EventSay(id)
{
	new text[193]

	read_args(text, 192);

	if (equal(text[5], "/"))
	{
		return
	}

	new payload[512], authid[32], name[33]
	new bool:isAdmin = is_user_admin(id)

	get_user_name(id, name, 32)
	get_user_authid(id, authid, 31)

	formatex(payload, 511, "{^"authid^":^"%s^",^"name^":^"%s^",^"admin^":%i,^"text^":%s}", authid, name, isAdmin, text)

	redis_publish("chat", payload)
}
