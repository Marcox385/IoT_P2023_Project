import os
from datetime import datetime
from fastapi import FastAPI
from fastapi.responses import JSONResponse
import uvicorn

app = FastAPI()
headers = {"Access-Control-Allow-Origin": "http://iotmadnessproject.hopto.org"}

def create_register_file(plant_id, humidity, water_level):
    path_db = os.path.abspath("Watering_System/db")
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
    path_db = os.path.abspath("Watering_System/db")
    path_db_plant = path_db + f"/{plant_id}/"

    if os.path.exists(path_db_plant):
        print(f"{path_db_plant} already exists")
        return False

    os.makedirs(path_db_plant)
    print(f"Created [{path_db_plant}]")
    return True

@app.get("/{plant_id}")
def get_plant_registers(plant_id):
    path_db = os.path.abspath("Watering_System/db")
    path_db_plant = path_db + f"/{plant_id}"
    if os.path.exists(path_db_plant):
        return JSONResponse(content={"list": os.listdir(path_db_plant)} headers=headers)

    else:
        return JSONResponse(content={} headers=headers)
    
@app.get("/{plant_id}/{filename}")
def get_register_file(plant_id, filename):
    path_db = os.path.abspath("Watering_System/db")
    path_db_register = path_db + f"/{plant_id}/{filename}"
    if os.path.exists(path_db_register):
        with open(path_db_register, "r") as file:
            return JSONResponse(content={"list": [i.rstrip("\n") for i in file.readlines()]} headers=headers)


# create_plant_register("test_plant")
# create_register_file("test_plant", 50, 75)
# x = get_plant_registers("test_plant")
# print(x)
# print(get_register_file("test_plant", x[0]))


def send_water(plant_id):
    os.system(f"mosquitto_pub -h localhost -t WaterBroadcast -m '{plant_id}' -u iot -P iot")
    print(f"MQTT message sent for watering {plant_id}")

if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=5000)