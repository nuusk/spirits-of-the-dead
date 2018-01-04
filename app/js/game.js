const electron = require('electron');
const {ipcRenderer} = electron;

const commandLine = document.getElementById('command-line');
const readyButton = document.getElementById('readyButton');
const name = document.getElementById('name');
const nameButton = document.getElementById('nameButton');

let ready = false;

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
  console.log(name.value);
  if (name.value.trim() != '')
    ipcRenderer.send('setName', name.value);
});