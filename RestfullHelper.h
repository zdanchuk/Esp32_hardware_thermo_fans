


void clearValues() {
  Serial.println(" !!!!! clearValues");
  cpu.temperature = 0;
  cpu.load = 0;
  cpu.power = 0;
  strcpy(cpu.hardwareName, emptyHardwareName);

  gpu.temperature = 0;
  gpu.load = 0;
  gpu.power = 0;
  strcpy(gpu.hardwareName, emptyHardwareName);

  ssd.temperature = 0;
  ssd.activity = 0;
  ssd.usedSpace = 0;
  strcpy(ssd.hardwareName, emptyHardwareName);

  memory.availableMemory = 0;
  memory.usedMemory = 0;
  memory.usedPercentage = 0;
  memory.allMemory = 0;
}

bool updateShortSernsorsInfo() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String serverPath = serverName + shortUrl;
    http.begin(serverPath.c_str());
    //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD"); // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<1536> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        clearValues();
        Serial.print(deserializeFailed);
        Serial.println(error.c_str());
        strcpy(cpu.hardwareName, emptyHardwareName);
        strcpy(gpu.hardwareName, emptyHardwareName);
        tft.drawString(cpu.hardwareName, 80, 3);
        tft.drawString(gpu.hardwareName, 80, 63);
        return false;
      }

      for (JsonObject item : doc.as<JsonArray>()) {
        const char* Hardware = item["Hardware"];  // "Cpu", "Gpu", "nvme"
        const char* Name = item["Name"];          // "Intel Core i7-8750H", "NVIDIA NVIDIA GeForce GTX 1070", "Samsung ...
        if (strcmp(Hardware, "Cpu") == 0) {
          if (strcmp(Name, cpu.hardwareName) != 0) {
            isNeedRedrawMainInfo = true;
            strcpy(cpu.hardwareName, Name);
          }
          for (JsonObject Sensor : item["Sensors"].as<JsonArray>()) {
            const char* Sensor_Sensor = Sensor["Sensor"];
            if (strcmp(Sensor_Sensor, "Temperature") == 0) { cpu.temperature = Sensor["Value"]; }
            if (strcmp(Sensor_Sensor, "Load") == 0) { cpu.load = Sensor["Value"]; }
            if (strcmp(Sensor_Sensor, "Power") == 0) { cpu.power = Sensor["Value"]; }
          }
        } else if (strcmp(Hardware, "Gpu") == 0) {
          if (strcmp(Name, gpu.hardwareName) != 0) {
            isNeedRedrawMainInfo = true;
            strcpy(gpu.hardwareName, Name);
          }
          for (JsonObject Sensor : item["Sensors"].as<JsonArray>()) {
            const char* Sensor_Sensor = Sensor["Sensor"];
            if (strcmp(Sensor_Sensor, "Temperature") == 0) { gpu.temperature = Sensor["Value"]; }
            if (strcmp(Sensor_Sensor, "Load") == 0) { gpu.load = Sensor["Value"]; }
            if (strcmp(Sensor_Sensor, "Power") == 0) { gpu.power = Sensor["Value"]; }
          }
        } else if (strcmp(Hardware, "NVMe") == 0) {
          if (strcmp(Name, ssd.hardwareName) != 0) {
            isNeedRedrawMainInfo = true;
            strcpy(ssd.hardwareName, Name);
          }
          for (JsonObject Sensor : item["Sensors"].as<JsonArray>()) {
            const char* Sensor_Sensor = Sensor["Sensor"];
            const char* Sensor_Name = Sensor["Name"];
            if (strcmp(Sensor_Name, "Temperature") == 0) { ssd.temperature = Sensor["Value"]; }
            if (strcmp(Sensor_Name, "Total Activity") == 0) { ssd.activity = Sensor["Value"]; }
            if (strcmp(Sensor_Name, "Used Space") == 0) { ssd.usedSpace = Sensor["Value"]; }
          }
        } else if (strcmp(Hardware, "Memory") == 0) {
          for (JsonObject Sensor : item["Sensors"].as<JsonArray>()) {
            const char* Sensor_Name = Sensor["Name"];
            if (strcmp(Sensor_Name, "Memory") == 0) { memory.usedPercentage = Sensor["Value"]; }
            if (strcmp(Sensor_Name, "Memory Used") == 0) { memory.usedMemory = Sensor["Value"]; }
            if (strcmp(Sensor_Name, "Memory Available") == 0) { memory.availableMemory = Sensor["Value"]; }
            memory.allMemory = memory.usedMemory + memory.availableMemory;
          }
        }
      }

      // Serial.println("CPU:" + String(cpu.temperature) + ";GPU:" + String(gpu.temperature) + ";SSD:" + String(ssd.temperature));
      StaticJsonDocument<100> doc2;
      doc2["cpu"] = int(cpu.temperature);
      doc2["gpu"] = int(gpu.temperature);
      doc2["ssd"] = int(ssd.temperature);
      doc2["speed"] = setFanSpeed;
      // serializeJson(doc2, Serial2);
    }
    // Free resources
    http.end();
    return true;
  }
  return false;
}

