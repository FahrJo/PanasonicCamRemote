let lock = false;
function updateSlider(element, final = false) {
  if (!lock || final){
    lock = true;
    var value = element.value;
    element.parentNode.previousElementSibling.lastElementChild.innerHTML = value;
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (xhr.readyState == XMLHttpRequest.DONE) {
          lock = false;
      }
    }
    xhr.open("GET", "/"+element.id+"?value="+value, true);
    xhr.send();
  }
}
function updateSliderZoom(element, final = false) {
  if (!lock || final){
    lock = true;
    if (final === true) {element.value = 500};
    var zoomValue = element.value;
    element.parentNode.previousElementSibling.lastElementChild.innerHTML = zoomValue;
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (xhr.readyState == XMLHttpRequest.DONE) {
          lock = false;
      }
    }
    xhr.open("GET", "/"+element.id+"?value="+zoomValue, true);
    xhr.send();
  }
}

var intervalId = setInterval(Pinger_ping, 3000);

function Pinger_ping() {
    ping = new XMLHttpRequest();   
    ping.timeout = 1000; 
    ping.onreadystatechange = function(){
        if(ping.readyState == 4){
            if(ping.status == 200){
                document.body.style.backgroundColor = "white";
            }
            else{
                document.body.style.backgroundColor = "red";
            }
        }
    }
    ping.ontimeout = function (e) {
        // XMLHttpRequest timed out. Do something here.
        document.body.style.backgroundColor = "red";
     };
    ping.open("GET", "/ping", true);    
    ping.send();
}
