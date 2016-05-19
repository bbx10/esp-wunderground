/*
 * Get current weather conditions and forecasts from Weather Underground.
 * This version uses ESP8266HTTPClient library to handle the HTTP protocol.
 */

const char SSID[] = "********";
const char PASSWORD[] = "***************";
// Use your own API key by signing up for a free developer account.
// http://www.wunderground.com/weather/api/
#define WU_API_KEY "****************"

#define WU_LOCATION "Antarctica/McMurdo"
#define FORECAST_INTERVAL   (60.0*60.0) // 1 hour in seconds
#define CURRENT_INTERVAL    (15.0*60.0) // 15 minutes in seconds

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

Ticker ForecastTicker, CurrentTicker;
bool ForecastCheck = true, CurrentCheck = true;

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

void getForecast() {
    ForecastCheck = true;
}

void getCurrent() {
    CurrentCheck = true;
}

void setup() {
    USE_SERIAL.begin(115200);
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFiMulti.addAP(SSID, PASSWORD);

    ForecastTicker.attach(FORECAST_INTERVAL, getForecast);
    CurrentTicker.attach(CURRENT_INTERVAL, getCurrent);
}

#define WUNDERGROUND "api.wunderground.com"

const char WUNDERGROUND_FORECAST_URL[] =
    "http://api.wunderground.com/api/" WU_API_KEY "/forecast/q/" WU_LOCATION ".json";

const char WUNDERGROUND_CURRENT_URL[] =
    "http://api.wunderground.com/api/" WU_API_KEY "/conditions/q/" WU_LOCATION ".json";

bool showForecast(const char *json)
{
    DynamicJsonBuffer jsonBuffer;

    // Skip characters until first '{' found
    // Ignore chunked length, if present
    const char *jsonstart = strchr(json, '{');
    //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
    if (jsonstart == NULL) {
        Serial.println(F("JSON data missing"));
        return false;
    }
    json = jsonstart;

    // Parse JSON
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()) {
        Serial.println(F("jsonBuffer.parseObject() failed"));
        return false;
    }

    // Extract weather info from parsed JSON
    JsonObject& forecast = root["forecast"]["txt_forecast"];
    const char *txt_forecast_date = forecast["date"];
    Serial.println(txt_forecast_date);
    const char *txt_forecast_forecastday_0_title = forecast["forecastday"][0]["title"];
    Serial.println(txt_forecast_forecastday_0_title);
    const char *txt_forecast_forecastday_0_fcttext = forecast["forecastday"][0]["fcttext"];
    Serial.println(txt_forecast_forecastday_0_fcttext);
    return true;
}

bool showCurrent(const char *json)
{
    DynamicJsonBuffer jsonBuffer;

    // Skip characters until first '{' found
    // Ignore chunked length, if present
    const char *jsonstart = strchr(json, '{');
    //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
    if (jsonstart == NULL) {
        Serial.println(F("JSON data missing"));
        return false;
    }
    json = jsonstart;

    // Parse JSON
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()) {
        Serial.println(F("jsonBuffer.parseObject() failed"));
        return false;
    }

    // Extract weather info from parsed JSON
    JsonObject& current = root["current_observation"];
    const float temp_f = current["temp_f"];
    Serial.print(temp_f, 1); Serial.print(F(" F, "));
    const float temp_c = current["temp_c"];
    Serial.print(temp_c, 1); Serial.print(F(" C, "));
    const char *humi = current[F("relative_humidity")];
    Serial.print(humi);   Serial.println(F(" RH"));
    const char *weather = current["weather"];
    Serial.println(weather);
    const char *pressure_mb = current["pressure_mb"];
    Serial.println(pressure_mb);
    const char *observation_time = current["observation_time_rfc822"];
    Serial.println(observation_time);
    return true;
}

typedef bool (*callback_t)(const char *);

void getWeather(const char *URL, callback_t cb)
{
    HTTPClient http;

    USE_SERIAL.print("[HTTP] begin...\n");
    http.begin(URL);

    USE_SERIAL.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            (*cb)(payload.c_str());
        }
    } else {
        USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

void loop() {
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {
        if (ForecastCheck) {
            getWeather(WUNDERGROUND_FORECAST_URL, showForecast);
            ForecastCheck = false;
        }
        if (CurrentCheck) {
            getWeather(WUNDERGROUND_CURRENT_URL, showCurrent);
            CurrentCheck = false;
        }
    }
}