bool updateMicStateInfo() {
  bool methodResult = false;
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      String serverPath = serverName + getMicState;
      http.begin(serverPath.c_str());
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD"); // Send HTTP GET request
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        String payload = http.getString();
        StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          tft.print("deserializeJson() failed: ");
          tft.println(error.c_str());
          return false;
        }

        int micStateTemp = doc["State"].as<int>();
        bool result = doc["Result"].as<bool>();
        bool withErrors = doc["WithErrors"].as<bool>();
        // Serial.println(" !!!!!!!! mic state changed to " + String(micStateTemp) + " Result: " + String(result) + " WithErrors: " + String(withErrors));
        if (result == true && withErrors == false) {
          if (micState != micStateTemp) {
            isEnabled = true;
            loopTime = millis();
            Serial.println(" !!!!!!!! mic state changed to " + String(micStateTemp));
          }
          micState = micStateTemp;
        } else {
          micState = 0;
        }
        methodResult = true;
      }
      // Free resources
      http.end();
    }
    if (methodResult == true) {
      redrawMicLogo();
    }
  return methodResult;
}

bool setNewMicState2(int newState) {
  bool result = false;
  if (WiFi.status() == WL_CONNECTED) {
    // WiFiClient client;
    HTTPClient http;

    String serverPath = serverName + setMicState + "/?state=" + String(newState);
    http.begin(serverPath.c_str());
    //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD"); // Send HTTP GET request
    // Specify content-type header
    // String httpRequestData = "state=" + String(newState);
    http.addHeader("Content-Type", "none", false, true);
    // http.addHeader("Content-Length", String(httpRequestData.length()), false, true);
    http.addHeader("Host", serverName, false, true);
    http.addHeader("User-Agent", "ESP32", false, true);
    http.addHeader("Connection", "keep-alive", false, true);
    // // Data to send with HTTP POST
    // // Send HTTP POST request
    // Serial.println("link : " + serverPath);
    // for (int i = 0; i < http.headers(); i++) {
    //   Serial.println(String(http.header(i)));
    // }
    int httpResponseCode = http.GET();//(httpRequestData);

    // Serial.println("Headers 2: " + String(http.headers()));
    // for (int i = 0; i < http.headers(); i++) {
    //   Serial.println(String(http.header(i)));
    // }
    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println(String(httpResponseCode) + ": Sended post ruquest to change mic state. Result: " + payload);
      // Serial.println(String(http));
      result = (httpResponseCode >= 200 && httpResponseCode <= 299);
      if (result == true) {
        String payload = http.getString();
        Serial.println(payload);
        StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          // clearValues();
          result = false;
        } else {

          int micStateTemp = doc["State"].as<int>();
          result = result && doc["Result"].as<bool>() && !doc["WithErrors"].as<bool>();
        }
      }
    }
    // Free resources
    http.end();
  }
  return result;
}




bool setNewMicState(int newState){
  bool methodResult = false;
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

    String serverPath = serverName + setMicState + String(newState);
    Serial.println("Send request to " + serverPath);
      http.begin(serverPath.c_str());
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD"); // Send HTTP GET request
      int httpResponseCode = http.GET();
    Serial.println("httpResponseCode " + String(httpResponseCode));

      if (httpResponseCode > 0) {
        String payload = http.getString();
        StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          tft.print("deserializeJson() failed: ");
          tft.println(error.c_str());
          return false;
        }

        int micStateTemp = doc["State"].as<int>();
        bool result = doc["Result"].as<bool>();
        bool withErrors = doc["WithErrors"].as<bool>();
        // Serial.println(" !!!!!!!! mic state changed to " + String(micStateTemp) + " Result: " + String(result) + " WithErrors: " + String(withErrors));
        if (result == true && withErrors == false) {
          if (micState != micStateTemp) {
            isEnabled = true;
            loopTime = millis();
            Serial.println(" !!!!!!!! mic state changed to " + String(micStateTemp));
          }
          micState = micStateTemp;
        } else {
          micState = 0;
        }
        methodResult = true;
      }
      // Free resources
      http.end();
    }
    if (methodResult == true) {
      redrawMicLogo();
    }
  return methodResult;
}