//make connection
const serverAddr = 'http://localhost';
const port = 1252;
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
