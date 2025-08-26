#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//Define OLED parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

//Define PINS
#define BUZZER 18
#define LED_1  19
#define PB_CANCEL 12
#define PB_OK 33
#define PB_UP 32
#define PB_DOWN 35
#define DHTPIN 14
#define LED_LOW 2
#define LED_HIGH 4

//GLobal variables
int hours = 0;
int minutes = 0;
int seconds = 0;
unsigned long timeNow = 0;
unsigned long timeLast = 0;
bool alarm_enabled = true;
int n_alarms = 2;
int alarm_hours[] = {0, 0};
int alarm_minutes[] = {1, 2};
bool alarm_triggered[] = {false, false};
bool alarm_active[] = {true, true};
int snooze_hours[] = {0, 0};
int snooze_minutes[] = {0, 0};
bool snooze_active[] = {false, false};
const int SNOOZE_DURATION = 5; // Snooze time in minutes

//Notes for the alarm tone
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

//Modes in Menu
int current_mode = 0;
int max_modes = 5;
String modes[] = {" 1 - set Time", " 2 - set Alarm 1", " 3 - set Alarm 2", " 4 - disable Alarm", " 5 - set timezone offset" };
 
//Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

//Define WIFI clock labels
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     5*3600 + 30*60  // Colombo time zone (GMT+5:30)
#define UTC_OFFSET_DST 0

//Setup
void setup() {
  // put your setup code here, to run once:
  pinMode(LED_1, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(LED_HIGH, OUTPUT);
  pinMode(LED_LOW, OUTPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println((F("SSD1306 allocation failed")));
    for (;;);
  }

  // Show the display buffer on the screen.
  display.display();
  delay(20);
  display.clearDisplay();

  //WIFI time fetching
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
    display.display();
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);
  display.display();

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  //Clear the buffer
  display.clearDisplay();
  print_line("Welcome to Medibox", 1, 1, 2);
  display.display();
  delay(2000);
  display.clearDisplay();
}

//LOOP
void loop() {
  display.clearDisplay();
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }
  check_temp();
  delay(500); // Reduce flickering by updating less frequently
}

//Print on OLED
void print_line(String text, int text_size, int row, int column) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row); //column and row
  display.println((text));
  // We don't call display.display() here to avoid flickering
}

//Print Time on OLED
void print_time_now(void) {
  print_line(String(hours), 2, 0, 32);
  print_line(":", 2, 0, 52);
  print_line(String(minutes), 2, 0, 65);
  print_line(":", 2, 0, 85);
  print_line(String(seconds), 2, 0, 100);
  display.display(); // Call display only once after all drawing is done
}

//Update time Using WIFI
void update_time(void) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    print_line("Connection Err", 0, 0, 2);
    display.display();
    return;
  }
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);
}

//Update time and Check Alarm
void update_time_with_check_alarm() {
  update_time();
  print_time_now();

  // Check for snoozed alarms
  for (int i = 0; i < n_alarms; i++) {
    if (snooze_active[i] && hours == snooze_hours[i] && minutes == snooze_minutes[i]) {
      ring_alarm(i);
    }
  }

  // Check for regular alarms
  if (alarm_enabled) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_active[i] && alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm(i);
        alarm_triggered[i] = true;
      }
    }
  }
}

//Ring Alarm with snooze and dismiss options
void ring_alarm(int alarm_index) {
  display.clearDisplay();
  print_line("MEDICINE TIME!", 1, 0, 2);
  print_line("OK: Snooze 5 min", 0, 30, 2);
  print_line("CANCEL: Dismiss", 0, 45, 2);
  display.display();

  digitalWrite(LED_1, HIGH);

  bool alarm_responded = false;
  
  while (!alarm_responded) {
    // Check for button presses
    if (digitalRead(PB_OK) == LOW) {
      // Snooze for 5 minutes
      delay(200);
      snooze_alarm(alarm_index);
      alarm_responded = true;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      // Dismiss the alarm
      delay(200);
      alarm_responded = true;
    }
    else {
      // Play alarm tone
      for (int i = 0; i < n_notes; i++) {
        if (digitalRead(PB_OK) == LOW || digitalRead(PB_CANCEL) == LOW) {
          break;
        }
        tone(BUZZER, notes[i]);
        delay(300);
        noTone(BUZZER);
        delay(2);
      }
    }
  }

  digitalWrite(LED_1, LOW);
  noTone(BUZZER);
  display.clearDisplay();
}

