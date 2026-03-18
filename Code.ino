#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WebServer.h>

// ---------------- WIFI ----------------
const char* ssid = "ESP32_Medicine";
const char* password = "12345678";

// ---------------- SERVO ----------------
Servo servos[3];
int servoPins[3] = {13, 12, 14};

// Quick open-close (ONCE → one tablet)
int closePos = 90;     // closed
int openPos = 120;     // small opening → adjust 115–140 for your tablets

// ---------------- BUZZER ----------------
int buzzerPin = 4;

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- WEB SERVER ----------------
WebServer server(80);

String medicineList[3] = { "Paracetamol 500mg","Betacap 100mg","Metformin 500mg" };

// -------------------- WEB PAGE --------------------
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Medicine Dispenser</title>
<style>
  body {background:#000;color:#0ff;font-family:Poppins,sans-serif;text-align:center;margin:0;}
  h1 {color:#00ffff;margin-top:30px;text-shadow:0 0 10px #00ffff;}
  .button-container {display:flex;flex-wrap:wrap;justify-content:center;margin-top:40px;gap:20px;}
  button {background:#111;border:2px solid #0ff;color:#0ff;padding:15px 25px;border-radius:12px;font-size:18px;cursor:pointer;transition:0.3s;box-shadow:0 0 15px #0ff4;}
  button:hover {background:#0ff;color:#000;box-shadow:0 0 25px #0ff;}
  #status {margin-top:30px;font-size:20px;color:#0f0;text-shadow:0 0 10px #0f0;}
  .sos-btn {background:red;color:white;border:2px solid #f00;box-shadow:0 0 15px #f00;}
  .sos-btn:hover {background:#ff4444;box-shadow:0 0 25px #f00;}
</style>
</head>
<body>
  <h1>💊 ESP32 Medicine Dispenser</h1>
  <p>Select a medicine:</p>
  <div class="button-container">
    <button onclick="dispense('fever')">Paracetamol 500mg</button>
    <button onclick="dispense('migrane')">Betacap 100mg</button>
    <button onclick="dispense('diabetes')">Metformin 500mg</button>
    <button class="sos-btn" onclick="dispense('sos')">🚨 SOS</button>
  </div>
  <div id="status">Status: Ready</div>
<script>
function dispense(name){
  document.getElementById("status").innerText="Sending "+name+"...";
  fetch("/dispense?name="+name)
  .then(res=>res.text())
  .then(data=>{
    document.getElementById("status").innerText="✅ "+data;
  })
  .catch(err=>{
    document.getElementById("status").innerText="⚠️ Connection Error";
  });
}
</script>
</body>
</html>
)rawliteral";

// ---------------- BUZZER ----------------
void buzz(int duration){
  digitalWrite(buzzerPin, HIGH);
  delay(duration);
  digitalWrite(buzzerPin, LOW);
}

// ---------- FAST DISPENSE ONE TABLET ----------
void dispenseOneTablet(int index) {

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Dispensing:");
  lcd.setCursor(0,1); lcd.print(medicineList[index]);

  // QUICK OPEN
  servos[index].write(openPos);
  delay(120);  // open only briefly → only 1 tablet falls

  // IMMEDIATE CLOSE
  servos[index].write(closePos);
  delay(120);

  buzz(150);

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Done:");
  lcd.setCursor(0,1); lcd.print(medicineList[index]);
}

// ---------------- SERVER HANDLER ----------------
void handleMedicine(){
  String med = server.arg("name");
  med.toLowerCase();

  if(med == "fever") {
    dispenseOneTablet(0);
    server.send(200,"text/plain","Paracetamol Dispensed");
    return;
  }
  if(med == "migrane") {
    dispenseOneTablet(1);
    server.send(200,"text/plain","Betacap Dispensed");
    return;
  }
  if(med == "diabetes") {
    dispenseOneTablet(2);
    server.send(200,"text/plain","Metformin Dispensed");
    return;
  }
  if(med == "sos"){
    buzz(1000);
    lcd.clear();
    lcd.print("SOS ALERT!");
    server.send(200,"text/plain","SOS Triggered");
    return;
  }

  server.send(404,"text/plain","Unknown command");
}

// ---------------- ROOT PAGE ----------------
void handleRoot(){
  server.send_P(200, "text/html", MAIN_page);
}

// ---------------- SETUP ----------------
void setup(){
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  Serial.println("AP Started");
  Serial.println(WiFi.softAPIP());

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Medicine Dispenser");
  lcd.setCursor(0,1); lcd.print("Ready...");

  pinMode(buzzerPin, OUTPUT);

  for(int i=0; i<3; i++){
    servos[i].attach(servoPins[i]);
    servos[i].write(closePos);
  }

  server.on("/", handleRoot);
  server.on("/dispense", handleMedicine);
  server.begin();
}

// ---------------- LOOP ----------------
void loop(){
  server.handleClient();
}
