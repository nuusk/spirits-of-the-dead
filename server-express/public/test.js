var socket = new WebSocket("ws://localhost:1252");

console.log(socket);

socket.addEventListener('message', function (event) {
    console.log('Message from server ', event.data);
});