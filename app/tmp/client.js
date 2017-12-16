"use strict";
const net = require('net');
const stdin = process.openStdin();
const keypress = require('keypress');
const address = '192.168.43.49';
// const address = 'localhost';
const port = 1252;

//emitt keypress events by stdin
keypress(process.stdin);

//create socket
let client = new net.Socket();

//connect to the socket with given address and port
client.connect(port, address, () => {
	console.log('Connected to the server ' + address + ':' + port + '...');
});

//listen for the keypress event
process.stdin.on('keypress', (ch, key) => {
  // console.log('got "keypress"', key);
  //console.log(key);
  client.write(JSON.stringify(key));
  if (key && key.ctrl && key.name == 'c') {
    quit();
  }
});

process.stdin.setRawMode(true);
process.stdin.resume();

client.on('data', (data) => {
	console.log('Server: ' + data);
});

client.on('close', () => {
	console.log('Connection closed');
});

client.on('error', (err) => {
  console.log('Error: ' + err);
  quit();
});

const quit = () => {
  client.destroy();
  process.stdin.pause();
}
