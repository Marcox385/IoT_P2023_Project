import os
from datetime import datetime
from fastapi import FastAPI

app = FastAPI()
path_db = os.path.abspath("Watering_System/db")


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

# Mandar requests para WATER al sistema

def send_water():
    # os.system(mqtt)
    pass
