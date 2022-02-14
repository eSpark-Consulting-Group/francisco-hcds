#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>
#include <driver/rtc_io.h>


#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  15        /* Time ESP32 will go to sleep (in seconds) */
#define USE_SERIAL Serial

#define BLUE_LED 14
#define GREEN_LED GPIO_NUM_21
#define ORANGE_LED GPIO_NUM_20
#define RED_LED GPIO_NUM_19

WiFiMulti wifiMulti;

String macId = "";
String url = "http://35.184.38.177:8000/";

boolean gpio_interr = false;
boolean timer_interr = false;

//RTC_DATA_ATTR

const int ESP_LED_PIN = 19;
//const int BUZZER = 12;
int d = 1;

const int BATTERY_PIN = 1;
float BATTERY_VOLTAGE;
String BATTERY_STATUS;



//*******************************************************************************
void measureBattery()
{
  int BATTERY_ADC = analogRead(BATTERY_PIN);    //takes ADC value (from 3.3v pin)

  float SCALING_FACTOR = 2.3 / 7200;            // Scaling ADC value for 4.5v (AAA battery)

  float BATTERY_ADC_VOLTAGE = BATTERY_ADC * SCALING_FACTOR;     //(Latest voltage of AAA battery)

  float VOLTAGE_CONVERSION_FACTOR = 4.5 / 2.3 ;

  BATTERY_VOLTAGE = BATTERY_ADC_VOLTAGE * VOLTAGE_CONVERSION_FACTOR + 0.5;

  Serial.println("ADC battery voltage");
  Serial.println(BATTERY_VOLTAGE);

  if (BATTERY_VOLTAGE > 3.9)
  {
    BATTERY_STATUS = "good";

    //    digitalWrite(GREEN_LED, LOW);
    //    digitalWrite(RED_LED, HIGH);
    //    digitalWrite(ORANGE_LED, HIGH);

    rtc_gpio_set_level(GREEN_LED, LOW);
    rtc_gpio_set_level(RED_LED, HIGH);
    rtc_gpio_set_level(ORANGE_LED, HIGH);

  }

  else if ((BATTERY_VOLTAGE >= 3.3) && (BATTERY_VOLTAGE <= 3.9))
  {
    BATTERY_STATUS = "medium";

    //    digitalWrite(GREEN_LED, HIGH);
    //    digitalWrite(RED_LED, HIGH);
    //    digitalWrite(ORANGE_LED, LOW);

    rtc_gpio_set_level(GREEN_LED, HIGH);
    rtc_gpio_set_level(RED_LED, HIGH);
    rtc_gpio_set_level(ORANGE_LED, LOW);
  }

  else if (BATTERY_VOLTAGE < 3.3)
  {
    BATTERY_STATUS = "low";

    //    digitalWrite(GREEN_LED, HIGH);
    //    digitalWrite(RED_LED, LOW);
    //    digitalWrite(ORANGE_LED, HIGH);

    rtc_gpio_set_level(GREEN_LED, HIGH);
    rtc_gpio_set_level(RED_LED, LOW);
    rtc_gpio_set_level(ORANGE_LED, HIGH);
  }

}


void print_wakeup_reason() {    //*****************************************************************************
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :
      Serial.println("Wakeup caused by external signal using RTC_IO");
      gpio_interr = true;
      break;

    case ESP_SLEEP_WAKEUP_EXT1 :
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;

    case ESP_SLEEP_WAKEUP_TIMER :
      Serial.println("Wakeup caused by timer");
      timer_interr = true;
      break;

    case ESP_SLEEP_WAKEUP_TOUCHPAD :
      Serial.println("Wakeup caused by touchpad");
      break;

    case ESP_SLEEP_WAKEUP_ULP :
      Serial.println("Wakeup caused by ULP program");
      break;

    default :
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
  }
}

//********************************************************************************
boolean checkMacId(String macId)
{
  HTTPClient http;
  boolean already_added = false;

  Serial.println("Sending request");
  //  if (WiFi.status() == WL_CONNECTED)
  //  {
  Serial.println("Inside request");
  http.begin(url + "sensor_info/get_sensors_by_id/5b09nPavNJHKx8leuYLK");

  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);

      if (payload.indexOf(macId) > 0)
      {
        already_added = true;
      }
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

  }

  http.end();
  return already_added;
  // }
}

//**********************************************************************************
void createSensor(String macId)
{

  HTTPClient http;
  String requestData = "{\"mac_address\":\"" + macId + "\" ,\"rack_block_id\": \"5b09nPavNJHKx8leuYLK\"}";

  USE_SERIAL.print("[HTTP] begin...\n");
  USE_SERIAL.println(requestData);
  Serial.println(requestData.length());
  http.begin(url + "sensor_info/create_sensor"); //HTTP

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(requestData.length()));
  int httpCode = http.POST(requestData);

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
    }
  } else {
    USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}


