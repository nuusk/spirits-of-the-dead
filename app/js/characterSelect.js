const electron = require('electron');
const {ipcRenderer} = electron;

const characters = document.getElementsByTagName('div');
// console.log(characters);

for (let i=0; i<characters.length; i++) {
  characters[i].addEventListener('click', ()=>{
    // console.log(characters[i]);
    const characterName = characters[i].id;
    console.log(characterName);
    ipcRenderer.send('character:select', characterName);
  });
}
