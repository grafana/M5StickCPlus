// Written for Grafana Labs to demonstrate how to use the M5Stick CPlus with Grafana Cloud
// 2023/01/21
// Willie Engelbrecht - willie.engelbrecht@grafana.com
// Introduction to time series: https://grafana.com/docs/grafana/latest/fundamentals/timeseries/
// All the battery API documentation: https://docs.m5stack.com/en/api/stickc/axp192_m5stickc
// Register for a free Grafana Cloud account including free metrics and logs: https://grafana.com


// ===================================================
// All the things that needs to be changed 
// Your local WiFi details
// Your Grafana Cloud details
// ===================================================
#include "config.h"


// ===================================================
// Includes for the M5StickC Plus or M5StickC - no need to change anything here
// ===================================================
#if I_HAVE_M5STICKCPLUS == 1
  #include <M5StickCPlus.h>
#elif I_HAVE_M5STICKC == 1
  #include <M5StickC.h>
#endif  
#include "M5_ENV.h"

TFT_eSprite Disbuff = TFT_eSprite(&M5.Lcd);

// ===================================================
// Includes - Needed to write Prometheus or Loki metrics/logs to Grafana Cloud
// No need to change anything here
// ===================================================
#include "certificates.h"
#include <PromLokiTransport.h>
#include <PrometheusArduino.h>


// ===================================================
// Global Variables
// ===================================================
SHT3X sht30;              // temperature and humidity sensor
QMP6988 qmp6988;          // pressure sensor

float temp     = 0.0;
float hum      = 0.0;
float pressure = 0.0;

int upload_fail_count = 0;

// Client for Prometheus metrics
PromLokiTransport transport;
PromClient client(transport);

// Create a write request for 11 time series.
WriteRequest req(11, 2048);

// Define all our timeseries
TimeSeries ts_m5stick_temperature(1, "m5stick_temp", PROM_LABELS);
TimeSeries ts_m5stick_humidity(1, "m5stick_hum", PROM_LABELS);
TimeSeries ts_m5stick_pressure(1, "m5stick_pressure", PROM_LABELS);
TimeSeries ts_m5stick_Iusb(1, "m5stick_Iusb", PROM_LABELS);
TimeSeries ts_m5stick_disCharge(1, "m5stick_disCharge", PROM_LABELS);
TimeSeries ts_m5stick_Iin(1, "m5stick_Iin", PROM_LABELS);
TimeSeries ts_m5stick_BatTemp(1, "m5stick_BatTemp", PROM_LABELS);
TimeSeries ts_m5stick_Vaps(1, "m5stick_Vaps", PROM_LABELS);
TimeSeries ts_m5stick_bat(1, "m5stick_bat", PROM_LABELS);
TimeSeries ts_m5stick_charge(1, "m5stick_charge", PROM_LABELS);
TimeSeries ts_m5stick_vbat(1, "m5stick_vbat", PROM_LABELS);


// ===================================================
// Set up procedure that runs once when the controller starts
// ===================================================
void setup() {
    M5.begin();               // Init M5StickCPlus.  
    M5.Lcd.setRotation(3);    // Rotate the screen.  
    Disbuff.createSprite(240, 135);
    Disbuff.fillRect(0, 0, 240, 135, TFT_BLACK);

    Disbuff.setTextSize(2);
    Disbuff.setCursor(10, 10);
    Disbuff.setTextColor(ORANGE, BLACK);
    Disbuff.printf("==  Grafana Labs ==");
    Disbuff.setTextColor(WHITE, BLACK);
    
    Wire.begin(32, 33);       // Wire init, adding the I2C bus.  
    qmp6988.init();           // Initiallize the pressure sensor    
    sht30.init();             // Initialize the temperature sensor

    // Configure and start the transport/WiFi layer
    Disbuff.setCursor(10, 30);
    Disbuff.printf("Hello, %s", YOUR_NAME);
    Disbuff.setCursor(10, 60);
    Disbuff.printf("Please wait:\r\n Connecting to WiFi");
    Disbuff.pushSprite(0, 0);
    transport.setUseTls(true);
    transport.setCerts(grafanaCert, strlen(grafanaCert));
    transport.setWifiSsid(WIFI_SSID);
    transport.setWifiPass(WIFI_PASSWORD);
    transport.setDebug(Serial);  // Remove this line to disable debug logging of the client.
    if (!transport.begin()) {
        Serial.println(transport.errmsg);
        while (true) {};        
    }
    Disbuff.setCursor(10, 105);
    Disbuff.setTextColor(GREEN, BLACK);
    Disbuff.printf("Connected!"); 
    Disbuff.pushSprite(0, 0); 
    delay(1500); 
    
    // Configure the Grafana Cloud client
    client.setUrl(GC_URL);
    client.setPath((char*)GC_PATH);
    client.setPort(GC_PORT);
    client.setUser(GC_USER);
    client.setPass(GC_PASS);
    client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
    if (!client.begin()) {
        Serial.println(client.errmsg);
        while (true) {};
    }
   
    //Serial.println(prometheus_labels);


    // Add our TimeSeries to the WriteRequest
    req.addTimeSeries(ts_m5stick_temperature);
    req.addTimeSeries(ts_m5stick_humidity);
    req.addTimeSeries(ts_m5stick_pressure);
    req.addTimeSeries(ts_m5stick_Iusb);
    req.addTimeSeries(ts_m5stick_disCharge);
    req.addTimeSeries(ts_m5stick_Iin);
    req.addTimeSeries(ts_m5stick_BatTemp);
    req.addTimeSeries(ts_m5stick_Vaps);
    req.addTimeSeries(ts_m5stick_bat);
    req.addTimeSeries(ts_m5stick_charge);
    req.addTimeSeries(ts_m5stick_vbat);

    req.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.

}