//************************************************************************************
void Update_Power_Up(String macId)
{
  HTTPClient http;
  //  measureBattery();
  String requestData = "{\"mac_address_id\":\"" + macId + "\" , \"location_id\":\"PabBqnXhbmASUP9WPHO4\" , \"severity\":\"NULL\" , \"part_damaged\":\"NULL\" , \"notes\":\"NULL\" , \"status\":\"ok\" , \"battery_status\":\"" + BATTERY_STATUS + "\"}";

  USE_SERIAL.print("[HTTP] begin...\n");
  USE_SERIAL.println(requestData);
  Serial.println(requestData.length());
  http.begin(url + "sensor_info/create_incident"); //HTTP

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(requestData.length()));
  int httpCode = http.POST(requestData);

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);
    //  Serial.println("Incident happened" + macId);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
    }
  } else {
    USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}


//************************************************************************************
void Update_Timer(String macId)
{
  HTTPClient http;
  //  measureBattery();
  String requestData = "{\"mac_address_id\":\"" + macId + "\" , \"location_id\":\"PabBqnXhbmASUP9WPHO4\" , \"severity\":\"NULL\" , \"part_damaged\":\"NULL\" , \"notes\":\"NULL\" , \"status\":\"NULL\" , \"battery_status\":\"" + BATTERY_STATUS + "\"}";

  USE_SERIAL.print("[HTTP] begin...\n");
  USE_SERIAL.println(requestData);
  Serial.println(requestData.length());
  http.begin(url + "sensor_info/create_incident"); //HTTP

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(requestData.length()));
  int httpCode = http.POST(requestData);

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);
    //  Serial.println("Incident happened" + macId);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
    }
  } else {
    USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}


//************************************************************************************
void Update_Shock(String macId)
{
  HTTPClient http;
  //  measureBattery();
  String requestData = "{\"mac_address_id\":\"" + macId + "\" , \"location_id\":\"PabBqnXhbmASUP9WPHO4\" , \"severity\":\"NULL\" , \"part_damaged\":\"NULL\" , \"notes\":\"NULL\" , \"status\":\"New Impact\" , \"battery_status\":\"NULL\"}";

  USE_SERIAL.print("[HTTP] begin...\n");
  USE_SERIAL.println(requestData);
  Serial.println(requestData.length());
  http.begin(url + "sensor_info/create_incident"); //HTTP

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", String(requestData.length()));
  int httpCode = http.POST(requestData);

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);
    //  Serial.println("Incident happened" + macId);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
    }
  } else {
    USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}




void setup() {  //********************************************************************************************************************************************



  USE_SERIAL.begin(115200);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  measureBattery();

  wifiMulti.addAP("eSpark Consulting", "47356880"); // SSID and Pass here





  pinMode(BLUE_LED, OUTPUT);
  //  pinMode(RED_LED, OUTPUT);
  //  pinMode(GREEN_LED, OUTPUT);
  //  pinMode(ORANGE_LED, OUTPUT);

  rtc_gpio_init(RED_LED);
  rtc_gpio_init(GREEN_LED);
  rtc_gpio_init(ORANGE_LED);

  rtc_gpio_set_direction(RED_LED, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_direction(GREEN_LED, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_direction(ORANGE_LED, RTC_GPIO_MODE_OUTPUT_ONLY);

  macId = WiFi.macAddress();

  print_wakeup_reason();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_11, 0); //1 = High, 0 = Low
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");

  /*
    Next we decide what all peripherals to shut down/keep on
    By default, ESP32 will automatically power down the peripherals
    not needed by the wakeup source, but if you want to be a poweruser
    this is for you. Read in detail at the API docs
    http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
    Left the line commented as an example of how to configure peripherals.
    The line below turns off all RTC peripherals in deep sleep.
  */
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");

  /*
    Now that we have setup a wake cause and if needed setup the
    peripherals state in deep sleep, we can now start going to
    deep sleep.
    In the case that no wake up sources were provided but deep
    sleep was started, it will sleep forever unless hardware
    reset occurs.
  */

  /*  WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }*/

  if ((wifiMulti.run() == WL_CONNECTED)) {

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());


    if (!checkMacId(macId))     //   && gpio_interr (Seems like it's Not Need in power up)
    {
      // gpio_interr = false;
      Serial.println("******************************************************************************");
      Serial.println("First time Power up Update");
      createSensor(macId);
      Serial.println("******************************************************************************");
    }

    else if (timer_interr)
    {
      Serial.println("******************************************************************************");
      timer_interr = false;
      Serial.println("Battery Update (Timer)");
      Update_Timer(macId);
      Serial.println("******************************************************************************");
    }

    else if (gpio_interr)
    {
      Serial.println("******************************************************************************");
      gpio_interr = false;
      Serial.println("Shock Update");
      Update_Shock(macId);
      Serial.println("******************************************************************************");
      int t1 = 0;
    }
    else
    {
      //Serial.println("else");
    }

    delay(5000);
    Serial.println("Going to sleep now");
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");

  }

}

void loop() {
  //This is not going to be called
}