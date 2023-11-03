#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ThingSpeak.h>

WiFiClient client;  // Declare o objeto WiFiClient

const char* ssid = "Rosivaldo_126_2g";
const char* password = "mshimizu123";
const long CHANNEL_ID = 2328109;
const char* WRITE_API_KEY = "T5ZIR3SAO1X1P548";

const int sensorPin = A0;
const int ledLow = D1;
const int ledMed = D2;
const int ledHigh = D3;
const int pumpPin = D4;

ESP8266WebServer server(80);

bool isPumpOn = false;

const int historySize = 100;
float humidityHistory[historySize];
int historyIndex = 0;
unsigned long lastUpdateTimeThingSpeak = 0; // Declare a variável lastUpdateTimeThingSpeak

void setup() {
  pinMode(ledLow, OUTPUT);
  pinMode(ledMed, OUTPUT);
  pinMode(ledHigh, OUTPUT);
  pinMode(pumpPin, OUTPUT);

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");

  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  Serial.println();
  Serial.println("Envia os dados do sensor para o ThingSpeak usando o ESP8266");
  Serial.println();
  ThingSpeak.begin(client); // Inicializa a biblioteca ThingSpeak

  server.on("/", HTTP_GET, []() {
    float humidity = readHumidity();
    updateHistory(humidity);

    String ledStatus = "Desligado";
    if (digitalRead(ledLow) == HIGH) {
      ledStatus = "Baixo";
    } else if (digitalRead(ledMed) == HIGH) {
      ledStatus = "Médio";
    } else if (digitalRead(ledHigh) == HIGH) {
      ledStatus = "Alto";
    }

    String pumpStatus = isPumpOn ? "Ligada" : "Desligada";

    String html = "<html><body>";
    html += "<h1>Nível de Umidade do Solo</h1>";
    html += "<p>Umidade: " + String(humidity) + "%</p>";
    html += "<p>Estado do LED: " + ledStatus + "</p>";
    html += "<p>Estado da Bomba: " + pumpStatus + "</p>";
    html += "<p>Histórico de Umidade:</p>";
    html += "<ul>";

    for (int i = 0; i < historySize; i++) {
      html += "<li>" + String(humidityHistory[i]) + "%</li>";
    }
    html += "</ul>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long lastUpdateTime = 0;
  if (millis() - lastUpdateTime > 1000) {
    float humidity = readHumidity();
    updateHistory(humidity);

    if (humidity < 30.0) {
      digitalWrite(ledLow, HIGH);
      digitalWrite(ledMed, LOW);
      digitalWrite(ledHigh, LOW);
      digitalWrite(pumpPin, LOW);
      isPumpOn = true;
      Serial.println("Estado: solo seco");
    } else if (humidity < 60.0) {
      digitalWrite(ledLow, LOW);
      digitalWrite(ledMed, HIGH);
      digitalWrite(ledHigh, LOW);
      digitalWrite(pumpPin, HIGH);
      isPumpOn = false;
      Serial.println("Estado: solo úmido");
    } else {
      digitalWrite(ledLow, LOW);
      digitalWrite(ledMed, LOW);
      digitalWrite(ledHigh, HIGH);
      digitalWrite(pumpPin, HIGH);
      isPumpOn = false;
      Serial.println("Estado: solo molhado");
    }

    lastUpdateTime = millis();
  }

  if (millis() - lastUpdateTimeThingSpeak > 10000) {  // Atualize ThingSpeak a cada 30 segundos
    float humidity = readHumidity();
    int sensorValue = analogRead(sensorPin);

    ThingSpeak.setField(1, humidity);
    ThingSpeak.setField(2, sensorValue);

    int x = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);

    if (x == 200) {
      Serial.println("Atualização bem-sucedida no ThingSpeak");
    } else {
      Serial.println("Problema no canal - erro HTTP " + String(x));
    }

    lastUpdateTimeThingSpeak = millis();
  }
}

float readHumidity() {
  int sensorValue = analogRead(sensorPin);
  return 100.0 - (float)sensorValue / 1023.0 * 100.0;
}

void updateHistory(float humidity) {
  humidityHistory[historyIndex] = humidity;
  historyIndex = (historyIndex + 1) % historySize;
}
