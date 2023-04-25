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

    plant_id = int(msg[0][1:])
    humidity = int(msg[1][9:11])
    water_lvl = int(msg[2][12:14])

    create_plant_register(plant_id)
    create_register_file(plant_id, humidity, water_lvl)

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