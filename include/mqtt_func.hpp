#pragma once
// ===========================================================
// Callback Funktion von MQTT. Die Funktion wird aufgerufen
// wenn ein Wert empfangen wurde.
// ===========================================================
void MQTT_callback(char *topic, byte *payload, unsigned int length) {
 
    if (millis() - mqtt_last_publish < 1000)
    {
        Serial.println("ignore payload because publish to same topic");
        return;
    }

    Serial.print("Received message [");
    Serial.print(topic);
    Serial.print("] ");

    // paranoia check to avoid npe if no payload
    if (payload==nullptr || length == 0) {
        Serial.println("no payload -> leave");
        return;
    }
    char msg[length + 1];
    for (uint32_t i = 0; i < length; i++) {
        msg[i] = (char)payload[i];
    }
    msg[length] = '\0';
    Serial.println(msg);

    size_t topicPrefixLen = strlen(G.MQTT_ClientId);
    if (strncmp(topic, G.MQTT_ClientId, topicPrefixLen) == 0) {
        topic += topicPrefixLen;
    } else {
        // Topic not used here. Probably a usermod subscribed to this topic.
        return;
    }
    //Prefix is stripped from the topic at this point


    //{"state": "on", "brightness": 255, "color": [255, 255, 255], "effect": "rainbow"}
    StaticJsonDocument<200> doc; //jsonBuffer
    DeserializationError error = deserializeJson(doc, msg);

      // Test if parsing succeeds.
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    if (doc.containsKey("brightness")) 
    {
        G.hell = doc["brightness"]; 
        sprintf(str, "|brightness:%d", G.hell);
        Serial.print(str);
    }

    strlcpy(str, doc["mode"] | "", 20);
    if (str[0] > 0) { 
        if (strstr("marquee", str))     G.prog = COMMAND_MODE_MARQUEE;      G.prog_init = 1;
        if (strstr("rainbow", str))     G.prog = COMMAND_MODE_RAINBOW;      G.prog_init = 1;
        if (strstr("change", str))      G.prog = COMMAND_MODE_CHANGE;       G.prog_init = 1;
        if (strstr("color", str))       G.prog = COMMAND_MODE_COLOR;        G.prog_init = 1;
        if (strstr("seconds", str))     G.prog = COMMAND_MODE_SECONDS;      G.prog_init = 1;
        if (strstr("clock", str))       G.prog = COMMAND_MODE_WORD_CLOCK;   parameters_changed = true;
               
        Serial.print("|mode:");
        Serial.print(G.prog);
    }        
       
    JsonArray array = doc["color"].as<JsonArray>();
    uint8_t i = 0;
    for(JsonVariant v : array) {
        G.rgb[Foreground][i]=v.as<uint8_t>();
        G.rgb[Effect][i]=G.rgb[Foreground][i];
        sprintf(str, "|color[%d]:%d", i, G.rgb[Foreground][i]);
        Serial.print(str);
        i++;
        if (i >= sizeof(G.rgb[Foreground])) break;
        parameters_changed = (G.prog == COMMAND_IDLE || COMMAND_MODE_WORD_CLOCK);
    }
    if (str[0]>0) {Serial.println("|");}
    
    return;
}

void MQTT_disconnect() {

    sprintf(str, "%s/cmnd", G.MQTT_Topic);
    mqttClient.unsubscribe(str);
    mqttClient.disconnect();
    Serial.println("MQTT diconnected");
}

bool MQTT_reconnect (const char *id, const char *user, const char *pass,const char* willTopic) {

    //if (!mqttClient.connected()) return false;

    if (millis() - mqtt_last_publish < 5000) return true;

    // Loop until we're reconnected
    //while (!mqttClient.connected() && mqtt_reconnect_retries < MQTT_MAX_RECONNECT_TRIES) {
    if (mqtt_reconnect_retries < MQTT_MAX_RECONNECT_TRIES) {
        mqtt_reconnect_retries++;
        Serial.printf("Attempting MQTT connection %d / %d ...", mqtt_reconnect_retries, MQTT_MAX_RECONNECT_TRIES);
        
        //MQTT_disconnect();
        mqtt_last_publish = millis();

        // Attempt to connect
        sprintf(str, "%s/status", willTopic);
        if (mqttClient.connect(id, user, pass, str, 0, true, "OFFLINE")) {
            //if (!mqttClient.connected()) {
                //delay(5000);
            //    return false;
            //}
            Serial.println("connected!");
             // Once connected, publish an announcement...
            sprintf(str, "%s/status", willTopic);
            mqttClient.publish(str,"ONLINE");
  
            // ... and resubscribe
            sprintf(str, "%s/cmnd", willTopic);
            mqttClient.subscribe(str);
            mqtt_reconnect_retries = 0;
            //return true;
        } else {

            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            
            if (mqtt_reconnect_retries == MQTT_MAX_RECONNECT_TRIES) {
                Serial.printf(", giving up after %d tries ...\n", mqtt_reconnect_retries);
             return false;
            }  

            Serial.println(" try again in 5 seconds");
            mqtt_last_publish = millis();
            // Wait 5 seconds before retrying
            //MQTT_disconnect();
            //delay(5000);
        }

    } 
    
   

    return true; 
}