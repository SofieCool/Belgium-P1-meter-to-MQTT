import time
from umqttsimple import MQTTClient
import ubinascii
import machine
import micropython
import network
import esp
esp.osdebug(None)
import gc
gc.collect()

# Communication constants
WIFI_SSID = "YOUR WIFI SSID"
WIFI_PASSWORD = "YOUR WIFI PASSWORD"
MQTT_BROKER = "YOUR MQTT BROKER IP ADRESS"
MQTT_USERNAME = "YOUR MQTT USERNAME"
MQTT_PASSWORD = "YOUR MQTT PASSWORD"
MQTT_CLIENT = ubinascii.hexlify(machine.unique_id())  # Gives a unique id to the MQTT client. You dont have to change it yourself.

# Dictionary of all data objects from a telegram
data_objects = {
  "0-0:96.1.4": "meter/id",
  "1-0:1.8.1": "meter/electricity/consumption/day",
  "1-0:1.8.2": "meter/electricity/consumption/night",
  "1-0:2.8.1": "meter/electricity/production/day",
  "1-0:2.8.2": "meter/electricity/production/night",
  "0-0:96.14.0": "meter/electricity/current_rate",
  "1-0:1.7.0": "meter/electricity/consumption/all_phases",
  "1-0:2.7.0": "meter/electricity/production/all_phases",
  "1-0:21.7.0": "meter/electricity/consumption/phase1",
  "1-0:22.7.0": "meter/electricity/production/phase1",
  "1-0:32.7.0": "meter/electricity/voltage/phase1",
  "1-0:52.7.0": "meter/electricity/voltage/phase2",
  "1-0:72.7.0": "meter/electricity/voltage/phase3",
  "1-0:31.7.0": "meter/electricity/current/phase1",
  "1-0:51.7.0": "meter/electricity/current/phase2",
  "1-0:71.7.0": "meter/electricity/current/phase3",
  "0-0:96.3.10": "meter/electricity/switch_position",
  "0-0:17.0.0": "meter/electricity/max_allowed_power_phase",
  "1-0:31.4.0": "meter/electricity/max_allowed_current_phase",
  "0-0:96.13.0": "meter/message",
  "0-1:24.1.0": "meter/other_devices_on_bus",
  "0-1:24.4.0": "meter/gas/switch_position",
  "0-1:24.2.3": "meter/gas/reading"
}

# Connect the ESP8266 with the router over WiFi
wifi_client = network.WLAN(network.STA_IF)

wifi_client.active(True)
wifi_client.connect(WIFI_SSID, WIFI_PASSWORD)

while not wifi_client.isconnected():
  pass

print('Connection successful')
print(wifi_client.ifconfig())

