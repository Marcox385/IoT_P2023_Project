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
    print("Message received: ", message.payload)

    msg = str(message.payload).decode('ascii')
    msg.split()
    print(msg)
    plant_id = int(msg[0][1:-1])
    humidity = int(msg[1][8:-2])
    water_lvl = int(msg[2][11:-2])
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