// Handle snooze function
void snooze_alarm(int alarm_index) {
  // Calculate time 5 minutes from now
  int snooze_min = minutes + SNOOZE_DURATION;
  int snooze_hr = hours;
  
  // Handle minute overflow
  if (snooze_min >= 60) {
    snooze_min %= 60;
    snooze_hr = (snooze_hr + 1) % 24;
  }
  
  // Set snooze time
  snooze_hours[alarm_index] = snooze_hr;
  snooze_minutes[alarm_index] = snooze_min;
  snooze_active[alarm_index] = true;
  
  display.clearDisplay();
  print_line("Alarm snoozed for", 0, 0, 2);
  print_line("5 minutes", 0, 15, 2);
  display.display();
  delay(1500);
}

//Getting Button press input
int wait_for_button_press(void) {
  while (true) {
    if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return (PB_OK);
    }
    else if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return (PB_UP);
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return (PB_DOWN);
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return (PB_CANCEL);
    }
    update_time();
  }
}

//Menu Display
void go_to_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);
    display.display();

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      current_mode += 1;
      current_mode = current_mode % max_modes;
      delay(200);
    }
    else if (pressed == PB_DOWN) {
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
      delay(200);
    }
    else if (pressed == PB_OK) {
      run_current_mode(current_mode);
      delay(200);
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

//Run current mode
void run_current_mode(int selected_mode) {
  if (selected_mode == 0) {
    set_time();
  }
  if (selected_mode == 1 || selected_mode == 2) {
    set_alarm(selected_mode - 1);
  }
  if (selected_mode == 3) {
    disable_alarm();
  }
  if (selected_mode == 4) {
    set_offset();
  }
}

//Set Time - Menu option
void set_time() {
  int temp_hour = hours;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    display.display();

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      hours = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = minutes;
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);
    display.display();

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      minutes = temp_minute;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time is set", 0, 0, 2);
  display.display();
  delay(1000);
}

//Set alarm - Menu option
void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    display.display();

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;

      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute:" + String(temp_minute), 0, 0, 2);
    display.display();
    
    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      alarm_active[alarm] = true; // Enable the alarm when set
      alarm_triggered[alarm] = false; // Reset trigger status
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  display.display();
  delay(1000);
}

// Function to disable specific alarms
void disable_alarm() {
  int selected_alarm = 0;
  
  // First select which alarm to disable
  while (true) {
    display.clearDisplay();
    String status = alarm_active[selected_alarm] ? "ACTIVE" : "DISABLED";
    print_line("Alarm " + String(selected_alarm + 1) + ": " + status, 0, 0, 2);
    print_line(String(alarm_hours[selected_alarm]) + ":" + String(alarm_minutes[selected_alarm]), 0, 15, 2);
    display.display();
    
    int pressed = wait_for_button_press();
    
    if (pressed == PB_UP || pressed == PB_DOWN) {
      delay(200);
      selected_alarm = (selected_alarm + 1) % n_alarms; // Toggle between alarms
    }
    else if (pressed == PB_OK) {
      delay(200);
      // Toggle the selected alarm's state
      alarm_active[selected_alarm] = !alarm_active[selected_alarm];
      
      display.clearDisplay();
      if (alarm_active[selected_alarm]) {
        print_line("Alarm " + String(selected_alarm + 1) + " enabled", 0, 0, 2);
        alarm_triggered[selected_alarm] = false; // Reset trigger status when enabling
      } else {
        print_line("Alarm " + String(selected_alarm + 1) + " disabled", 0, 0, 2);
        snooze_active[selected_alarm] = false; // Cancel any snooze when disabling
      }
      display.display();
      delay(1000);
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

//Check Temperature and Humidity
void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  
  if (data.temperature > 32) {
    print_line("TEMP HIGH", 0, 40, 1);
    digitalWrite(LED_HIGH, HIGH);
  }
  else if (data.temperature < 24) {
    print_line("TEMP LOW", 0, 40, 1);
    digitalWrite(LED_LOW, HIGH);
  }

  if (data.humidity > 80) {
    print_line("HUMIDITY HIGH", 0, 55, 1);
    digitalWrite(LED_HIGH, HIGH);
  }
  else if (data.humidity < 65) {
    print_line("HUMIDITY LOW", 0, 55, 1);
    digitalWrite(LED_LOW, HIGH);
  }

  display.display(); // Update display once after all text is added
  
  digitalWrite(LED_LOW, LOW);
  digitalWrite(LED_HIGH, LOW);
}

//Setting timeZone offset
void set_offset() {
  int offset_hours;
  int temp_hour = hours;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    display.display();

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      offset_hours = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
  
  int offset_minutes;
  int temp_minute = minutes;
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);
    display.display();

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      offset_minutes = temp_minute;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
  
  int OFFSET = offset_hours * 3600 + offset_minutes * 60;
  configTime(OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  
  display.clearDisplay();
  print_line("Timezone set", 0, 0, 2);
  display.display();
  delay(1000);
}