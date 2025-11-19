// Minimalversion - Boolean-Steuerung über Serial
// LED an Pin 6

const int BOOL_LED_PIN = 6;

struct BoolElement {
  String id;
  bool state;
};

BoolElement boolElements[5];
int elementCount = 0;

void setup() {
  Serial.begin(9600);
  pinMode(BOOL_LED_PIN, OUTPUT);
  digitalWrite(BOOL_LED_PIN, LOW);
}

void loop() {
  handleSerial();
  updateLEDs();
}

void handleSerial() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("SET:")) {
      parseCommand(cmd);
    }
  }
}

void parseCommand(String cmd) {
  int p1 = cmd.indexOf(':');
  int p2 = cmd.indexOf(':', p1 + 1);
  int p3 = cmd.indexOf(':', p2 + 1);

  String id = cmd.substring(p1 + 1, p2);
  String type = cmd.substring(p2 + 1, p3);
  String value = cmd.substring(p3 + 1);

  if (type == "BOOL") {
    bool state = (value == "1");
    saveOrUpdateBool(id, state);
  }
}

void saveOrUpdateBool(String id, bool state) {
  for (int i = 0; i < elementCount; i++) {
    if (boolElements[i].id == id) {
      boolElements[i].state = state;
      return;
    }
  }
  boolElements[elementCount++] = {id, state};
}

void updateLEDs() {
  for (int i = 0; i < elementCount; i++) {
    if (boolElements[i].id == "LED1") {
      digitalWrite(BOOL_LED_PIN, boolElements[i].state ? HIGH : LOW);
    }
  }
}
