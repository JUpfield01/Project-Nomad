#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#define enA 18 
#define in1 23
#define in2 22
#define enB 5  
#define in3 21
#define in4 19

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0
#define REVERSE_LEFT 5
#define REVERSE_RIGHT 6

int pwmVal = 255;

//SSID Config
const char* ssid     = "Nomad AP";
const char* password = "Nomad12345678";

AsyncWebServer server(80);  //Create web server using port 80
AsyncWebSocket ws("/ws");

//Web page which contains slider to control speed (change PWM) of the robot via a slider and control the robots direction via a press & hold button based contoller
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    /* Arrow icon css styling */
    .arrows {
      font-size:70px;
      color:white;
      text-align: center;
    }
    /* Stop icon css styling */
    .stop {
      font-size:50px;
      color:red;
      text-align: center
    }
    /* Control button css styling */
    td {
      background-color:#404040;
      border-radius:25%;
      box-shadow: 5px 5px #888888;
      text-align: center;
    }
    /* Control button css styling (when button is pressed, used to create pressed effect) */
    td:active {
      transform: translate(5px,5px);
      box-shadow: none; 
    }

    /* Configures interactivity of slider per browser to ensure compatability with popular browsers */
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    /* Css container for slider */
    .slidecontainer {
      width: 100%; /* Width of the outside container */
      text-align: center;
    }
    
    /* PWM Slider */
    .slider {
      -webkit-appearance: none;  /* Override default CSS styles */
      appearance: none;
      width: 80%; /* Full-width */
      height: 25px; /* Specified height */
      background: #d3d3d3; /* Grey background */
      outline: none; /* Remove outline */
      opacity: 0.7; /* Set transparency (for mouse-over effects on hover) */
      -webkit-transition: .2s; /* 0.2 seconds transition on hover */
      transition: opacity .2s;
    }
    
    /* Mouse-over effects for slider */
    .slider:hover {
      opacity: 1; /* Fully shown on mouse-over */
    }
    
    /* The slider handle (use -webkit- (Chrome, Opera, Safari, Edge) and -moz- (Firefox) to override default look) */
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none; /* Override default look */
      appearance: none;
      width: 25px; /* Set a specific slider handle width */
      height: 25px; /* Slider handle height */
      background: #008080; /* Green background */
      cursor: pointer; /* Cursor on hover */
    }
    
    .slider::-moz-range-thumb {
      width: 25px; /* Set a specific slider handle width */
      height: 25px; /* Slider handle height */
      background: #008080; /* Green background */
      cursor: pointer; /* Cursor on hover */
    }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <h1 style="color: teal;text-align:center;font-family: Helvetica Neue,Helvetica,Arial,sans-serif;font-weight: bold;">'Nomad' Walker Wi-Fi Controller</h1>
    <h2 style="color: teal;text-align:center;font-family: Helvetica Neue,Helvetica,Arial,sans-serif;font-weight: normal;">by James Upfield</h2>

    <p style="color: teal;text-align:center;font-family: Helvetica Neue,Helvetica,Arial,sans-serif;font-weight: normal;">Press or drag the slider to change the speed of the robot</p>
    <div class="slidecontainer">
      <input type="range" min="100" max="255" value="255" class="slider" id="myRange">
      <p style="color: teal;text-align:center;font-family: Helvetica Neue,Helvetica,Arial,sans-serif;font-weight: normal;font-weight: bold;">Selected PWM Value: <span id="demo"></span></p>
    </div>
    
    <table id="mainTable" style="width:330px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <td ontouchstart='onTouchStartAndEnd("3")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11017;</span></td>
        <td ontouchstart='onTouchStartAndEnd("1")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11014;</span></td>
        <td ontouchstart='onTouchStartAndEnd("4")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11016;</span></td>
      </tr>
      
      <tr>
        <td ontouchstart='onTouchStartAndEnd("0")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" style="color:#404040;" >&#11016;</span></td>  
        <td ontouchstart='onTouchStartAndEnd("0")' ontouchend='onTouchStartAndEnd("0")'><span class="stop" >&#128721;</span></td>   
        <td ontouchstart='onTouchStartAndEnd("0")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" style="color:#404040;" >&#11016;</span></td>  
      </tr>
      
      <tr>
        <td ontouchstart='onTouchStartAndEnd("5")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11019;</span></td>
        <td ontouchstart='onTouchStartAndEnd("2")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11015;</span></td>
        <td ontouchstart='onTouchStartAndEnd("6")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11018;</span></td>
      </tr>
    </table>

    <script>
      var webSocketUrl = "ws:\/\/" + window.location.hostname + "/ws";
      var websocket;

      var slider = document.getElementById("myRange");
      var output = document.getElementById("demo");
      output.innerHTML = slider.value;
      
      slider.oninput = function() {
        output.innerHTML = this.value;
        websocket.send(this.value);
      }
      
      function initWebSocket() 
      {
        websocket = new WebSocket(webSocketUrl);
        websocket.onopen    = function(event){};
        websocket.onclose   = function(event){setTimeout(initWebSocket, 2000);};
        websocket.onmessage = function(event){};
      }

      function onTouchStartAndEnd(value) 
      {
        websocket.send(value);
      }
          
      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
    
  </body>
</html> 

)HTMLHOMEPAGE";


void processCarMovement(String inputValue)
{
  Serial.printf("Got value as %s %d\n", inputValue.c_str(), inputValue.toInt());
  Serial.println("");  
  switch(inputValue.toInt())
  {
    case 1:
      Serial.println("UP");     
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);            
      break;
  
    case 2:
      Serial.println("DOWN"); 
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);  
      break;
  
    case 3:
      Serial.println("LEFT"); 
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
      break;
  
    case 4:
      Serial.println("RIGHT"); 
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      break;

    case 5:
      Serial.println("REVERSE LEFT"); 
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
      break;

    case 6:
      Serial.println("REVERSE RIGHT"); 
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      
      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW);
      break;
  
    case 0:
      Serial.println("STOP");
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);

      digitalWrite(in3, LOW);
      digitalWrite(in4, LOW); 
      break;
  
    default:
      Serial.println("DEFAULT");
      
      pwmVal = inputValue.toInt(); 

      break;
  }
}

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}


void onWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      Serial.println("");
      //client->text(getRelayPinsStatusJson(ALL_RELAY_PINS_INDEX));
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      Serial.println("");
      processCarMovement("0");
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        processCarMovement(myData.c_str());       
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}


void setup(void) 
{
  //setUpPinModes();
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);

  Serial.begin(115200);

  Serial.println("Starting");

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() 
{
  ws.cleanupClients();
  //Serial.printf("enA: %d, enB: %d", pwmVal, pwmVal);
  // Serial.println(""); 
  analogWrite(enA, pwmVal); // Send PWM signal to L298N Enable pin
  analogWrite(enB, pwmVal); // Send PWM signal to L298N Enable pin 
}
