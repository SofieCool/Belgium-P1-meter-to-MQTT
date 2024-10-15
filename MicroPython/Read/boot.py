
# Function to get the value from the telegram_line  
def get_value(telegram_line):
  start = telegram_line.find("(")  # Find the start position from the value
  
  end = telegram_line.find("*")    # Find the end position from the value
  if end == -1:
    end = telegram_line.find(")")
    
  if start == -1 or end == -1:     # If no start or end is found, return the full telegram_line
    return telegram_line
  
  return telegram_line[start+1:end]

# Read from the P1 port (input()) in a loop and send the data to the MQTT broker
while True:
  try:
    telegram_line = input()
    
    value = get_value(telegram_line)
    print(value)
 
  except OSError as e:
    break
  except EOFError:
    break


