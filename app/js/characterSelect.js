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
  // moveCenter(characters);
}

function moveCenter(chosenCharacter) {
  // chosenCharacter.parentNode.style.display = 
  var pos = 0;
  console.log(chosenCharacter);
  var id = setInterval(()=>{
    // console.log(pos);
    if (pos == 350) {
      clearInterval(id);
    } else {
      pos++;
      chosenCharacter.style.top = pos + 'px';
      chosenCharacter.style.left = pos + 'px';
    }
  }, 10);
}

for (let i=0; i<characters.length; i++) {
  characters[i].addEventListener('click', ()=>{
    const characterName = characters[i].id;
    // removeOthers(i);
    moveCenter(this);
    setTimeout(()=>{
      ipcRenderer.send('character:select', characterName);
    }, 40500);
  });
}
