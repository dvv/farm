
# HTTP/HTTPS server farm made easy

Provides means to scale HTTP/HTTPS servers in unobtrusive way.
Uses haproxy as reverse proxy and load balancer for HTTP, and stud as HTTPS terminator.
Supports socket.io. Serves permissive Flash Sockets Policy.

## How to Install

    apt-get install runit libev-dev libssl-dev uuid-dev
    npm install farm

Go to ~/node_modules/farm and run

Now you have haproxy listening to :80 for HTTP and :65443 for HTTPS terminated by stud listening to :443.

## How to use

Edit /etc/service/haproxy/haproxy.conf and /etc/service/stud/run. Changes you might want to make are marked with 'CONFIG:'.

_N.B. Your server should respond to health requests emitted by haproxy to ensure the farm node is alive and functional._
You can simply prepend your middleware with bundled connect-compatible handler found at node/health.js:

    app.use(require('lib/health')());

## Example

    cd node
    node example.js
    open http://localhost
    open https://localhost

## License 

(The MIT License)

Copyright (c) 2011 Vladimir Dronnikov &lt;dronnikov@gmail.com&gt;

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
