const electron = require('electron');
const url = require('url');
const path = require('path');
const {app, BrowserWindow, ipcMain} = electron;
const net = require('net');
const serverAddress = require('../resources/sockets.json');

const WINDOW_HEIGHT = 700;
const WINDOW_WIDTH = 900;

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
  splitAndProcessMessage(data.toString('utf8'));
});

function splitAndProcessMessage(input) 
{
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
  data = data.toString('utf8');
  data = JSON.parse(data);

  if (data.type == 'playersInfo' || data.type == 'chat')
    _window.webContents.send(data.type, data);
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

  //when a character is selected, we can connect the client to the server
  client.connect(serverAddress.server.port, serverAddress.server.address, () => {
    console.log('Connected to the server ' + serverAddress.server.address + ':' + serverAddress.server.port + '...');
  });

  _window.on('closed', () => {
    app.quit();
    _window = null;
  });
});

//waiting for player to select the character
ipcMain.on('character:select', (e, player) => {
  //after he selects the character, close the selection menu and load the actual game
  _window.loadURL(url.format({
    pathname: path.join(__dirname, 'game.html'),
    protocol: 'file:',
    slashes: true
  }));

  _window.on('closed', () => {
    app.quit();
    _window = null;
  });
});

ipcMain.on('terminal:command', (e, command) => {
  console.log(JSON.stringify(command));
  client.write(JSON.stringify(command));
});

ipcMain.on('setReadyState', (e, readyState) => {
  if (readyState)
    client.write('ready');
  else
    client.write('notready');
});

ipcMain.on('setName', (e, name) => {
  client.write('name ' + name);
});

ipcMain.on('chat', (e, msg) => {
  client.write('chat ' + msg);
});