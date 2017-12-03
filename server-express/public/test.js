//make connection
const serverAddr = 'http://localhost';
//const serverAddr = '192.168.8.107';
const port = 1253;
const socket = io.connect(serverAddr + ':' + port);

const message = document.getElementById('message');
const sendButton = document.getElementById('send');
const output = document.getElementById('output');

sendButton.addEventListener('click', () => {
  socket.emit('testEventName', {
    message: message.value
  });
});

//listen for events
socket.on('testEventName', (data) => {
  output.innerHTML += data.message + '</br>';
});

socket.on('connect', () => {
  console.log('connected.');
});

socket.on('message', (data) => {
  console.log('message sent.');
});

socket.on('disconnect', () => {
  console.log('disconnected.');
});
