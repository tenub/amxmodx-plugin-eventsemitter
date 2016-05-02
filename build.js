const pkg = require('./package.json');
const amxxpath = (pkg.amxmodx && pkg.amxmodx.path) ? pkg.amxmodx.path : '';

require('amxmodx-build')(amxxpath);
