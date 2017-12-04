var socket = new WebSocket("ws://localhost:1252");

socket.onmessage = function(event){
    console.log("ONMESSAGE: " + event.data);
}

socket.onopen = function(event){
    console.log("Socket: Connected!")
}

socket.onclose = function(event){
    console.log("Socket: Connection closed!");
}