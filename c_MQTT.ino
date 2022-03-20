//remain MQTT connection
void reconnect() 
{
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      init_wifi();
    }
    Serial.print("Connecting to Broker …");
    
    if ( client.connect("Mr Dhason is a good friend", "", NULL) )
    {
      Serial.println("[DONE]" );
    }
    else {
      Serial.println( " : retrying in 5 seconds]" );
      delay( 5000 );
    }
  }
}

//subscribe to topic given by user
bool subTopic(bool isTopicSubbed) {
  if(!isTopicSubbed) {
    
  String temp_pref_topic = preferences.getString("pref_topic");
  pref_topic = temp_pref_topic.c_str();
    client.subscribe(pref_topic);
    Serial.print("subscribed to ");
    Serial.println(pref_topic);
    isTopicSubbed = true;
    return isTopicSubbed;
  }
}
