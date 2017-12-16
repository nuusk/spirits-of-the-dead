const electron = require('electron');
const url = require('url');
const path = require('path');
const {app, BrowserWindow, ipcMain} = electron;
const net = require('net');

const ADDRESS = '192.168.8.107';
const PORT = 1252;
const WINDOW_HEIGHT = 600;
const WINDOW_WIDTH = 800;



//~~~~~~~~~~ Socket Connection ~~~~~~~~~~~

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
  // processMessage(data);
  console.log('Server: ' + data);
});

function processMessage(data) {
  data = data.toString('utf8');
  console.log(data);
  data = JSON.parse(data);

  if (data.type == 'playersInfo' || data.type == 'chat')
    window.webContents.send(data.type, data);
}


//~~~~~~~~~~~ App rendering ~~~~~~~~~~~~~~~

//pointer to the game window
let _window;

app.on('ready', () => {
  //create the window with a set height and width
  _window = new BrowserWindow({width: WINDOW_WIDTH, height: WINDOW_HEIGHT});

  //load game.html to the current window
  _window.loadURL(url.format({
    pathname: path.join(__dirname, 'character-select.html'),
    protocol: 'file:',
    slashes: true
  }));

  _window.on('closed', () => {
    app.quit();
    _window = null;
  });
});

//waiting for player to select the character
ipcMain.on('character:select', (e, player) => {
  //after he selects the character, close the selection menu and run the actual game
  _window = new BrowserWindow({width: WINDOW_WIDTH, height: WINDOW_HEIGHT});
  _window.loadURL(url.format({
    pathname: path.join(__dirname, 'game.html'),
    protocol: 'file:',
    slashes: true
  }));

  _window.on('closed', () => {
    app.quit();
    _window = null;
  });

  //when a character is selected, we can connect the client to the server
  client.connect(PORT, ADDRESS, () => {
  	console.log('Connected to the server ' + ADDRESS + ':' + PORT + '...');
  });
});

ipcMain.on('terminal:command', (e, command) => {
  console.log(e);
  console.log(command);
  client.write(JSON.stringify(command));
});


//========================ipcMain================================
/*
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
*/
