//make connection
const serverAddr = 'http://localhost';
const port = 1252;
const socket = io.connect(serverAddr + ':' + port);
