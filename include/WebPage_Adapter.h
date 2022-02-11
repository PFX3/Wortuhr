#pragma once

#include "Network.h"
#include "Uhr.h"
#include "WebPageContent.h"
#include "WebSocketsServer.h"

#define RESPONSE_SIZE 900
#define SIZE_OF_FAVICON 197

const char favicon[] PROGMEM = {
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00,
    0x18, 0x00, 0xae, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x89, 0x50,
    0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
    0x44, 0x52, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x08, 0x06,
    0x00, 0x00, 0x00, 0x1f, 0xf3, 0xff, 0x61, 0x00, 0x00, 0x00, 0x75, 0x49,
    0x44, 0x41, 0x54, 0x38, 0x8d, 0xcd, 0x91, 0xbd, 0x0a, 0x80, 0x40, 0x0c,
    0x83, 0xbf, 0xaa, 0xb3, 0x83, 0xb8, 0xf9, 0xe2, 0x3e, 0x9d, 0x93, 0x93,
    0x83, 0x83, 0x20, 0x38, 0x78, 0xc6, 0x41, 0x45, 0xe1, 0xf0, 0xf7, 0x16,
    0x03, 0xa5, 0x94, 0x36, 0x21, 0xa4, 0xb0, 0x43, 0x4d, 0x8d, 0xc6, 0x1e,
    0x69, 0x40, 0x70, 0x5b, 0x3b, 0x2a, 0xcb, 0x05, 0x0b, 0x71, 0xab, 0x07,
    0x02, 0x02, 0x30, 0x4f, 0xed, 0x25, 0xa2, 0x10, 0x32, 0x40, 0x72, 0xb9,
    0x55, 0x71, 0xcd, 0x76, 0x0a, 0x74, 0x10, 0x9b, 0x9f, 0x81, 0xd2, 0xd6,
    0xbb, 0xb3, 0x2e, 0x3b, 0xd5, 0x08, 0xce, 0xe0, 0xef, 0x5f, 0xd8, 0x50,
    0x9e, 0x9b, 0x0c, 0x73, 0x30, 0xb9, 0x1f, 0x64, 0x60, 0x6b, 0xff, 0xea,
    0xc2, 0xec, 0x30, 0xbc, 0x15, 0x31, 0x80, 0x19, 0x09, 0x86, 0x36, 0x37,
    0x8e, 0x9c, 0xde, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
    0xae, 0x42, 0x60, 0x82, 0x0a};

class WebPage_Adapter : public WebSocketsServer {

public:
    // forward the port to the parent class constructor
    WebPage_Adapter(int port) : WebSocketsServer(port) {}

    /**
     * @param client WSclient_t *  ptr to the client struct
     */
    void handleNonWebsocketConnection(WSclient_t *client) override {
        DEBUG_WEBSOCKETS(
            "[WS-Server][%d][handleHeader] no Websocket connection close.\n",
            client->num);
        char buf[200];
        int index = client->cUrl.indexOf('?');
        int length = index >= 0 ? index : client->cUrl.length();
        // remove GET request data from URL
        String url = client->cUrl.substring(0, length);
        // ----------------------------------------
        if (url.endsWith("favicon.ico")) {
            sprintf(buf,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: image/x-ico\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n"
                    "\r\n",
                    SIZE_OF_FAVICON);
            client->tcp->write(buf);
            for (int i = 0; i < SIZE_OF_FAVICON; i++) {
                buf[i] = pgm_read_byte(&favicon[i]);
            }
            client->tcp->write(buf, SIZE_OF_FAVICON);

        } else {
            // ------------------------------------
            if (url.equals("/")) {
                client->tcp->write(
                    "HTTP/1.1 200 OK\r\n"
                    "Server: arduino-WebSocket-Server\r\n"
                    "Content-Type: text/html\r\n"
                    //--                    "Content-Length: 32\r\n"
                    "Connection: close\r\n"
                    //--                    "Sec-WebSocket-Version: 13\r\n"
                    "\r\n");
                Send_HTML_Code(client, html_code, html_size);
            } else {

                // --------------------------------
                int len = sprintf(buf, "HTTP/1.1 404 Not Found\r\n"
                                       "Content-Type: text/plain\r\n\r\n"
                                       "Seite ");
                client->cUrl.toCharArray(buf + len, sizeof(buf) - len);
                strcat(buf, " nicht gefunden\n");
                client->tcp->write(buf);
            }
        }

        clientDisconnect(client);
    }

    void Send_HTML_Code(const WSclient_t *client, const char *data,
                        uint32_t size) const {
        char buf[RESPONSE_SIZE];
        unsigned sent = 0;
        unsigned blen = 0;
        while (sent < size) {
            buf[blen] = pgm_read_byte(&data[sent]);
            blen++;
            if (blen == RESPONSE_SIZE) {
                client->tcp->write(buf, blen);
                blen = 0;
            }
            sent++;
        }
        if (blen > 0) {
            client->tcp->write(buf, blen);
        }
    }
};

