import paho.mqtt.client as mqttClient
from . import create_register_file
from . import create_plant_register

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global Connected
        Connected = True
    else:
        print("Connection failed")

def on_message(client, userdata, message):
    print("Message received: ", message.payload)

    msg = str(message.payload)
    msg.split("#")
    print(msg)
    
    create_plant_register("test_plant")
    create_register_file("test_plant", 40, 50)

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