import os
from datetime import datetime
from fastapi import FastAPI
from fastapi.responses import JSONResponse
import uvicorn

app = FastAPI()
headers = {"Access-Control-Allow-Origin": "*"}

def create_register_file(plant_id, humidity, humidity_desc, water_lvl, water_lvl_desc):
    path_db = os.path.abspath("/home/IoT_P2023_Project/Backend/Watering_System/db")
    time_atm = datetime.now().date().strftime('%m-%d-%Y')
    path_db_plant = path_db + f"/{plant_id}/"
    path_db_register = path_db_plant + f"{time_atm}.txt"

    if os.path.exists(path_db_register):
        with open(path_db_register, "a+") as file:
            file.write(f"\n{humidity}, {humidity_desc}, {water_lvl}, {water_lvl_desc}")
            print(f"Appended to file in path [{path_db_register}]")
    else:
        with open(path_db_register, "w+") as file:
            file.write(f"{humidity}, {humidity_desc}, {water_lvl}, {water_lvl_desc}")
            print(f"File created in path [{path_db_register}]")


def create_plant_register(plant_id):
    path_db = os.path.abspath("/home/IoT_P2023_Project/Backend/Watering_System/db")
    path_db_plant = path_db + f"/{plant_id}/"

    if os.path.exists(path_db_plant):
        print(f"{path_db_plant} already exists")
        return False

    os.makedirs(path_db_plant)
    print(f"Created [{path_db_plant}]")
    return True

@app.get("/water/{plant_id}")
def send_water(plant_id):
    os.system(f"mosquitto_pub -h localhost -t WaterBroadcast -m '{plant_id}' -u iot -P iot")
    print(f"MQTT message sent for watering {plant_id}")
    return JSONResponse(content={}, headers=headers)

@app.get("/{plant_id}")
def get_plant_registers(plant_id):
    path_db = os.path.abspath("/home/IoT_P2023_Project/Backend/Watering_System/db")
    path_db_plant = path_db + f"/{plant_id}"
    
    if os.path.exists(path_db_plant):
        return JSONResponse(content={"list": os.listdir(path_db_plant)}, headers=headers)

    else:
        return JSONResponse(content={}, headers=headers)
    
@app.get("/{plant_id}/{filename}")
def get_register_file(plant_id, filename):
    path_db = os.path.abspath("/home/IoT_P2023_Project/Backend/Watering_System/db")
    path_db_register = path_db + f"/{plant_id}/{filename}"

    if os.path.exists(path_db_register):
        with open(path_db_register, "r") as file:
            content = [i.rstrip("\n") for i in file.readlines()]
            y = []
            for i in content:
                x = i.split(", ")
                y.append({"humidity": x[0], "humidityStatus": x[1], "waterLevel": x[2], "waterLevelStatus": x[3]})
            return JSONResponse(content={"list": y}, headers=headers)

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=5000, ssl_certfile="/etc/letsencrypt/live/iotmadnessproject.hopto.org/fullchain.pem", ssl_keyfile="/etc/letsencrypt/live/iotmadnessproject.hopto.org/privkey.pem")