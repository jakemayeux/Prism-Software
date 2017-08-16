const applyButton = document.querySelector('#apply')
const fileName = document.querySelector('#fileName')
const fileInput = document.querySelector('#fileInput')
const globalOptions = document.querySelector('#globalOptions')
const colorDropdown = document.querySelector('#colors .dropdown')
const colorDroparrow = document.querySelector('#colors .droparrow')
const colorSelect = document.querySelector('#colors .dropselect')
const mixColors = document.querySelector('#mixColors')
const fileOutput = document.querySelector('#fileOutput')
const loader = document.querySelector('#loader')
const child_process = require('child_process')
const fs = require('fs')
const os = require('os')
const colorPresetFolder = 'colors'
const bin = {
  purge: '../src/purge-tower.out',
  reorder: 'python ../src/reorder.py',
  zmix: 'python ../src/mixing-v2.py'
}

var colorPresets = []

colorDroparrow.classList.add('show')

//remember settings
for(i of document.querySelectorAll('input')){
  if(i.name == ""){
    continue
  }
  let x = localStorage.getItem('input'+i.name)
  if(x){
    i.value = x
  }
}
for(i of document.querySelectorAll('input')){
  i.onchange = storeSetting
}

function storeSetting(){
  localStorage.setItem('input'+this.name, this.value)
}

try{
  fs.mkdirSync(os.tmpdir()+'/prism')
}catch(e){} //if we get an error the dir is already made

fs.readdir(colorPresetFolder, (err, files) => {
  files.forEach(file => {
    colorPresets.push(file)
    let e = document.createElement('div')
    e.innerText = file.replace('.txt', '')
    e.onclick = selectDropdown
    colorDropdown.appendChild(e)
  })
  if(localStorage.getItem('lastColor') != null){
    colorSelect.innerText = localStorage.getItem('lastColor')
  }else{
    colorSelect.innerText = colorDropdown.children[1].innerText
  }
  displayColors(colorSelect.innerText)
})

function selectDropdown(){
  colorDropdown.classList.remove('show')
  colorDroparrow.classList.add('show')
  colorSelect.innerText = this.innerText
  displayColors(this.innerText)
}

function displayColors(file){
  localStorage.setItem('lastColor', file.replace('.txt', ''))
  file += '.txt'
  fs.readFile(colorPresetFolder+'/'+file, 'utf8', function(err, data){
    while(mixColors.children.length > 0){
      mixColors.removeChild(mixColors.children[0])
    }
    for(i of data.split('\n')){
      if(i == ''){
        continue
      }
      let l = i.split(',')
      let total = 0
      for(x of l){
        total += parseFloat(x)
      }
      let w = l[0]/total
      let c = l[2]/total - w
      let m = l[3]/total - w
      let y = l[4]/total - w
      let k = l[1]/total
      c*=100
      m*=100
      y*=100
      k*=100
      let rgb = ColorConverter.toRGB(new CMYK(c,m,y,k))
      let e = document.createElement('div')
      e.classList.add('aColor')
      e.style.backgroundColor = 'rgb('+rgb.r+','+rgb.g+','+rgb.b+')'
      mixColors.appendChild(e)
    }
    mixColors.children[0].innerText = 'Layer 1'
    mixColors.children[mixColors.children.length-1].innerText = 'Layer '+mixColors.children.length
  })
}

colorDroparrow.onclick =()=> {
  colorDropdown.classList.add('show')
  colorDroparrow.classList.remove('show')
}

for (i of document.querySelectorAll('.checkbox')) {
  i.onclick = toggleCheck
}

function toggleCheck() {
  this.classList.toggle('checked')
}

applyButton.onclick =()=> {
  var infile = fileInput.files[0].path
  var outfile = fileOutput.value

  var jobs = []
  for(i of document.querySelectorAll('.right .button')){
    if(checked(i)){
      jobs.push(i.id)
    }
  }

  var files = [infile]
  if(jobs.length == 1){
    files.push(outfile)
  }else if(jobs.length == 2){
    files.push(os.tmpdir()+'/prism/tmp1.gcode')
    files.push(outfile)
  }else{
    files.push(os.tmpdir()+'/prism/tmp1.gcode')
    files.push(os.tmpdir()+'/prism/tmp2.gcode')
    files.push(outfile)
  }
  loading(true)
  runScripts(jobs, files)
}

function runScripts(jobs, files){
  console.log(jobs, files)
  let job = jobs.shift()
  let infile = files[0]
  let outfile = files[1]
  let proc = child_process.exec(bin[job]+getOptions('#'+job, infile, outfile))
  console.log(bin[job]+getOptions('#'+job, infile, outfile))
  proc.stdout.on('data', (data)=>{
    console.log(data)
  })
  proc.stderr.on('data', (data)=>{
    console.log(data)
  })
  proc.on('exit', ()=>{
    console.log(job + ' done')
    if(files.length > 3){
      files.shift()
    }
    if(jobs.length == 1){
      files = files.slice(-2)
    }
    if(jobs.length > 0){
      runScripts(jobs, files)
    }else{
      loading(false)
    }
  })
}

function loading(x){
  if(x){
    loader.style.display='inline-block'
  }else{
    loader.style.display='none'
  }
}

fileInput.onchange =()=> {
  //splits the full path into array
  //regex matches "/" and "\"
  //pops the last element which would be the filename
  fileName.innerText = fileInput.files[0].name
}

function getOptions(selector, inf, ouf){
  let e = document.querySelector(selector)
  let ret = ' -f ' + inf
  ret += ' -o ' + ouf
  for(i of e.querySelectorAll('input')){
    if(i.attributes.type.value == 'checkbox'){
      ret += ' ' + i.name
    }else{
      ret += ' ' + i.name + ' ' + i.value
    }
  }
  return ret
}

//returns whether or not the element has a child with the checked class
//accepts a string selector or the element itself
function checked(x){
  var e
  if(typeof(x) == 'string'){
    e = document.querySelector(x)
  }else{
    e = x
  }
  // the "!!" converts to bool
  return !!e.querySelector('.checked')
}

window.onkeydown =(e)=> {
  //console.log(e.keyCode)

  switch(e.keyCode){
    case 27:
      colorDropdown.classList.remove('show')
      colorDroparrow.classList.add('show')
  }
}