//-- WebSocketserver
WebPage_Adapter webSocket = WebPage_Adapter(80);

//------------------------------------------------------------------------------

uint16_t split(uint8_t *payload, uint8_t start, uint8_t length = 3) {
    char buffer[length];
    uint8_t m = 0;
    for (uint16_t k = start; k < (start + length); k++) {
        buffer[m] = payload[k];
        m++;
    }
    return atoi(buffer);
}

//------------------------------------------------------------------------------

void payloadTextHandling(const uint8_t *payload, char *text, uint8_t length,
                         uint8_t start = 3) {
    uint8_t ii = 0;
    for (uint8_t k = start; k < start + length - 1; k++) // need space for  '\0'
    {
        text[ii] = payload[k];
        ii++;
    }
    uint8_t index = length - 1;
    for (int8_t counter = length - 2; counter > -1; counter--) {
        if (!isSpace(text[counter])) {
            index = counter + 1;
            break;
        }
    }
    text[index] = '\0';
}

//------------------------------------------------------------------------------

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
    // Disable Accesspoint Mode Disable Timer on Web Event
    if (AP_Status > 0) {
        AP_Status = 0;
    }
    int ii;
    int jj;
    char tmp[30];
    payload = (payload == NULL) ? (uint8_t *)"" : payload;
    Serial.printf("Client-Nr.: [%u]  WStype: %u payload: %s\n", num, type,
                  payload);

    switch (type) {
    case WStype_DISCONNECTED: {
        Serial.printf("[%u] Disconnected!\n", num);
        break;
    }
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                      ip[1], ip[2], ip[3], payload);
        break;
    }
    case WStype_TEXT: {
        Serial.printf("[%u] get Text: %s\n", length, payload);

        uint16_t command = split(payload, 0);
        G.param1 = 0;

        switch (command) {
        case COMMAND_MODE_WORD_CLOCK: { // Uhrzeit Vordergrund Farbe einstellen
            G.prog = COMMAND_MODE_WORD_CLOCK;
            parameters_changed = true;
            G.rgb[Foreground][0] = split(payload, 3);
            G.rgb[Foreground][1] = split(payload, 6);
            G.rgb[Foreground][2] = split(payload, 9);
            G.rgb[Foreground][3] = split(payload, 12);

            G.rgb[Background][0] = split(payload, 15);
            G.rgb[Background][1] = split(payload, 18);
            G.rgb[Background][2] = split(payload, 21);
            G.rgb[Background][3] = split(payload, 24);

            G.rgb[Frame][0] = split(payload, 3);
            G.rgb[Frame][1] = split(payload, 6);
            G.rgb[Frame][2] = split(payload, 9);
            G.rgb[Frame][3] = split(payload, 12);

            G.rgb[Effect][0] = split(payload, 3);
            G.rgb[Effect][1] = split(payload, 6);
            G.rgb[Effect][2] = split(payload, 9);
            G.rgb[Effect][3] = split(payload, 12);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_MODE_SECONDS: { // Sekunden
            G.prog = COMMAND_MODE_SECONDS;
            G.param1 = split(payload, 33);
            if (G.param1 == 0) {
                G.prog_init = 1;
            }

            G.rgb[Effect][0] = split(payload, 3);
            G.rgb[Effect][1] = split(payload, 6);
            G.rgb[Effect][2] = split(payload, 9);
            G.rgb[Effect][3] = split(payload, 12);
            G.hell = split(payload, 27);
            G.geschw = split(payload, 30);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_MODE_MARQUEE: { // Laufschrift
            G.prog = COMMAND_MODE_MARQUEE;
            G.param1 = split(payload, 33);
            if (G.param1 == 0) {
                G.prog_init = 1;
            }

            G.rgb[Effect][0] = split(payload, 3);
            G.rgb[Effect][1] = split(payload, 6);
            G.rgb[Effect][2] = split(payload, 9);
            G.rgb[Effect][3] = split(payload, 12);
            G.hell = split(payload, 27);
            G.geschw = split(payload, 30);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_MODE_RAINBOW: { // Regenbogen
            G.prog = COMMAND_MODE_RAINBOW;
            G.prog_init = 1;

            G.hell = split(payload, 27);
            G.geschw = split(payload, 30);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_MODE_CHANGE: { // Farbwechsel
            G.prog = COMMAND_MODE_CHANGE;
            G.prog_init = 1;

            G.hell = split(payload, 27);
            G.geschw = split(payload, 30);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_MODE_COLOR: { // Farbe
            G.prog = COMMAND_MODE_COLOR;
            G.prog_init = 1;

            G.rgb[Effect][0] = split(payload, 3);
            G.rgb[Effect][1] = split(payload, 6);
            G.rgb[Effect][2] = split(payload, 9);
            G.rgb[Effect][3] = split(payload, 12);
            break;
        }
            //------------------------------------------------------------------------------

        case COMMAND_MODE_ANIMATION: { // Animation
            G.prog = COMMAND_MODE_ANIMATION;
            G.prog_init = 1;
            G.animType = split(payload, 3);
            G.animDuration = split(payload, 6);
            G.animSpeed = split(payload, 9);
            G.animColorize = split(payload, 12);
            G.animDemo = split(payload, 15);
            //					Serial.printf("animType %d
            // animDuration %d animSpeed %d animColorize %d animDemo %d\n",
            // G.animType, G.animDuration, G.animSpeed, G.animColorize,
            // G.animDemo);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_BRIGHTNESS: { // Helligkeit
            G.hell = split(payload, 27);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SPEED: { // Geschwindigkeit
            G.geschw = split(payload, 30);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_INITIAL_VALUES: { // Startwerte speichern
            Serial.println("Startwerte gespeichert");
            Serial.println(G.rgb[Foreground][0]);
            Serial.println(G.rgb[Foreground][1]);
            Serial.println(G.rgb[Foreground][2]);
            Serial.println(G.rgb[Foreground][3]);
            G.conf = COMMAND_SET_INITIAL_VALUES;
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_TIME: { // Uhrzeit setzen
            G.conf = COMMAND_SET_TIME;
            ii = 0;
            tmp[0] = '\0';
            uint32_t tt = split(payload, 3, 16);
            Serial.println(tt);
            for (uint8_t k = 12; k < 28; k++) {
                tmp[ii] = payload[k];
                ii++;
            }
            struct timeval tv;
            tv.tv_sec = atoi(tmp);
            tv.tv_usec = 0;
            Serial.printf("Conf: Time: %ld\n", tv.tv_sec);
            settimeofday(&tv, nullptr);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_HOSTNAME: { // Hostname speichern
            G.conf = COMMAND_SET_HOSTNAME;
            payloadTextHandling(payload, G.hostname, 
                                sizeof(G.hostname) / sizeof(G.hostname[0]));
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_SETTING_SECOND: { // Anzeige Sekunden speichern
            G.conf = COMMAND_SET_SETTING_SECOND;
            G.zeige_sek = split(payload, 3);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_MINUTE: { // Anzeige Minuten speichern
            G.conf = COMMAND_SET_MINUTE;
            G.zeige_min = split(payload, 3);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_LDR: { // LDR speichern
            G.conf = COMMAND_SET_LDR;
            G.ldr = split(payload, 3);
            G.ldrCal = split(payload, 6);
            Serial.printf("LDR : %u\n\n", G.ldr);
            Serial.printf("LDR Kalibrierung: %u\n\n", G.ldrCal);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_AUTO_LDR: { // Automatische Helligkeit
            G.conf = COMMAND_SET_AUTO_LDR;
            G.autoLdrEnabled = split(payload, 3);
            G.autoLdrBright = split(payload, 6);
            G.autoLdrDark = split(payload, 9);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_LANGUAGE_VARIANT: { // LDR speichern
            G.conf = COMMAND_SET_LANGUAGE_VARIANT;
            G.Sprachvariation[ItIs15] = split(payload, 3);
            G.Sprachvariation[ItIs20] = split(payload, 6);
            G.Sprachvariation[ItIs40] = split(payload, 9);
            G.Sprachvariation[ItIs45] = split(payload, 12);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_MQTT: { // MQTT Daten speichern
            G.conf = COMMAND_SET_MQTT;
            G.MQTT_State = split(payload, 3);
            G.MQTT_Port = split(payload, 6, 5);

            strlcpy(str, G.MQTT_Server, sizeof(G.MQTT_Server) / sizeof(G.MQTT_Server[0]));
            payloadTextHandling(
                payload,G.MQTT_Server,
                sizeof(G.MQTT_Server) / sizeof(G.MQTT_Server[0]), 11);
 
            if (strcmp(str, G.MQTT_Server) != 0) G.prog_init = 1; //Reset

            payloadTextHandling(
                payload, G.MQTT_User,
                sizeof(G.MQTT_User) / sizeof(G.MQTT_User[0]), 41);
            payloadTextHandling(
                payload, G.MQTT_Pass,
                sizeof(G.MQTT_Pass) / sizeof(G.MQTT_Pass[0]), 71);
            payloadTextHandling(
                payload, G.MQTT_ClientId,
                sizeof(G.MQTT_ClientId) / sizeof(G.MQTT_ClientId[0]), 101);
            payloadTextHandling(
                payload, G.MQTT_Topic,
                sizeof(G.MQTT_Topic) / sizeof(G.MQTT_Topic[0]), 131);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_TIME_MANUAL: { // Uhrzeit manuell setzen
            G.conf = COMMAND_SET_TIME_MANUAL;
            time_t old = time(nullptr);
            struct tm tm;
            localtime_r(&old, &tm);
            tm.tm_hour = split(payload, 3);
            tm.tm_min = split(payload, 6);
            tm.tm_sec = 0;
            struct timeval tv;
            tv.tv_sec = mktime(&tm);
            tv.tv_usec = 0;
            Serial.println("Uhrzeit manuell eingstellt");
            Serial.printf("Conf: Time: %ld\n", tv.tv_sec);
            settimeofday(&tv, nullptr);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_COLORTYPE: { // ColorType speichern
            G.conf = COMMAND_SET_COLORTYPE;
            G.param1 = split(payload, 3);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_UHRTYPE: { // UhrType speichern
            G.conf = COMMAND_SET_UHRTYPE;
            G.UhrtypeDef = split(payload, 3);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_WEATHER_DATA: { // Openweathermap speichern
            G.conf = COMMAND_SET_WEATHER_DATA;
            ii = 0;
            for (uint8_t k = 3; k < 10; k++) {
                if (payload[k] != ' ') {
                    G.cityid[ii] = payload[k];
                    ii++;
                }
            }
            G.cityid[ii] = '\0';
            //
            jj = 0;
            for (uint8_t l = 11; l < 43; l++) {
                if (payload[l] != ' ') {
                    G.apikey[jj] = payload[l];
                    jj++;
                }
            }
            G.apikey[jj] = '\0';
            Serial.println("write EEPROM!");
            Serial.print("CityID : ");
            Serial.println(G.cityid);
            Serial.print("APIkey : ");
            Serial.println(G.apikey);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_BRIGHTNESS: { // Helligkeit speichern
            G.conf = COMMAND_SET_BRIGHTNESS;
            G.h6 = split(payload, 3);
            G.h8 = split(payload, 6);
            G.h12 = split(payload, 9);
            G.h16 = split(payload, 12);
            G.h18 = split(payload, 15);
            G.h20 = split(payload, 18);
            G.h22 = split(payload, 21);
            G.h24 = split(payload, 24);
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_MARQUEE_TEXT: { // Lauftext speichern
            G.conf = COMMAND_SET_MARQUEE_TEXT;
            payloadTextHandling(payload, G.ltext,
                                sizeof(G.ltext) / sizeof(G.ltext[0]));
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_TIMESERVER: { // Zeitserver speichern
            G.conf = COMMAND_SET_TIMESERVER;
            payloadTextHandling(payload, G.zeitserver,
                                sizeof(G.zeitserver) / sizeof(G.zeitserver[0]));
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_SET_BOOT: { // Bootoptionen speichern
            G.conf = COMMAND_SET_BOOT;
            G.bootLedBlink = split(payload, 3);
            G.bootLedSweep = split(payload, 6);
            G.bootShowWifi = split(payload, 9);
            G.bootShowIP = split(payload, 12);
            break;
        }
            //------------------------------------------------------------------------------

        case COMMAND_SET_WIFI_DISABLED:    // WLAN ausschalten
        case COMMAND_SET_WIFI_AND_RESTART: // WLAN-Daten löschen und neu starten
        case COMMAND_SET_WPS_MODE:         // Aktivieren des WPS Modus
        case COMMAND_RESET: {              // Reset
            G.conf = command;
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_REQUEST_MQTT_VALUES:   // MQTT Config anfordern
        case COMMAND_REQUEST_CONFIG_VALUES: // Config anfordern
        case COMMAND_REQUEST_COLOR_VALUES:  // Farbwerte anfordern
        case COMMAND_REQUEST_WIFI_LIST:     // Wlan Liste anfordern
        case COMMAND_REQUEST_ANIMATION: {   // Werte fuer Animation anfordern
            G.conf = command;
            G.client_nr = num;
            break;
        }

            //------------------------------------------------------------------------------

        case COMMAND_REQUEST_AUTO_LDR: { // Werte fuer Auto Helligkeit anfordern
            G.param1 = split(payload, 3);
            G.conf = command;
            G.client_nr = num;
            break;
        }

            //------------------------------------------------------------------------------

        default:
            break;
        }
        break;
    }
    case WStype_BIN: {
        Serial.printf("[%u] get binary length: %u\n", num, length);
        hexdump(payload, length);
        //--echo data back to browser
        // webSocket.sendBIN(num, payload, length);
        break;
    }
    default:
        break;
    }
}
