/*
 * Earthquake Detection and Twitter Alert System
 * Author: Michael Barcelo
 *
 * Description:
 * This program runs on an ESP32 microcontroller and uses an MPU6050 accelerometer and gyroscope 
 * to detect potential earthquakes by monitoring acceleration thresholds. When an earthquake 
 * is detected, it triggers an alert by posting a tweet to a Twitter account.
 * 
*/

#include "I2Cdev.h"
#include "MPU6050.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <mbedtls/md.h>
#include <UrlEncode.h>
#include <HTTPClient.h>

MPU6050 mpu;

const char* network_ssid = "XXXXXXXXXX";
const char* network_password = "XXXXXXXXXX";

// Define OAuth Credentials
const char* apiKey = "XXXXXXXXXX";
const char* apiSecretKey = "XXXXXXXXXX";
const char* accessToken = "XXXXXXXXXX";
const char* accessTokenSecret = "XXXXXXXXXX";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // sync every 60 seconds

#define HMAC_SHA1_SIZE 20 // HMAC-SHA1 produces a 20-byte digest

// Define earthquake detection thresholds
const int16_t earthquakeAccelThreshold = 16000; // Acceleration threshold in mg (can be tuned)
const unsigned long detectionDuration = 250;  // Duration in ms over which threshold must be exceeded
unsigned long quakeStart = 0;
bool quakeDetected = false;

int16_t ax, ay, az;
int16_t gx, gy, gz;

void calculateHMACSHA1(const char* key, const char* data, uint8_t* hmacResult) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1); // 1 indicates using HMAC
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key, strlen(key));
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)data, strlen(data));
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
}

String base64Encode(const uint8_t* data, size_t length) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String encoded = "";
    
    for (size_t i = 0; i < length; i += 3) {
        int value = (data[i] << 16) + ((i + 1 < length ? data[i + 1] : 0) << 8) + (i + 2 < length ? data[i + 2] : 0);
        encoded += base64_chars[(value >> 18) & 0x3F];
        encoded += base64_chars[(value >> 12) & 0x3F];
        encoded += (i + 1 < length) ? base64_chars[(value >> 6) & 0x3F] : '=';
        encoded += (i + 2 < length) ? base64_chars[value & 0x3F] : '=';
    }
    return encoded;
}

String generateNonce(int length = 32) {
    const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    String nonce = "";
    for (int i = 0; i < length; i++) {
        nonce += characters[random(0, sizeof(characters) - 1)];
    }
    return nonce;
}

String calculateOAuthSignature(const char* apiKey, const char* apiSecretKey, const char* accessToken, const char* accessTokenSecret, unsigned long epochTime, String nonce) {
    Serial.println("---- Calculating OAuth Signature ----");

    String signature_base_string = "POST&";
    signature_base_string += urlEncode("https://api.twitter.com/2/tweets") + "&";

    String epochTimeStr = String(epochTime);
    String parameters[] = {
        "oauth_consumer_key", apiKey,
        "oauth_nonce", nonce.c_str(),
        "oauth_signature_method", "HMAC-SHA1",
        "oauth_timestamp", epochTimeStr.c_str(),
        "oauth_token", accessToken,
        "oauth_version", "1.0"
    };

    int numParams = sizeof(parameters) / sizeof(parameters[0]);
    String temp = "";
    for (int i = 0; i < numParams; i += 2) {
        String encoded_key = urlEncode(parameters[i]);
        String encoded_value = urlEncode(parameters[i + 1]);
        temp += encoded_key + "=" + encoded_value;
        if (i < numParams - 2) {
            temp += "&";
        }
    }

    signature_base_string += urlEncode(temp);

    Serial.println("Signature Base String: " + signature_base_string);

    String signing_key = urlEncode(apiSecretKey) + "&" + urlEncode(accessTokenSecret);
    uint8_t hmacResult[HMAC_SHA1_SIZE];
    calculateHMACSHA1(signing_key.c_str(), signature_base_string.c_str(), hmacResult);
    String encoded_signature = base64Encode(hmacResult, HMAC_SHA1_SIZE);

    Serial.println("Signing Key: " + signing_key);
    Serial.println("Encoded Signature: " + encoded_signature);
    
    return encoded_signature;
}

