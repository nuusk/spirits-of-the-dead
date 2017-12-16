const electron = require('electron');
const {ipcRenderer} = electron;

const startButton = document.getElementById('startButton');
const exitButton = document.getElementById('exitButton');

startButton.addEventListener('click', () => {
  ipcRenderer.send('startGame');
});

exitButton.addEventListener('click', () => {
  ipcRenderer.send('exitApp');
})