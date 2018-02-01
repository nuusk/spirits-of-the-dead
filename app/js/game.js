const electron = require('electron');
const {ipcRenderer} = electron;

const wall = document.getElementById('wall');
const commandLine = document.getElementById('command-line');
const readyButton = document.getElementById('set-ready');
const name = document.getElementById('name');
const nameButton = document.getElementById('set-name');
const username = document.getElementById('username');
const onlineLabel = document.getElementById('players-online');
const readyLabel = document.getElementById('players-ready');
const answerNumber = document.getElementById('answer-number');
const mostPopularAnswer = document.getElementById('most-popular-answer');

const lobbyDiv = document.getElementsByClassName('lobby');
const answersDiv = document.getElementsByClassName('answers');

let ready = false;
let gameStarted = false;
let chatting = true;
let nameSet = false;

window.onload = () => {
  ipcRenderer.send('getGameStarted');
  ipcRenderer.send('getPlayersLobbyInfo');
  document.getElementById(`${global.location.search.slice(8)}-avatar`).style.display = 'block';
  answersDiv[0].style.display = 'none';
};

//~~~~~~~~~~~~~~~~~~~~~ Events ~~~~~~~~~~~~~~~~~~~~~~~
commandLine.addEventListener('keydown', (e)=>{
  if (e.keyCode == 13 && commandLine.value.trim() != "") {
    if (chatting)
    {
      wall.value += 'You: ' + commandLine.value + '\n';
      wall.scrollTop = wall.scrollHeight;
      ipcRenderer.send('chat', commandLine.value);
    }
    else
      ipcRenderer.send('terminal:command', commandLine.value);

    commandLine.value = '';
  }
});

readyButton.addEventListener('click', () => {
  if (!ready)
    readyButton.innerHTML = 'Not ready';
  else
    readyButton.innerHTML = 'Ready';

  ready = !ready;
  ipcRenderer.send('setReadyState', ready);
});

nameButton.addEventListener('click', () => {
  if (!nameSet && username.value.trim() != '') {
    nameSet = true;
    ipcRenderer.send('setName', username.value);
    username.disabled = true;
    nameButton.disabled = true;
  }
});

username.addEventListener('focus', () => {
  this.value = '';
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
  answerNumber.innerHTML = `Answers:  ${data.answerNumber}`;
  mostPopularAnswer.innerHTML = `Most popular: ${data.mostPopularAnswer}`;
});

ipcRenderer.on('chat', (e, data) => {
  wall.value += data.message + '\n';
  wall.scrollTop = wall.scrollHeight;
});

ipcRenderer.on('gameStart', () => {
  gameStarted = true;
  chatting = false;
  lobbyDiv[0].style.display = 'none';
  answersDiv[0].style.display = 'block';
  wall.value = "--------- GAME STARTED ---------\n";
  wall.value += "-----------> ENJOY! <-----------\n"
});

ipcRenderer.on('gameEnd', () => {
  gameStarted = false;
  chatting = true;
  answersDiv[0].style.display = 'none';
  lobbyDiv[0].style.display = 'block';
  readyButton.style.display = 'block';

  ready = false;
  readyButton.innerHTML = 'Ready';

  window.scrollTop = window.scrollHeight;

  wall.value += '--------- THE END ---------';
  wall.value += '\n--------- THE END ---------';
  wall.value += '\n--------- THE END ---------\n\n\n\n\n';
  wall.scrollTop = wall.scrollHeight;
  wall.focus();
});

ipcRenderer.on('stage', (e, data) => {
  commandLine.value = "";
  commandLine.disabled = true;

  setTimeout(() => {
    commandLine.disabled = false;
    commandLine.focus();
  }, 500);

  wall.value= "";
  wall.value += data.text + '\n';

  for (let i = 0; i < data.answers.length; i++)
    wall.value += `${i+1}. ${data.answers[i]}\n`;

  wall.scrollTop = wall.scrollHeight;
});
