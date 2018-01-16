const electron = require('electron');
const url = require('url');
const path = require('path');
const {app, BrowserWindow, ipcMain} = electron;
const net = require('net');
const serverAddress = require('../resources/sockets.json');

const WINDOW_HEIGHT = 700;
const WINDOW_WIDTH = 900;
const END_OF_MSG = '10101010101010101010101010101010';


//~~~~~~~~~~~~~~~~~~~~~ Socket Connection ~~~~~~~~~~~~~~~~~~~~~~~
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
  splitAndProcessMessage(data.toString('utf8'));
});

function splitAndProcessMessage(input) {
  let bracketCount = 0;
  let start = 0;

  for (let i = 0; i < input.length; i++)
  {
    if (input.charAt(i) == '{')
      bracketCount++;

    else if (input.charAt(i) == '}')
    {
      bracketCount--;

      if (bracketCount == 0)
      {
        let singleObject = input.substr(start, i - start + 1);
        console.log('Server: ' + singleObject);
        processMessage(singleObject);
        start = i + 1;
      }
    }
  }
}

function processMessage(data) {
  data = JSON.parse(data.toString('utf8'));
  _window.webContents.send(data.type, data);
}


//~~~~~~~~~~~~~~~~~~~~~ App Rendering ~~~~~~~~~~~~~~~~~~~~~~~
let _window;

app.on('ready', () => {
  _window = new BrowserWindow({width: WINDOW_WIDTH, height: WINDOW_HEIGHT});

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



//~~~~~~~~~~~~~~~~~~~~~ ipcMain ~~~~~~~~~~~~~~~~~~~~~~~
ipcMain.on('character:select', (e, player) => {
  _window.loadURL(`file://${__dirname}/game.html?avatar=${player}`);

  client.connect(serverAddress.server.port, serverAddress.server.address, () => {
    console.log('Connected to the server ' + serverAddress.server.address + ':' + serverAddress.server.port + '...');
  });

  _window.on('closed', () => {
    app.quit();
    _window = null;
  });
});

ipcMain.on('chat', (e, message) => {
  sendMessage('chat ' + message);
});

ipcMain.on('terminal:command', (e, command) => {
  sendMessage(command);
});

ipcMain.on('setReadyState', (e, readyState) => {
  if (readyState)
    sendMessage('ready');
  else
    sendMessage('notready');
});

ipcMain.on('setName', (e, name) => {
  sendMessage('name ' + name);
});

ipcMain.on('getPlayersLobbyInfo', () => {
  sendMessage('playersLobbyInfo');
});

ipcMain.on('getGameStarted', (e) => {
  sendMessage('getGameStarted');
});



function sendMessage(message) {
  message += END_OF_MSG;
  client.write(message);
}
