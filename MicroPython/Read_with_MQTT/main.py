# Function to connect the ESP8266 with the MQTT broker
def connect_mqtt():
  mqtt_client = MQTTClient(MQTT_CLIENT, MQTT_BROKER, user=MQTT_USERNAME, password=MQTT_PASSWORD)
  mqtt_client.connect()
  print('Connected to %s MQTT broker.' % (MQTT_BROKER))
  return mqtt_client

# Function to reset the ESP8266 when it's not connected to the MQTT broker
def reset():
  print('Failed to connect to MQTT broker. Reconnecting...')
  time.sleep(10)
  machine.reset()

# Function to get the obis code from the telegram_line    
def get_obis_code(telegram_line):
  end = telegram_line.find("(")   # Find the end position from the obis code

  if end == -1:                   # If no end is found, return the full telegram_line
    return telegram_line
  
  return telegram_line[0:end]   

# Function to get the value from the telegram_line  
def get_value(telegram_line):
  start = telegram_line.find("(")  # Find the start position from the value
  
  end = telegram_line.find("*")    # Find the end position from the value
  if end == -1:
    end = telegram_line.find(")")
    
  if start == -1 or end == -1:     # If no start or end is found, return the full telegram_line
    return telegram_line
  
  return telegram_line[start+1:end]

# Connect with the MQTT broker
try:
  mqtt_client = connect_mqtt()
except OSError as e:
  reset()

# Read from the P1 port (input()) in a loop and send the data to the MQTT broker
while True:
  try:
    telegram_line = input()
    
    obis_code = get_obis_code(telegram_line)
    value = get_value(telegram_line)
    
    if obis_code in data_objects:
      mqtt_client.publish(data_objects[obis_code], value)
  
  except OSError as e:
    reset()
  except EOFError:
    break

