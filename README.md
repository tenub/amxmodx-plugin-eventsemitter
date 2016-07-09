# amxmodx-plugin-eventsemitter

> Emit server events to a redis subscriber

## Environment Variables

Create a `.env.inc` file located in the `src/scripting/include` directory binding the following symbols:

### `REDIS_HOST`

IP or domain to the server running redis

### `REDIS_PORT`

Port on which the redis server is running

### `REDIS_PASS`

Authentication password

An example file is included in the `src/scripting/include` directory.

## Build

Using Node.js, one may easily automate building the plugin using the command: `npm i && npm run build`.
