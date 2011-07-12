'use strict';

/*!
 *
 * Copyright(c) 2011 Vladimir Dronnikov <dronnikov@gmail.com>
 * MIT Licensed
 *
*/

/**
 * Listen to specified URL and respond with status 200
 * to signify this server is alive
 */

module.exports = function setup(url) {

	if (!url) url = '/haproxy?monitor';

	return function handler(req, res, next) {
		if (req.url === url) {
			res.writeHead(200);
			res.end();
		} else {
			next();
		}
	};
};
