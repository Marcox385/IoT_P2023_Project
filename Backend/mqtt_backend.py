import paho.mqtt.client as mqttClient
from backend import create_register_file
from backend import create_plant_register

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global Connected
        Connected = True
    else:
        print("Connection failed")

def on_message(client, userdata, message):
    msg = str(message.payload.decode('ascii'))
    msg = msg.split(" ")
    print("Message received: ", msg)

    try:
        plant_id = msg[2].strip()
        percentage_index = msg[0].find("%")
        humidity = int(msg[0][9:percentage_index])
        percentage_index = msg[1].find("%")
        water_lvl = int(msg[1][12:percentage_index])

        parenthesis_index_start = msg[0].find("(")
        parenthesis_index_end = msg[0].find(")")
        humidity_desc = msg[0][parenthesis_index_start + 1:parenthesis_index_end]
        parenthesis_index_start = msg[1].find("(")
        parenthesis_index_end = msg[1].find(")")
        water_lvl_desc = msg[1][parenthesis_index_start + 1:parenthesis_index_end]
    except Exception as e:
        pass
    else:
        create_plant_register(plant_id)
        create_register_file(plant_id, humidity, humidity_desc, water_lvl, water_lvl_desc)

Connected = False

broker_address= "localhost"
port = 1883
user = "iot"
password = "iot"

client = mqttClient.Client("Python")
client.username_pw_set(user, password=password)
client.on_connect = on_connect
client.on_message = on_message
client.connect(broker_address, port, 60)
client.subscribe("PlantStats")
client.loop_forever()
