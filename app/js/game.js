const electron = require('electron');
const {ipcRenderer} = electron;

const commandLine = document.getElementById('command-line');

commandLine.addEventListener('keydown', (e)=>{
  if (e.keyCode == 13) 
    ipcRenderer.send('terminal:command', commandLine.value);
});
