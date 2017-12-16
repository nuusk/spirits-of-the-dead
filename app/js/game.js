const electron = require('electron');
const {ipcRenderer} = electron;

const commandLine = document.getElementById('command-line');
console.log(commandLine);

commandLine.addEventListener('onkeydown', (e)=>{
  console.log(e);
  if (e.keyCode == 13) {
    ipcRenderer.send('terminal:command', this.value);
  }
});
