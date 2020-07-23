// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
// Modified by CW Aylward for DevOps dissertation project 2019, under the MIT open source license agreement

#include "AZ3166WiFi.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"

#include "config.h"
#include "utility.h"
#include "SystemTickCounter.h"

static bool hasWifi = false;
int messageCount = 1;
int sentMessageCount = 0;
static bool messageSending = true;
static uint64_t send_interval_ms;

static float temperature;
static float humidity;

// Add x509 Certificate to connect to IoT Edge Device using SSL
static const char edgeCert [] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIFdzCCA1+gAwIBAgIJAL17oSXERnbcMA0GCSqGSIb3DQEBCwUAMCoxKDAmBgNVBAMMH0F6dXJlX0lvVF9IdWJfQ0FfQ2VydF9UZXN0X09ubHkwHhcNMTkwOTE5MDkzMDE1WhcNMTkxMDE5MDkzMDE1WjAqMSgwJgYDVQQDDB9BenVyZV9Jb1RfSHViX0NBX0NlcnRfVGVzdF9Pbmx5MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAo9eViUY3Kpgn11f2dmS1iPKvhvVzjP3bsMSpFawA/x9V+8l3PirXlYLu6sCMHkxG9Qz12xuvN6NJBZW9BMd1zi0Xiy+r7+SCes6v54Cd3yWhYTDgEHcvHjq01WWeUgkOCFHjKFFLgIuYnwo56Qr2ebNyrsJi97MEkZTBQ+LgiyzuASzEcef6HhITHipC7Nj1L58O8mjOFETHyQZ1bSyZiAO82EjHSSPifjKk4Jxx27UDzkyQWQU5X6AzVFwf9KL6XiiQynQC4/z2DFfMmoCAQMXlmAKWcIpZcaXpQz10pSYns5X7TbBru2LP+XwqXaJFCP2LRN+VhuRpfkzTbFtLFCIIU95YMqcEBtoB3YWkCZaROMQropCxr3RSH8zxYakkknC6F/M2Q58uyWrBmnxXcXaj+rQVuructdKNn7jWDmonPfuYEpBW/hl4+pd48hu2TMJ4C1ZqgX16RG4bsZnPJ3vquSBumuD9fvwnGWhD/r9amSi4nv4j1gJcSEwmjF5TBM4A/iiMRhorN1gvRrBrbGCeIbn+H/oGq7POj/glOSZCsNiffBA72WBhCzxFK81vgmCvdarZY7bOZekoNPBi/U2gwPsf+Us+x+qS9keOCFDYJC2kwpniIB79PmSztE9IpcMfTEFkNId7/w3oqRzaPFljzUKQWFtfxAlLOSRb/RECAwEAAaOBnzCBnDAdBgNVHQ4EFgQUD3NdtJhg/rhwqs4enoEsoPKzJtQwWgYDVR0jBFMwUYAUD3NdtJhg/rhwqs4enoEsoPKzJtShLqQsMCoxKDAmBgNVBAMMH0F6dXJlX0lvVF9IdWJfQ0FfQ2VydF9UZXN0X09ubHmCCQC9e6ElxEZ23DAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjANBgkqhkiG9w0BAQsFAAOCAgEAHQRtcnCBJSbnEZOJnsHCsGpfsfbiLS5vFn/Fer5XfgWgHCmyu2dXbBzflSfaArmYz/bnCliyqXe8lDhLMSNoXQaUJgPwQchznaONgBqiHIUdxZJkPCBn/O4Ccz785kwXWwQwOmXfEoy9IdOjjCJqSmVZh9nkwzitaeqHhRQ3l3EhVH/BlEcue+RnHHOjhK9wud0QJHb2zrEQQNCxirKFTcy7SJVUPoMMbbEQbGT2D4BNC/y3dXpnT4s5pptBOe6GaoU0M/QXYHDHQVRUAgAK64OjHW5XvanmZTqVIF8fYx/gaScJr5P0kpVU8KlkNtC08eon7p4E3lefCi8KfSG40Hj5SE8ciZ0/W1MTNpwJTD3i7qzBHCFHYdQ8WJXxr8eqPcUMxzOKae9EPCkSZ0OEHFIH4JWZTXvNUtcYOLvxQVqqcBDH5Zp6D/bcEc8+Gxs2Pk32aJMI/7uPhQBUQte9/7OftQqsDNNGOKORPowCyq2FOI+CmTGGACngn1wCEKAJ7FaielHCq182H6aYWCBpNws8s6SwodcrL1In0NS+WNmM4lY32Bk5gc46URknU74SioFz1JOP5npLXV0cmhD/9OW+o8ODXk/pHib85uDiZ6vXiKq7CSnzFe+e7gJfCYzqQ7DF32XhYKWXcPNZmeLl/nKnvAMRTQ4JY6R2ntPht/k="
"-----END CERTIFICATE-----\r\n";

// Connect to Wifi 
static void InitWifi()
{
  Screen.print(2, "Connecting...");
  
  if (WiFi.begin() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Screen.print(1, ip.get_address());
    hasWifi = true;
    Screen.print(2, "Running... \r\n");
  }
  else
  {
    hasWifi = false;
    Screen.print(1, "No Wi-Fi\r\n ");
  }
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    blinkSendConfirmation();
    sentMessageCount++;
  }

  Screen.print(1, "> IoT Edge");
  char line1[20];
  sprintf(line1, "Count: %d/%d",sentMessageCount, messageCount); 
  Screen.print(2, line1);

  char line2[20];
  sprintf(line2, "T:%.2f H:%.2f", temperature, humidity);
  Screen.print(3, line2);
  messageCount++;
}

static void MessageCallback(const char* payLoad, int size)
{
  blinkLED();
  Screen.print(1, payLoad, true);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  parseTwinMessage(updateState, temp);
  free(temp);
}

static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}


// Set up the device 
void setup()
{
  Screen.init();
  Screen.print(0, "IoT DevKit");
  Screen.print(2, "Initializing...");
  
  Screen.print(3, " > Serial");
  Serial.begin(115200);

  // Initialize the WiFi module
  Screen.print(3, " > WiFi");
  hasWifi = false;
  InitWifi();
  if (!hasWifi)
  {
    return;
  }

  LogTrace("HappyPathSetup", NULL);

  Screen.print(3, " > Sensors");
  SensorInit();

  Screen.print(3, " > IoT Edge");
  DevKitMQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "DevKit-GetStarted");
  DevKitMQTTClient_SetOption("TrustedCerts", edgeCert);
  DevKitMQTTClient_Init(true);

  DevKitMQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  DevKitMQTTClient_SetMessageCallback(MessageCallback);
  DevKitMQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  DevKitMQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);

  send_interval_ms = SystemTickCounterRead();

}

void loop()
{
  if (hasWifi)
  {
    if (messageSending && 
        (int)(SystemTickCounterRead() - send_interval_ms) >= getInterval())
    {
      // Send teperature data
      char payloadMessage[MESSAGE_MAX_LEN];

      // Reading temperature and humidity data into a payload message
      bool tempHumAlarm = readMessage(messageCount, payloadMessage, &temperature, &humidity);
      // Generate an message event including the payload message
      EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(payloadMessage, MESSAGE);
      DevKitMQTTClient_Event_AddProp(message, "tempHumAlarm", tempHumAlarm ? "true" : "false");
      // Send the event instance for the IoT edge device to receive the message
      DevKitMQTTClient_SendEventInstance(message);

      Serial.println(payloadMessage);

      send_interval_ms = SystemTickCounterRead();
    }
    else
    {
      DevKitMQTTClient_Check();
    }
  }
  delay(5000);
}