void sendTweet(const char* tweet) {
    String url = "https://api.twitter.com/2/tweets";
    HTTPClient http;

    String nonce = generateNonce();
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    String oauthSignature = calculateOAuthSignature(apiKey, apiSecretKey, accessToken, accessTokenSecret, epochTime, nonce);

    String authHeader = "OAuth ";
    authHeader += "oauth_consumer_key=\"" + String(urlEncode(apiKey)) + "\",";
    authHeader += "oauth_nonce=\"" + String(urlEncode(nonce)) + "\",";
    authHeader += "oauth_signature=\"" + String(urlEncode(oauthSignature)) + "\",";
    authHeader += "oauth_signature_method=\"HMAC-SHA1\",";
    authHeader += "oauth_timestamp=\"" + String(urlEncode(String(epochTime))) + "\",";
    authHeader += "oauth_token=\"" + String(urlEncode(accessToken)) + "\",";
    authHeader += "oauth_version=\"1.0\"";

    Serial.println("---- Sending HTTP POST Request ----");
    Serial.println("Authorization Header: " + authHeader);
    
    String payload = String("{\"text\":\"") + String(tweet) + "\"}";
    int contentLength = payload.length();
    
    Serial.println("Payload: " + payload);
    Serial.println("Content-Length: " + String(contentLength));
    
    http.begin(url);
    http.addHeader("Authorization", authHeader);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", String(contentLength));
    http.addHeader("Host", "api.twitter.com");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Response Code: " + String(httpResponseCode));
        Serial.println("Response: " + response);
    } else {
        Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();
}

void connectToWiFi(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("\nAttempting to connect to " + String(ssid) + " via WiFi...");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}

void setup() {
  // Initialize I2C interface
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
  #endif

  Serial.begin(38400);
  while (!Serial);
    delay(500);

  // Initialize MPU6050 and check connection
  Serial.println("Initializing MPU...");
  mpu.initialize();
  Serial.println("Testing MPU6050 connection...");
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (true);
  } else {
    Serial.println("MPU6050 connection successful");
  }

  // Set offsets (if needed, based on calibration)
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  
  connectToWiFi(network_ssid, network_password);
  timeClient.begin();
  randomSeed(analogRead(0)); // Seed the random number generator
}

void loop() {
    // Read accelerometer and gyroscope data
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Calculate the combined acceleration magnitude (ignoring gravity for shock detection)
    int16_t accelMagnitude = sqrt(ax * ax + ay * ay + az * az);

    // Check if the magnitude exceeds the threshold
    if (accelMagnitude > earthquakeAccelThreshold) {
        Serial.println("accelMagnitude is greater than threshold!");

        if (!quakeDetected) {
            // Begin timing for earthquake detection
            quakeStart = millis();
            quakeDetected = true;
            Serial.println("Potential earthquake detected! Monitoring...");
        } else {
            // Confirm earthquake if threshold is exceeded for the specified duration
            if ((millis() - quakeStart) > detectionDuration) {
                Serial.println("Earthquake confirmed!");
                unsigned long eventID = timeClient.getEpochTime();
                String tweet = "TEST: Earthquake detected! Stay safe! Event ID: " + String(eventID); 
                sendTweet(tweet.c_str());
                quakeDetected = false; // Reset after confirming earthquake
            }
        }
    } 

    // Reset detection if below threshold and duration has passed
    if (quakeDetected && accelMagnitude <= earthquakeAccelThreshold && (millis() - quakeStart) > detectionDuration) {
        Serial.println("No earthquake detected.");
        quakeDetected = false;
    }

    // Output readings for debugging
    Serial.print("Accel (X, Y, Z): ");
    Serial.print(ax); Serial.print(" ");
    Serial.print(ay); Serial.print(" ");
    Serial.print(az); Serial.print(" ");
    Serial.print(" Magnitude: ");
    Serial.println(accelMagnitude);

    delay(50); // Sampling delay
}