// ===================================================
// Loop continiously until forever,
// reading from both sensors, and submitting to your
// Grafana Cloud account for visualisation
// ===================================================
void loop() {
    Disbuff.setTextColor(WHITE, BLACK);
    int64_t time;
    time = transport.getTimeMillis();
    Serial.printf("\r\n====================================\r\n");
    
    // Get new updated values from our sensor
    pressure = qmp6988.calcPressure();
    if (sht30.get() == 0) {     // Obtain the data of sht30.
        temp = sht30.cTemp;      // Store the temperature obtained from sht30.
        hum  = sht30.humidity;   // Store the humidity obtained from the sht30.
    } else {
        temp = 0, hum = 0;
    }
    if (pressure < 950) { ESP.restart(); } // Sometimes this sensor fails, and if we get an invalid reading it's best to just restart the controller to clear it out
    if (pressure/100 > 1200) { ESP.restart(); } // Sometimes this sensor fails, and if we get an invalid reading it's best to just restart the controller to clear it out
    Serial.printf("Temp: %2.1f °C \r\nHumi: %2.0f%%  \r\nPressure:%2.0f hPa\r\n", temp, hum, pressure / 100);

    // Gather some internal data as well, about battery states, voltages, charge rates and so on
    int Iusb = M5.Axp.GetIdischargeData() * 0.375;
    Serial.printf("Iusbin:%da\r\n", Iusb);

    int disCharge = M5.Axp.GetIdischargeData() / 2;
    Serial.printf("disCharge:%dma\r\n", disCharge);

    double Iin = M5.Axp.GetIinData() * 0.625;
    Serial.printf("Iin:%.3fmA\r\n", Iin);

    int BatTemp = M5.Axp.GetTempData()*0.1-144.7;
    Serial.printf("Battery temperature:%d\r\n", BatTemp);

    int Vaps = M5.Axp.GetVapsData();
    Serial.printf("battery capacity :%dmW\r\n", Vaps);

    int bat = M5.Axp.GetPowerbatData()*1.1*0.5/1000;
    Serial.printf("battery power:%dmW\r\n", bat);

    int charge = M5.Axp.GetIchargeData() / 2;
    Serial.printf("icharge:%dmA\r\n", charge);

    double vbat = M5.Axp.GetVbatData() * 1.1 / 1000;
    Serial.printf("vbat:%.3fV\r\n", vbat);

    // Now add all of our collected data to the timeseries
    ts_m5stick_temperature.addSample(time, temp);
    ts_m5stick_humidity.addSample(time, hum);
    ts_m5stick_pressure.addSample(time, pressure);
    ts_m5stick_Iusb.addSample(time, Iusb);
    ts_m5stick_disCharge.addSample(time, disCharge);
    ts_m5stick_Iin.addSample(time, Iin);
    ts_m5stick_BatTemp.addSample(time, BatTemp);
    ts_m5stick_Vaps.addSample(time, Vaps);
    ts_m5stick_bat.addSample(time, bat);
    ts_m5stick_charge.addSample(time, charge);
    ts_m5stick_vbat.addSample(time, vbat);

    // Now send all of our data to Grafana Cloud!
    PromClient::SendResult res = client.send(req);
    ts_m5stick_temperature.resetSamples();
    ts_m5stick_humidity.resetSamples();
    ts_m5stick_pressure.resetSamples();
    ts_m5stick_Iusb.resetSamples();
    ts_m5stick_disCharge.resetSamples();
    ts_m5stick_Iin.resetSamples();
    ts_m5stick_BatTemp.resetSamples();
    ts_m5stick_Vaps.resetSamples();
    ts_m5stick_bat.resetSamples();
    ts_m5stick_charge.resetSamples();
    ts_m5stick_vbat.resetSamples();
    
    // Update the LCD screen
    Disbuff.fillRect(0, 30, 240, 135, BLACK);  // Fill the screen with black (to clear the screen).
    Disbuff.setCursor(0, 40);
    Disbuff.printf("  Temp: %2.1f  \r\n  Humi: %2.0f%%  \r\n  Pressure:%2.0f hPa\r\n", temp, hum, pressure / 100);
    

    // Display some debug information on the screen to make it easier to determine if your device is working or not. Can be disabled in the config.h file
    if (LCD_SHOW_DEBUG_INFO == "1") {
      // Are we connected to WiFi or not ?
      Disbuff.setCursor(0, 92);
      if (WiFi.status() != WL_CONNECTED) {
        Disbuff.setTextColor(WHITE, BLACK);
        Disbuff.printf("  Wifi: ");
        Disbuff.setTextColor(RED, BLACK);
        Disbuff.printf("Not connected!");
      }
      else {          
        Disbuff.printf("  Wifi: ");
        Disbuff.setTextColor(GREEN, BLACK);
        Disbuff.printf("Connected");
      }

      // Are we able to upload metrics or not - display on the LCD screen if configured in config.h
      Disbuff.setCursor(0, 110);
      if (res == 0) {
        upload_fail_count = 0;
        Disbuff.setTextColor(WHITE, BLACK);
        Disbuff.print("  Upload ");
        Disbuff.setTextColor(GREEN, BLACK);
        Disbuff.print("complete");        
      } else {
        upload_fail_count += 1;
        Disbuff.setTextColor(WHITE, BLACK);
        Disbuff.print("  Upload ");
        Disbuff.setTextColor(RED, BLACK);
        Disbuff.print("failed: " + String(upload_fail_count));
      }
    }

    // Copy the display buffer to the actual LCD screen. This VERY fast and looks like an instantanious update. No flickering
    Disbuff.pushSprite(0, 0);

    // Sleep for 5 seconds
    delay(5000);
}
