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

const lobbyDiv = document.getElementById('lobbyDiv');
const answersDiv = document.getElementById('answersDiv');

let ready = false;
let gameStarted = false;
let chatting = true;


window.onload = () => {
  ipcRenderer.send('getGameStarted');
  ipcRenderer.send('getPlayersLobbyInfo');
};


//~~~~~~~~~~~~~~~~~~~~~ Events ~~~~~~~~~~~~~~~~~~~~~~~
commandLine.addEventListener('keydown', (e)=>{
  if (e.keyCode == 13) {
    if (chatting)
      ipcRenderer.send('chat', commandLine.value);
    else
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
  if (username.value.trim() != '') {
    ipcRenderer.send('setName', username.value);
    // username.value = "";
    username.disabled = true;
    nameButton.disabled = true;
  }
});

username.addEventListener('focus', () => {
  console.log('yo');
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
});

ipcRenderer.on('gameStart', () => {
  gameStarted = true;
  chatting = false;
  lobbyDiv.style.display = 'none';
  answersDiv.style.display = 'block';
  wall.value = "--------- GAME STARTED ---------\n";
  wall.value += "-----------> ENJOY! <-----------\n"
});

ipcRenderer.on('gameEnd', () => {
  gameStarted = false;
  chatting = true;
  answersDiv.style.display = 'none';
  lobbyDiv.style.display = 'block';
  readyButton.style.display = 'block';

  ready = false;
  readyButton.value = 'Ready';

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

  wall.value += '\n' + data.text + '\n';

  for (let i = 0; i < data.answers.length; i++)
    wall.value += `${i+1}. ${data.answers[i]}\n`;

  wall.value += '\n';
  wall.scrollTop = wall.scrollHeight;
});
