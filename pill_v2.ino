#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ESP_FlexyStepper.h>

const int  MAX_STRING_LENGTH = 3000;

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Ape Unit";
const char* password = "evolution2010";

const char* PARAM_INPUT_1 = "input1";

// IO pin assignments
const int MOTOR_STEP_PIN = 13;
const int MOTOR_DIRECTION_PIN = 12;

// create the stepper motor object
ESP_FlexyStepper stepper;

// Pill Database
int numPatients = 11;
int patient;
int Pills[] = {3, 7, 5, 1, 0, 2, 9, 4, 6, 8, 10};
int pillAmount;
int stepRotations;
bool flag = false;

// HTML web page to handle an input field (input1)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Pill Dispensary</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
      .body {
      font-family: Helvetica;
      text-align: center;
      }
      .input {
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 1em;
      font-weight: 700;   
      }
    </style>
  </head><body class="body">
  <h1>Hello.</h1>
  <h2>Please enter your assigned Patient Number. <br> <br> <form action="/get">
    Patient #: <input type="text" name="input1" class="input">
    <input type="submit" value="Submit" class="input">
  </form></h2>
</body></html>)rawliteral";


char result_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Success: Pill Dispensary</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
   <style>
      .body {
      font-family: Helvetica;
      text-align: center;
      }
      .button {
      border: none;
      color: red;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 1.5em;
      font-weight: 700;
      }
    </style>
  </head><body class="body">
  <h1>Welcome back!</h1>
  <h2>Today you're receiving **PILLAMOUNT** pills. <br> Get well soon.</h2>
  <a href="/" class="button">Return</a>
</body>
</html>)rawliteral";

const char error_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Error: Pill Dispensary</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
      .body {
      font-family: Helvetica;
      text-align: center;
      }
      .button {
      border: none;
      color: red;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 1.5em;
      font-weight: 700;
      }
    </style>
  </head><body class="body">
  <h1>Sorry!</h1>
  <h2> I do not know your medical information. <br> Please contact your healthcare professional.</h2>
  <a href="/" class="button">Return</a>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void str_replace(char *src, char *oldchars, char *newchars) { // utility string function
  char *p = strstr(src, oldchars);
  char buf[MAX_STRING_LENGTH];
  do {
    if (p) {
      memset(buf, '\0', strlen(buf));
      if (src == p) {
        strcpy(buf, newchars);
        strcat(buf, p + strlen(oldchars));
      } else {
        strncpy(buf, src, strlen(src) - strlen(p));
        strcat(buf, newchars);
        strcat(buf, p + strlen(oldchars));
      }
      memset(src, '\0', strlen(src));
      strcpy(src, buf);
    }
  } while (p && (p = strstr(src, oldchars)));
}


void setup() {
  Serial.begin(115200);
  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;

    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      pillAmount = Pills[inputMessage.toInt()];
      patient = inputMessage.toInt();

      // conversion of int to char
      char pAm[5]; // conversion of an integer with up to 2 digits (-9, 99)
      String pillStr;
      pillStr = String(pillAmount);
      pillStr.toCharArray(pAm, 5);
      Serial.println(pAm);

      // Inserting the pill amount variable into HTML
      if (patient <= numPatients && patient >= 0) {
        str_replace(result_html, "**PILLAMOUNT**", pAm);
        request->send(200, "text/html", result_html);
        //flag = false;
        
        stepRotations = pillAmount * 200;
        stepper.setSpeedInRevolutionsPerSecond(1.0);
        stepper.setAccelerationInRevolutionsPerSecondPerSecond(2.0);
        stepper.setStepsPerRevolution(220);
        
        stepper.moveRelativeInRevolutions(pillAmount);
        
        Serial.println(pillAmount);
        ESP.restart();
      }
      else if (patient >= numPatients + 1 || pillAmount <= -1) {
        flag = true;
        request->send(200, "text/html", error_html);
      }
    }
    else {
      inputMessage = "No data inserted";
      inputParam = "none";
    }
  });

  server.onNotFound(notFound);
  server.begin();

}

void loop() {
  //stepper.processMovement();

  /*if (pillAmount >= 1 && !flag) {
    flag = true;

   
   //stepper.setSpeedInStepsPerSecond(350);
   // stepper.setAccelerationInStepsPerSecondPerSecond(100);
    //stepper.moveRelativeInRevolutions(stepRotations);
    //delay(5000);


    Serial.print(millis());
    Serial.print("\t");
    Serial.println(stepRotations);
  }*/
  

}
