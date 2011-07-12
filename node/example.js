'use strict';

var n = 0;

function Worker(port) {
  var id = ++n;
  this.http = require('http').createServer(function(req, res) {
    //
    // report health status to haproxy
    // N.B. you should use connect-compatible middleware layer
    // found in health.js beside this file
    //
    if (req.url === '/haproxy?monitor') {
      res.writeHead(200);
      res.end();
    //
    // hello world
    //
    } else {
      res.writeHead(200, {'content-type': 'text/html'});
      res.end('<h1>Hello from a farm worker #' + id + '</h1>');
    }
  });
  this.http.listen(port, function() {
    console.log('Worker started on port', port);
  });
}

new Worker(3001);
new Worker(3002);
new Worker(3003);
new Worker(3004);
