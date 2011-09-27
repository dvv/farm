#!/usr/bin/env node
'use strict';

var ZMQ = require('zeromq');

var BROKER_REP = '127.0.0.1:5554';
var BROKER_PUB = '127.0.0.1:5555';

function log() {}
var log = console.log;

//
// broker
//
var brokerListener = ZMQ.createSocket('rep');
var brokerPublisher = ZMQ.createSocket('pub');
var n = 0;
brokerListener.bind('tcp://' + BROKER_REP, function(err) {
	if (err) throw err;
	brokerListener.on('message', function(data) {
		if (!(n % 10000)) console.error('BIN', n, Date.now());
		++n;
		log('BIN: ' + data.toString('utf8'));
		brokerPublisher.send(data);
		brokerListener.send('');
	});
	console.error('Broker is listening to ' + BROKER_REP + '...');
});
brokerPublisher.bind('tcp://' + BROKER_PUB, function(err) {
	if (err) throw err;
	brokerPublisher.on('message', function(data) {
		log('BPUB: ' + data.toString('utf8'));
	});
	console.error('Broker is listening to ' + BROKER_PUB + '...');
});

//
// worker
//
function Worker(id) {
	var w = ZMQ.createSocket('req');
	w.connect('tcp://' + BROKER_REP);
	this.send = function(data) {
		log('WOUT:', id, data.toString('utf8'));
		return w.send(data);
	};
	var w1 = ZMQ.createSocket('sub');
	w1.connect('tcp://' + BROKER_PUB);
	w1.subscribe('');
	var n = 0;
	w1.on('message', function(data) {
		if (!(n % 10000)) console.error('WIN', n, Date.now());
		++n;
		log('WIN:', id, data.toString('utf8'));
	});
}

var w1 = new Worker(1);
var w2 = new Worker(2);
var w3 = new Worker(3);
var w4 = new Worker(4);

setTimeout(function(){

//for (var i = 0; i < 100000; ++i) {
	w1.send('foo1');
	//w2.send('foo2');
	//w3.send('foo3');
//}

}, 500);

// REPL for introspection
var repl = require('repl').start('node> ').context;
process.stdin.on('close', process.exit);
repl.w1 = w1;
repl.w2 = w2;
repl.w3 = w3;
repl.w4 = w4;
