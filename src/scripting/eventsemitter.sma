#include <amxmodx>
#include <amxmisc>

#include "include/redis.inc"

new g_ServerIp[32], g_ServerKey[64], g_ServerMap[32]

public plugin_init()
{
	// register plugin
	register_plugin("Server events emitter", "1.0", "pvab")

	// call initialization functions
	get_game_state()
	//set_task(5.0, "get_game_state")
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
	new payload[512], name[32], authid[32], ip[32]
	new admin = is_user_admin(id)

	get_user_name(id, name, 31)
	get_user_authid(id, authid, 31)
	get_user_ip(id, authid, 31)
	format(payload, 511, "{^"connected^":^"%s^",^"authid^":^"%s^",^"name^":^"%s^",^"ip^":^"%s^",^"admin^":^"%s^"}", true, authid, name, ip, admin)
	client_print(0, print_console, "Publish: %s", payload)
	redis_publish(g_ServerKey, payload)
}

public client_disconnect(id)
{
	new payload[512], authid[32]

	get_user_authid(id, authid, 31)
	format(payload, 511, "{^"connected^":^"%s^",^"authid^":^"%s^"}", false, authid)
	redis_publish(g_ServerKey, payload)
}
