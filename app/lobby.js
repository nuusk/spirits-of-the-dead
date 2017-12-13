const electron = require('electron');
const {ipcRenderer} = electron;

const nameButton = document.getElementById('nameButton');
const name = document.getElementById('nameBox');
const readyButton = document.getElementById('readyButton');
const onlinePlayers = document.getElementById('onlinePlayersLabel');
const readyPlayers = document.getElementById('readyPlayersLabel');
const sendButton = document.getElementById('sendButton');
const message = document.getElementById('messageBox');
const chatBox = document.getElementById('chatBox');


let ready = false;

nameButton.addEventListener('click', () => {
  ipcRenderer.send('setName', name.value);
});

readyButton.addEventListener('click', () => {
  ready = !ready;
  ipcRenderer.send('setReady', ready);
});

sendButton.addEventListener('click', () => {
  ipcRenderer.send('chat', message.value);
  chatBox.value += 'You: ' + message.value + '\n';
});


//========================ipcRenderer================================
ipcRenderer.on('playersInfo', (event, arg) => {
  onlinePlayers.innerHTML = "Online: " + arg.online;
  readyPlayers.innerHTML = "Ready: " + arg.ready;
});

ipcRenderer.on('chat', (event, arg) => {
  chatBox.value += arg.text + "\n";
});