// Set your local WiFi username and password. Please use a 2.4GHz access point
#define WIFI_SSID     ""
#define WIFI_PASSWORD ""

// Put you name in below, and the location where you will place the sensor
#define YOUR_NAME ""

// Comma-separated Prometheus labels (key=value) to apply to the metrics from the sensor
// Example: site=home,location=study
//
// Adding labels might require you to increase the size of the WriteRequest buffer in M5StickCPlus.ino
#define PROM_LABELS "site=home,location=study"

// For more information on where to get these values see: https://github.com/grafana/diy-iot/blob/main/README.md#sending-metrics
#define GC_URL ""
#define GC_PATH "/api/prom/push"
#define GC_PORT 443
#define GC_USER ""
#define GC_PASS ""

// Set to 1 to show debug information on the LCD screen, or 0 to not display
// The debug information will show if you are able to connect to the WiFi or not, and whether writing the metrics to Prometheus was successful or not
#define LCD_SHOW_DEBUG_INFO "1"

// It is possible that you have the M5StickCPlus, or the M5StickC (without the Plus). They look very similar, but look at the sticker on the back where it will tell you 
// Below, set the value to 1 for the correct device that you have
#define I_HAVE_M5STICKCPLUS 1
#define I_HAVE_M5STICKC 0