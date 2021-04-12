void networkStartup()
{
  int result  = 0;
  int counter = 0;

  commSerial.print("Connecting to ");
  commSerial.println(WIFI_SSID);

  /* Set to station mode and connect to network */

  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(192, 168, 1, 80), IPAddress(192, 168, 1, 0), IPAddress(255, 255, 255, 0));
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);

  /* Wait for connection */

  while ((WiFi.status() != WL_CONNECTED) && (counter < RETRY_COUNT)) {
    counter++;
    commSerial.print(".");
    delay(500);
  }
  commSerial.println("");

  /* Check if we obtained an IP address */

  if (WiFi.status() != WL_CONNECTED) {
    commSerial.println("Failed to connect.");
    result = 0;
  } else {
    commSerial.println("");
    commSerial.println("WiFi connected");
    commSerial.println("IP address: ");
    commSerial.println(WiFi.localIP());
    result = 1;
  }
}
