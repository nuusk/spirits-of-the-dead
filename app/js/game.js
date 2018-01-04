const electron = require('electron');
const {ipcRenderer} = electron;

const commandLine = document.getElementById('command-line');
const readyButton = document.getElementById('readyButton');
const name = document.getElementById('name');
const nameButton = document.getElementById('nameButton');
const onlineLabel = document.getElementById('playersOnline');
const readyLabel = document.getElementById('playersReady');

const lobbyDiv = document.getElementById('lobbyDiv');

let ready = false;
let gameStarted = false;


window.onload = () => {
  ipcRenderer.send('getGameStarted');
  ipcRenderer.send('getPlayersLobbyInfo');
};


//~~~~~~~~~~~~~~~~~~~~~ Events ~~~~~~~~~~~~~~~~~~~~~~~
commandLine.addEventListener('keydown', (e)=>{
  if (e.keyCode == 13) 
    ipcRenderer.send('terminal:command', commandLine.value);
});

readyButton.addEventListener('click', () => {
  if (!ready)
    readyButton.value = 'Not ready';
  else 
    readyButton.value = 'Ready';

  ready = !ready;
  ipcRenderer.send('setReadyState', ready);
});

nameButton.addEventListener('click', () => {
  if (name.value.trim() != '')
    ipcRenderer.send('setName', name.value);
});


//~~~~~~~~~~~~~~~~~~~~~ ipcRenderer ~~~~~~~~~~~~~~~~~~~~~~~
ipcRenderer.on('gameStartedInfo', (e, data) => {
  gameStarted = data.gameStarted == "true" ? true : false;
});

ipcRenderer.on('playersLobbyInfo', (e, data) => {
  if (!gameStarted)
  {
    onlineLabel.innerHTML = `Online: ${data.online}`;
    readyLabel.innerHTML = `Ready: ${data.ready}`;
  }
  else
  {
    onlineLabel.innerHTML = `In game: ${data.inGame}`;
    readyLabel.innerHTML = `Waiting: ${data.waiting}`;
  } 
});

ipcRenderer.on('chat', (e, data) => {
  commandLine.value += data.message + '\n';
});

ipcRenderer.on('gameStart', () => {
  gameStarted = true;
  lobbyDiv.style.display = 'none';
});

ipcRenderer.on('gameEnd', () => {
  gameStarted = false;
  lobbyDiv.style.display = 'block';
});