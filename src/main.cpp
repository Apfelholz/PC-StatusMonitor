#include <Arduino.h>
#include <LiquidCrystal.h>

// Hardware mapping
const int BOOL_LED_PIN = 6; // LED channel LED1 -> D6

// LCD pins (adjust if needed)
const int LCD_RS = 7;
const int LCD_EN = 8;
const int LCD_D4 = 9;
const int LCD_D5 = 10;
const int LCD_D6 = 11;
const int LCD_D7 = 12;

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Serial input buffer
String serialBuffer = "";

// Channel storage
const int MAX_CHANNELS = 20;

struct Channel {
  String name;
  String type; // "BOOL", "PERCENT", "VALUE", "TEXT"
  String payload;
  int scrollOffset;
};

Channel channels[MAX_CHANNELS];
int channelCount = 0;

// Display rotation
const unsigned long PAGE_ROTATE_MS = 3000;
unsigned long lastPageRotate = 0;
int pageIndex = 0; // which page (2 entries per page)

// Helpers
int findChannelIndex(const String &name) {
  for (int i = 0; i < channelCount; ++i) {
    if (channels[i].name.equalsIgnoreCase(name)) return i;
  }
  return -1;
}

void ensureChannel(const String &name, const String &type, const String &payload) {
  int idx = findChannelIndex(name);
  if (idx >= 0) {
    // existing
    if (!channels[idx].payload.equals(payload)) {
      channels[idx].payload = payload;
      channels[idx].type = type;
      channels[idx].scrollOffset = 0;
    }
    return;
  }
  if (channelCount >= MAX_CHANNELS) return; // drop if full
  channels[channelCount].name = name;
  channels[channelCount].type = type;
  channels[channelCount].payload = payload;
  channels[channelCount].scrollOffset = 0;
  channelCount++;
}

void updateBoolHardware(const String &name, const String &payload) {
  if (name.equalsIgnoreCase("LED1")) {
    bool v = (payload == "1" || payload.equalsIgnoreCase("true"));
    digitalWrite(BOOL_LED_PIN, v ? HIGH : LOW);
  }
}

String formatEntryForLCD(Channel &ch) {
  String out = "";
  if (ch.type.equalsIgnoreCase("BOOL")) {
    String v = (ch.payload == "1" || ch.payload.equalsIgnoreCase("true")) ? "ON" : "OFF";
    out = ch.name + ":" + v;
  } else if (ch.type.equalsIgnoreCase("PERCENT")) {
    int p = ch.payload.toInt();
    out = ch.name + ":" + String(p) + "%";
  } else if (ch.type.equalsIgnoreCase("VALUE")) {
    out = ch.name + ":" + ch.payload;
  } else if (ch.type.equalsIgnoreCase("TEXT")) {
    // for TEXT, handle scroll if longer than 16
    if (ch.payload.length() <= 16) {
      out = ch.payload;
    } else {
      int L = ch.payload.length();
      int off = ch.scrollOffset % L;
      out = "";
      for (int i = 0; i < 16; ++i) {
        int pos = (off + i) % L;
        out += ch.payload.charAt(pos);
      }
      // advance scroll for next time
      ch.scrollOffset = (ch.scrollOffset + 1) % L;
    }
  } else {
    out = ch.name + ":" + ch.payload;
  }
  // ensure exactly 16 chars
  if (out.length() < 16) {
    int pad = 16 - out.length();
    for (int i = 0; i < pad; ++i) out += ' ';
  } else if (out.length() > 16) {
    out = out.substring(0, 16);
  }
  return out;
}

void drawPage(int page) {
  int perPage = 2;
  for (int line = 0; line < 2; ++line) {
    int idx = page * perPage + line;
    lcd.setCursor(0, line);
    if (idx < channelCount) {
      String s = formatEntryForLCD(channels[idx]);
      lcd.print(s);
    } else {
      // clear line
      lcd.print("                ");
    }
  }
}

void processCommand(const String &cmd) {
  // Expected format: SET:CHANNEL:TYPE:PAYLOAD
  if (!cmd.startsWith("SET:")) return;
  int idx = 4; // after "SET:"
  int p1 = cmd.indexOf(':', idx);
  if (p1 < 0) return;
  String channel = cmd.substring(idx, p1);
  int p2 = cmd.indexOf(':', p1 + 1);
  if (p2 < 0) return;
  String type = cmd.substring(p1 + 1, p2);
  String payload = cmd.substring(p2 + 1);
  type.trim(); channel.trim(); payload.trim();

  // Update BOOL hardware immediately if applicable
  if (type.equalsIgnoreCase("BOOL")) updateBoolHardware(channel, payload);

  // Store/update channel
  ensureChannel(channel, type, payload);
}

void setup() {
  pinMode(BOOL_LED_PIN, OUTPUT);
  digitalWrite(BOOL_LED_PIN, LOW);
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for PC...");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lastPageRotate = millis();
}

void loop() {
  // Read serial into buffer until newline
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        processCommand(serialBuffer);
        serialBuffer = "";
        // redraw immediately first page when new data arrives
        pageIndex = 0;
        drawPage(pageIndex);
      }
    } else {
      serialBuffer += c;
      if (serialBuffer.length() > 256) serialBuffer = serialBuffer.substring(serialBuffer.length() - 256);
    }
  }

  unsigned long now = millis();
  unsigned long pages = (channelCount + 1) / 2; // ceil
  if (pages == 0) pages = 1;
  if (now - lastPageRotate >= PAGE_ROTATE_MS) {
    lastPageRotate = now;
    pageIndex = (pageIndex + 1) % pages;
    drawPage(pageIndex);
  }
}