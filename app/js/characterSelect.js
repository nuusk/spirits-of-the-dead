const electron = require('electron');
const {ipcRenderer} = electron;

const characters = document.getElementsByTagName('div');

for (let i=0; i<characters.length; i++) {
  characters[i].addEventListener('click', ()=>{
    const characterName = characters[i].id;
    setTimeout(()=>{
      ipcRenderer.send('character:select', characterName);
    }, 800);
  });
}
