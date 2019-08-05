let hue = 130
let sat = 100
let light = 55
let alpha = 1

const colorChange = () => {
  document.querySelector('.colorname').textContent = getHSL()
  //alert(getHSL())
  if(websock)
websock.send("hsv "+Math.round(hue*255/360)+" " +Math.round(sat*255/100)+" "+Math.round(light*255/100)+" "+alpha)
 console.log("hsv "+Math.round(hue*255/360)+" " +Math.round(sat*255/100)+" "+Math.round(light*255/100)+" "+alpha)
  const swatch = document.querySelector('.swatch')
  swatch.style.backgroundColor = getHSL()
  document.querySelector('.satcolor').style.backgroundColor = getHSL()
}
/*const incrementPlayerCount = () => {
  console.log('Player Wins')
  const playerTextScore = $('.scores .player').textContent
  const playerScore = parseInt(playerTextScore) + 1
  $('.scores .player').textContent = playerScore
  if (playerScore === 2) {
    gameOver(true)
  }
}*/

//hsl (128,100 %, 50 %)
const getHSL = () => {
  return `hsla(${hue}, ${sat}%, ${light}%, ${alpha})`
}

const main = () => {
  const hueInput = document.querySelector('input[name=hue]')
  hueInput.addEventListener('input', () => {
    hue = hueInput.value
    console.log(getHSL())
    colorChange()
  })

  const satInput = document.querySelector('input[name=sat]')
  satInput.addEventListener('input', () => {
    sat = satInput.value
    console.log(getHSL())
    colorChange()
  })

  const lightInput = document.querySelector('input[name=light]')
  lightInput.addEventListener('input', () => {
    light = lightInput.value
    console.log(getHSL())
    colorChange()
  })

  const alphaInput = document.querySelector('input[name=alpha]')
  alphaInput.addEventListener('input', () => {
    alpha = alphaInput.value
    console.log(getHSL())
    colorChange()
  })

  colorChange()
}

document.addEventListener('DOMContentLoaded', main)
