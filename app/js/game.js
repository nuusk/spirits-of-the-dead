const electron = require('electron');
const {ipcRenderer} = electron;

const wall = document.getElementById('wall');
const commandLine = document.getElementById('command-line');
const readyButton = document.getElementById('readyButton');
const name = document.getElementById('name');
const nameButton = document.getElementById('nameButton');
const onlineLabel = document.getElementById('playersOnline');
const readyLabel = document.getElementById('playersReady');
const answerNumber = document.getElementById('answerNumber');
const mostPopularAnswer = document.getElementById('mostPopularAnswer');

const lobbyDiv = document.getElementById('lobbyDiv');
const answersDiv = document.getElementById('answersDiv');

let ready = false;
let gameStarted = false;


window.onload = () => {
  ipcRenderer.send('getGameStarted');
  ipcRenderer.send('getPlayersLobbyInfo');
};


//~~~~~~~~~~~~~~~~~~~~~ Events ~~~~~~~~~~~~~~~~~~~~~~~
commandLine.addEventListener('keydown', (e)=>{
  if (e.keyCode == 13) 
  {
    ipcRenderer.send('terminal:command', commandLine.value);
    commandLine.value = '';
  }
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
    readyButton.style.display = 'none';
  } 
});

ipcRenderer.on('answersInfo', (e, data) => {
  answerNumber.innerHTML = data.answerNumber;
  mostPopularAnswer.innerHTML = data.mostPopularAnswer;
});

ipcRenderer.on('chat', (e, data) => {
  wall.value += data.message + '\n';
});

ipcRenderer.on('gameStart', () => {
  gameStarted = true;
  lobbyDiv.style.display = 'none';
  answersDiv.style.display = 'block';
});

ipcRenderer.on('gameEnd', () => {
  gameStarted = false;
  answersDiv.style.display = 'none';  
  lobbyDiv.style.display = 'block';
  readyButton.style.display = 'block';  

  ready = false;
  readyButton.value = 'Ready';
});

ipcRenderer.on('stage', (e, data) => {
  wall.value += data.text + '\n';
  
  for (let i = 0; i < data.answers.length; i++)
    wall.value += `${i+1}. ${data.answers[i]}`;
  
  wall.value += '\n';
});