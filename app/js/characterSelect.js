const electron = require('electron');
const {ipcRenderer} = electron;

const characters = document.getElementsByTagName('div');
const menu = document.getElementById('character-menu');
// console.log(characters);

function removeOthers(index) {
  const numberOfCharacters = characters.length;
  for (let i=0; i<index; i++) {
    menu.removeChild(characters[0]);
  }
  for (let i=0; i<numberOfCharacters-index-1; i++) {
    menu.removeChild(characters[1]);
  }
}

for (let i=0; i<characters.length; i++) {
  characters[i].addEventListener('click', ()=>{
    const characterName = characters[i].id;
    // menu.className += ' selected';
    // removeOthers(i);
    setTimeout(()=>{
      ipcRenderer.send('character:select', characterName);
    }, 800);
  });
}
