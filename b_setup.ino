//instantiate wifi connect
bool init_wifi()
{
  String temp_pref_ssid = preferences.getString("pref_ssid"); //get String from flash memory
  String temp_pref_pass = preferences.getString("pref_pass");
  String temp_pref_topic = preferences.getString("pref_topic");
  
  pref_ssid = temp_pref_ssid.c_str(); //apply string onto const variable
  pref_pass = temp_pref_pass.c_str();
  pref_topic = temp_pref_topic.c_str();
  
  Serial.println(pref_ssid);
  Serial.println(pref_pass);
  Serial.println(pref_topic);

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);

  start_wifi_millis = millis();
  WiFi.begin(pref_ssid, pref_pass);
  while (WiFi.status() != WL_CONNECTED) { //begin wifi connection
    delay(500);
    Serial.print(",");
    if (millis() - start_wifi_millis > wifi_timeout) {
      WiFi.disconnect(true, true);
      return false;
    }
  }
  Serial.println("Connected to a Wi-Fi network");
  client.connect("Mr Dhason is a good friend", "", NULL);
  wifi_stage = CONNECTED;
  
  return true;
}

//scan availble network and display on Bluetooth Serial app
void scan_wifi_networks()
{
  WiFi.mode(WIFI_STA);
  int n =  WiFi.scanNetworks();
  if (n == 0) {
    SerialBT.println("no networks found");
  } else {
    SerialBT.println();
    SerialBT.print(n);
    SerialBT.println(" networks found");
    delay(1000);
    for (int i = 0; i < n; ++i) {
      ssids_array[i + 1] = WiFi.SSID(i);
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(ssids_array[i + 1]);
      network_string = i + 1;
      network_string = network_string + ": " + WiFi.SSID(i) + " (Strength:" + WiFi.RSSI(i) + ")";
      SerialBT.println(network_string); //display all available wifi connection on serial bluetooth app
    }
    wifi_stage = SCAN_COMPLETE;
  }
}

//callback function invoke from main page
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    wifi_stage = SCAN_START;
  }

  if (event == ESP_SPP_DATA_IND_EVT && wifi_stage == SCAN_COMPLETE) { // data from phone is SSID
    int client_wifi_ssid_id = SerialBT.readString().toInt();
    client_wifi_ssid = ssids_array[client_wifi_ssid_id];
    wifi_stage = SSID_ENTERED;
  }

  if (event == ESP_SPP_DATA_IND_EVT && wifi_stage == WAIT_PASS) { // data from phone is password
    client_wifi_password = SerialBT.readString();
    client_wifi_password.trim();
    wifi_stage = WAIT_TOPIC;
  }

  if (event == ESP_SPP_DATA_IND_EVT && wifi_stage == TOPIC_ENTERED) { // data from phone is topic
    client_topic = SerialBT.readString();
    client_topic.trim();
    wifi_stage = PASS_ENTERED;
  }

}

//disconenct bluetooth function
void disconnect_bluetooth()
{
  delay(1000);
  Serial.println("BT stopping");
  SerialBT.println("Bluetooth disconnecting...");
  delay(1000);
  SerialBT.flush();
  SerialBT.disconnect();
  SerialBT.end();
  Serial.println("BT stopped");
  delay(1000);
  bluetooth_disconnect = false;
}
