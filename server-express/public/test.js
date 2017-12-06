// const address = "192.168.8.106";
const address = "localhost";
const port = "1252";
const button = document.getElementById('send');

let socket = new WebSocket("ws://"+address+":"+port);

socket.onmessage = function(event){
  console.log("ONMESSAGE: " + event.data);
}

socket.onopen = function(event){
  console.log("Socket: Connected!")
}

socket.onclose = function(event){
  console.log("Socket: Connection closed!");
}

button.onclick = () => {
  socket.send("dzień dobry");
};


// chrome.sockets.tcp.create({}, function(createInfo) {
//   chrome.sockets.tcp.connect(createInfo.socketId,
//     "192.168.8.106", 1252, function(socketInfo) {
//
//     });
// });
