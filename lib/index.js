
const addon = require('bindings')('node_pjsip');

module.exports = {
	Init: addon.Init,
	RegisterAccount: addon.registerAccount,
	Shutdown: addon.shutdown,
	// Add more mappings as you implement them
};
