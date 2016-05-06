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
	formatex(g_ServerKey, 63, "server:%s", g_ServerIp)

	redis_publish(g_ServerKey, payload)
}

public client_authorized(id)
{
	new payload[512], authid[32], name[33], ip[32]
	new admin = is_user_admin(id)

	get_user_name(id, name, 32)
	get_user_authid(id, authid, 31)
	get_user_ip(id, authid, 31)

	formatex(payload, 511, "{^"connected^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"ip^":^"%s^",^"admin^":%d}", true, authid, name, ip, admin)

	redis_publish(g_ServerKey, payload)
}

public client_disconnect(id)
{
	new payload[512], authid[32]

	get_user_authid(id, authid, 31)

	formatex(payload, 511, "{^"connected^":^"%s^",^"authid^":^"%s^"}", false, authid)

	redis_publish(g_ServerKey, payload)
}

public EventSay(id)
{
	new text[193]
	read_args(text, sizeof(text) - 1);

	if (equal(text[5], "/"))
	{
		return
	}

	new payload[512], authid[32], name[33]
	new admin = is_user_admin(id)

	get_user_name(id, name, 32)
	get_user_authid(id, authid, 31)

	formatex(payload, 511, "{ ^"authid^": ^"%s^", ^"name^": ^"%s^", ^"admin^": %d, ^"text^": %s }", authid, name, admin, text)

	redis_publish("chat", payload)
}

stock EscapeString(String:input[], String:output[])
{
	return
}
