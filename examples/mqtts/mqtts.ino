/**
 * @file mqtts.ino
 * @author Arnoud Devoogdt <arnoud@dptechnics.com>
 * @date 9 September 2025
 * @copyright DPTechnics bv
 * @brief Walter Modem library examples
 *
 * @section LICENSE
 *
 * Copyright (C) 2025, DPTechnics bv
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of DPTechnics bv nor the names of its contributors may
 *      be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *   4. This software, with or without modification, must only be used with a
 *      Walter board from DPTechnics bv.
 *
 *   5. Any software provided in binary form under this license must not be
 *      reverse engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY DPTECHNICS BV “AS IS” AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL DPTECHNICS BV OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 *
 * This file contains a sketch which uses the modem in Walter to subscribe and
 * publish data to an MQTT broker using TLS (MQTTS).
 */

#include <HardwareSerial.h>
#include <WalterModem.h>
#include <pgmspace.h>
#include <esp_mac.h>

#define MQTTS_PORT 8883
#define MQTTS_HOST "broker.emqx.io"
#define MQTTS_TOPIC "walter-tls-test-topic"
#define MQTTS_CLIENT_ID "walter-client"
#define MQTTS_USERNAME ""
#define MQTTS_PASSWORD ""

/**
 * @brief Root CA certificate in PEM format.
 *
 * @note Example: https://www.emqx.com/en/mqtt/public-mqtt5-broker
 *
 * Used to validate the server's TLS certificate.
 */
const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

/**
 * The TLS profile to use for the application (1 is reserved for BlueCherry)
 */
#define MQTTS_TLS_PROFILE 2

/**
 * @brief The modem instance.
 */
WalterModem modem;

/**
 * @brief Response object containing command response information.
 */
WalterModemRsp rsp;

/**
 * @brief Buffer for incoming response
 */
uint8_t incomingBuf[256] = { 0 };

/**
 * @brief MQTT client and message prefix based on mac address
 */
char macString[32];

/**
 * @brief This function checks if we are connected to the LTE network
 *
 * @return true when connected, false otherwise
 */
bool lteConnected()
{
  WalterModemNetworkRegState regState = modem.getNetworkRegState();
  return (regState == WALTER_MODEM_NETWORK_REG_REGISTERED_HOME ||
          regState == WALTER_MODEM_NETWORK_REG_REGISTERED_ROAMING);
}

/**
 * @brief This function waits for the modem to be connected to the LTE network.
 *
 * @param timeout_sec The amount of seconds to wait before returning a time-out.
 *
 * @return true if connected, false on time-out.
 */
bool waitForNetwork(int timeout_sec = 300)
{
  Serial.print("Connecting to the network...");
  int time = 0;
  while(!lteConnected()) {
    Serial.print(".");
    delay(1000);
    time++;
    if(time > timeout_sec)
      return false;
  }
  Serial.println();
  Serial.println("Connected to the network");
  return true;
}

/**
 * @brief Disconnect from the LTE network.
 *
 * This function will disconnect the modem from the LTE network and block until
 * the network is actually disconnected. After the network is disconnected the
 * GNSS subsystem can be used.
 *
 * @return true on success, false on error.
 */
bool lteDisconnect()
{
  /* Set the operational state to minimum */
  if(modem.setOpState(WALTER_MODEM_OPSTATE_MINIMUM)) {
    Serial.println("Successfully set operational state to MINIMUM");
  } else {
    Serial.println("Error: Could not set operational state to MINIMUM");
    return false;
  }

  /* Wait for the network to become available */
  WalterModemNetworkRegState regState = modem.getNetworkRegState();
  while(regState != WALTER_MODEM_NETWORK_REG_NOT_SEARCHING) {
    delay(100);
    regState = modem.getNetworkRegState();
  }

  Serial.println("Disconnected from the network");
  return true;
}

/**
 * @brief This function tries to connect the modem to the cellular network.
 *
 * @return true on success, false on error.
 */
bool lteConnect()
{
  /* Set the operational state to NO RF */
  if(modem.setOpState(WALTER_MODEM_OPSTATE_NO_RF)) {
    Serial.println("Successfully set operational state to NO RF");
  } else {
    Serial.println("Error: Could not set operational state to NO RF");
    return false;
  }

  /* Create PDP context */
  if(modem.definePDPContext()) {
    Serial.println("Created PDP context");
  } else {
    Serial.println("Error: Could not create PDP context");
    return false;
  }

  /* Set the operational state to full */
  if(modem.setOpState(WALTER_MODEM_OPSTATE_FULL)) {
    Serial.println("Successfully set operational state to FULL");
  } else {
    Serial.println("Error: Could not set operational state to FULL");
    return false;
  }

  /* Set the network operator selection to automatic */
  if(modem.setNetworkSelectionMode(WALTER_MODEM_NETWORK_SEL_MODE_AUTOMATIC)) {
    Serial.println("Network selection mode was set to automatic");
  } else {
    Serial.println("Error: Could not set the network selection mode to automatic");
    return false;
  }

  return waitForNetwork();
}

