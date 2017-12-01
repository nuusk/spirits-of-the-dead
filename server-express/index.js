const express = require('express');
const socket = require('socket.io');

const PORT = 1252;

//app setup
const app = express();
const server = app.listen(PORT, () => {
  console.log('Server started listening on port ' + PORT + '...');
});

//static files
app.use(express.static('public'));

//socket setup
const io = socket(server);

io.on('connection', (socket) => {
  console.log('Created socket connection with ' + socket.id + '.');
});
