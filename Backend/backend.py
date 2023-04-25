import os
import time
from datetime import datetime
from fastapi import FastAPI
import paho.mqtt.client as mqttClient

app = FastAPI()
path_db = os.path.abspath("Watering_System/db")


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
    #create_plant_register(msg[0])
    #create_register_file(msg[0], msg[1], msg[2])

Connected = False

broker_address= "165.232.154.179"
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

def create_register_file(plant_id, humidity, water_level):
    time_atm = datetime.now().date().strftime('%m-%d-%Y')
    path_db_plant = path_db + f"/{plant_id}/"
    path_db_register = path_db_plant + f"{time_atm}.txt"

    if os.path.exists(path_db_register):
        with open(path_db_register, "a+") as file:
            file.write(f"\n{humidity}, {water_level}")
            print(f"Appended to file in path [{path_db_register}]")
    else:
        with open(path_db_register, "w+") as file:
            file.write(f"{humidity}, {water_level}")
            print(f"File created in path [{path_db_register}]")


def create_plant_register(plant_id):
    path_db_plant = path_db + f"/{plant_id}/"

    if os.path.exists(path_db_plant):
        print(f"{path_db_plant} already exists")
        return False

    os.makedirs(path_db_plant)
    print(f"Created [{path_db_plant}]")
    return True

@app.get("/{plant_id}")
def get_plant_registers(plant_id):
    path_db_plant = path_db + f"/{plant_id}"
    if os.path.exists(path_db_plant):
        return os.listdir(path_db_plant)

    else:
        return None
    
@app.get("/{plant_id}/{filename}")
def get_register_file(plant_id, filename):
    path_db_register = path_db + f"/{plant_id}/{filename}"
    if os.path.exists(path_db_register):
        with open(path_db_register, "r") as file:
            return [i.rstrip("\n") for i in file.readlines()]


# create_plant_register("test_plant")
# create_register_file("test_plant", 50, 75)
# x = get_plant_registers("test_plant")
# print(x)
# print(get_register_file("test_plant", x[0]))


def send_water(plant_id):
    os.system(f"mosquitto_pub -h localhost -t WaterBroadcast -m '{plant_id}' -u iot -P iot")
    print(f"MQTT message sent for watering {plant_id}")
