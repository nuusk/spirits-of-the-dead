//========================App================================
const electron = require('electron');
const url = require('url');
const path = require('path');

const {app, BrowserWindow, ipcMain} = electron;

let window;

app.on('ready', () => {
  window = new BrowserWindow({width: 300, height: 500});

  window.loadURL(url.format({
    pathname: path.join(__dirname, 'mainMenu.html'),
    protocol: 'file:',
    slashes: true
  }));

  window.on('closed', () => app.quit());
});


//========================ipcMain================================

//Menu
ipcMain.on('startGame', () => {
  window.loadURL(url.format({
    pathname: path.join(__dirname, 'lobby.html'),
    protocol: 'file:',
    slashes: true
  }));

  client.connect(port, address, () => {
    console.log('Connected to the server: ' + address + ':' + port);    
  });
});

ipcMain.on('exitApp', () => {
  app.exit();
});
//Menu


//Lobby
ipcMain.on('setName', (event, arg) => {
  client.write('name ' + arg);
})

ipcMain.on('setReady', (event, arg) => {
  if (arg)
    client.write('ready');
  else
    client.write('notready');
});

ipcMain.on('chat', (event, arg) => {
  client.write('chat ' + arg);
});
//Lobby


//========================Sockets================================
const net = require('net');
const address = 'localhost';
const port = 1252;

let client = new net.Socket();

client.on('close', () => {
  console.log('Connection closed');
  client.destroy();
  app.quit();
});

client.on('error', (err) => {
  console.log('Error: ' + err);
  client.destroy();
  app.quit();
});

client.on('data', (data) => {
  processMessage(data);
});

function processMessage(data) {
  data = data.toString('utf8');
  console.log(data);
  data = JSON.parse(data);
  
  if (data.type == 'playersInfo' || data.type == 'chat')
    window.webContents.send(data.type, data);
}