/**
 * @brief Writes TLS credentials to the modem's NVS and configures the TLS profile.
 *
 * This function stores the provided TLS certificates and private keys into the modem's
 * non-volatile storage (NVS), and then sets up a TLS profile for secure communication.
 * These configuration changes are persistent across reboots.
 *
 * @note
 * - Certificate indexes 0–10 are reserved for Sequans and BlueCherry internal usage.
 * - Private key index 1 is reserved for BlueCherry internal usage.
 * - Do not attempt to override or use these reserved indexes.
 *
 * @return
 * - true if the credentials were successfully written and the profile configured.
 * - false otherwise.
 */
bool setupTLSProfile(void)
{

  if(!modem.tlsWriteCredential(false, 12, ca_cert)) {
    Serial.println("Error: CA cert upload failed");
    return false;
  }

  if(modem.tlsConfigProfile(MQTTS_TLS_PROFILE, WALTER_MODEM_TLS_VALIDATION_CA,
                            WALTER_MODEM_TLS_VERSION_12, 12)) {
    Serial.println("TLS profile configured");
  } else {
    Serial.println("Error: TLS profile configuration failed");
    return false;
  }

  return true;
}

/**
 * @brief Common routine to publish a message to an MQTTS topic.
 */
static bool mqttsPublishMessage(const char* topic, const char* message)
{
  Serial.printf("Publishing to topic '%s': %s\r\n", topic, message);
  if(modem.mqttPublish(topic, (uint8_t*) message, strlen(message))) {
    Serial.println("MQTTS publish succeeded");
    return true;
  }
  Serial.println("Error: MQTTS publish failed");
  return false;
}

/**
 * @brief Common routine to check for and print incoming MQTTS messages.
 */
static void mqttsCheckIncoming(const char* topic)
{
  while(modem.mqttDidRing(topic, incomingBuf, sizeof(incomingBuf), &rsp)) {
    Serial.printf("Incoming MQTTS message on '%s'\r\n", topic);
    Serial.printf("  QoS: %d, Message ID: %d, Length: %d\r\n", rsp.data.mqttResponse.qos,
                  rsp.data.mqttResponse.messageId, rsp.data.mqttResponse.length);
    Serial.println("  Payload:");
    for(int i = 0; i < rsp.data.mqttResponse.length; i++) {
      Serial.printf("  '%c' 0x%02X\r\n", incomingBuf[i], incomingBuf[i]);
    }
  }
}

/**
 * @brief The main Arduino setup method.
 */
void setup()
{
  Serial.begin(115200);
  delay(5000);

  Serial.printf("\r\n\r\n=== WalterModem MQTTS example ===\r\n\r\n");

  /* Build a unique client ID from the ESP MAC address */
  esp_read_mac(incomingBuf, ESP_MAC_WIFI_STA);
  sprintf(macString, "walter%02X:%02X:%02X:%02X:%02X:%02X", incomingBuf[0], incomingBuf[1],
          incomingBuf[2], incomingBuf[3], incomingBuf[4], incomingBuf[5]);

  /* Start the modem */
  if(WalterModem::begin(&Serial2)) {
    Serial.println("Successfully initialized the modem");
  } else {
    Serial.println("Error: Could not initialize the modem");
    return;
  }

  /* Connect the modem to the LTE network */
  if(!lteConnect()) {
    Serial.println("Error: Could not connect to LTE");
    return;
  }

  /* Set up the TLS profile */
  if(setupTLSProfile()) {
    Serial.println("TLS Profile setup succeeded");
  } else {
    Serial.println("Error: TLS Profile setup failed");
    return;
  }

  /* Configure the MQTTS client */
  if(modem.mqttConfig(MQTTS_CLIENT_ID, MQTTS_USERNAME, MQTTS_PASSWORD, MQTTS_TLS_PROFILE)) {
    Serial.println("Successfully configured the MQTTS client");
  } else {
    Serial.println("Error: Failed to configure MQTTS client");
    return;
  }

  /* Connect to a public MQTTS broker */
  if(modem.mqttConnect(MQTTS_HOST, MQTTS_PORT)) {
    Serial.println("Successfully connected to MQTTS broker");
  } else {
    Serial.println("Error: Failed to connect to MQTTS broker");
    return;
  }

  /* Subscribe to the test topic */
  if(modem.mqttSubscribe(MQTTS_TOPIC)) {
    Serial.printf("Successfully subscribed to '%s'", MQTTS_TOPIC);
  } else {
    Serial.println("Error: MQTTS subscribe failed");
  }
}

/**
 * @brief The main Arduino loop method.
 */
void loop()
{
  static unsigned long lastPublish = 0;
  const unsigned long publishInterval = 15000; // 15 seconds

  static int seq = 0;
  static char outgoingMsg[64];

  /* Periodically publish a message */
  if(millis() - lastPublish >= publishInterval) {
    lastPublish = millis();
    seq++;

    if(seq % 3 == 0) {
      sprintf(outgoingMsg, "%s-%d", macString, seq);
      if(!mqttsPublishMessage(MQTTS_TOPIC, outgoingMsg)) {
        Serial.println("MQTTS publish failed, restarting...");
        delay(1000);
        ESP.restart();
      }
      Serial.println();
    }
  }

  /* Check for incoming messages */
  mqttsCheckIncoming(MQTTS_TOPIC);
}