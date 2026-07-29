import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from . import SmartliBms

DEPENDENCIES = ["smartli_bms"]

CONF_SMARTLI_BMS_ID = "smartli_bms_id"
CONF_CURRENT = "current"
CONF_PACK_VOLTAGE = "pack_voltage"
CONF_BUS_VOLTAGE = "bus_voltage"
CONF_STATE_OF_CHARGE = "state_of_charge"
CONF_STATE_OF_HEALTH = "state_of_health"
CONF_FULL_CAPACITY = "full_capacity"
CONF_REMAINING_CAPACITY = "remaining_capacity"
CONF_TOTAL_CHARGED_AH = "total_charged_ah"
CONF_TOTAL_DISCHARGED_AH = "total_discharged_ah"
CONF_CELL_MIN_VOLTAGE = "cell_min_voltage"
CONF_CELL_MAX_VOLTAGE = "cell_max_voltage"
CONF_CELL_DELTA_VOLTAGE = "cell_delta_voltage"


def voltage_sensor_schema(accuracy_decimals):
    return sensor.sensor_schema(
        unit_of_measurement="V",
        accuracy_decimals=accuracy_decimals,
        device_class="voltage",
        state_class="measurement",
    )


CELL_KEYS = [f"cell_voltage_{number}" for number in range(1, 16)]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SMARTLI_BMS_ID): cv.use_id(SmartliBms),
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement="A",
            accuracy_decimals=2,
            device_class="current",
            state_class="measurement",
        ),
        cv.Optional(CONF_PACK_VOLTAGE): voltage_sensor_schema(2),
        cv.Optional(CONF_BUS_VOLTAGE): voltage_sensor_schema(2),
        cv.Optional(CONF_STATE_OF_CHARGE): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=2,
            device_class="battery",
            state_class="measurement",
        ),
        cv.Optional(CONF_STATE_OF_HEALTH): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=2,
            state_class="measurement",
        ),
        cv.Optional(CONF_FULL_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=2,
            state_class="measurement",
        ),
        cv.Optional(CONF_REMAINING_CAPACITY): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=2,
            state_class="measurement",
        ),
        cv.Optional(CONF_TOTAL_CHARGED_AH): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=0,
            state_class="total_increasing",
        ),
        cv.Optional(CONF_TOTAL_DISCHARGED_AH): sensor.sensor_schema(
            unit_of_measurement="Ah",
            accuracy_decimals=0,
            state_class="total_increasing",
        ),
        cv.Optional(CONF_CELL_MIN_VOLTAGE): voltage_sensor_schema(3),
        cv.Optional(CONF_CELL_MAX_VOLTAGE): voltage_sensor_schema(3),
        cv.Optional(CONF_CELL_DELTA_VOLTAGE): voltage_sensor_schema(3),
        **{
            cv.Optional(cell_key): voltage_sensor_schema(3)
            for cell_key in CELL_KEYS
        },
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SMARTLI_BMS_ID])

    setters = {
        CONF_CURRENT: "set_current_sensor",
        CONF_PACK_VOLTAGE: "set_pack_voltage_sensor",
        CONF_BUS_VOLTAGE: "set_bus_voltage_sensor",
        CONF_STATE_OF_CHARGE: "set_state_of_charge_sensor",
        CONF_STATE_OF_HEALTH: "set_state_of_health_sensor",
        CONF_FULL_CAPACITY: "set_full_capacity_sensor",
        CONF_REMAINING_CAPACITY: "set_remaining_capacity_sensor",
        CONF_TOTAL_CHARGED_AH: "set_total_charged_ah_sensor",
        CONF_TOTAL_DISCHARGED_AH: "set_total_discharged_ah_sensor",
        CONF_CELL_MIN_VOLTAGE: "set_cell_min_voltage_sensor",
        CONF_CELL_MAX_VOLTAGE: "set_cell_max_voltage_sensor",
        CONF_CELL_DELTA_VOLTAGE: "set_cell_delta_voltage_sensor",
    }

    for key, setter in setters.items():
        if sensor_config := config.get(key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(getattr(parent, setter)(sens))

    for index, cell_key in enumerate(CELL_KEYS):
        if sensor_config := config.get(cell_key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(parent.set_cell_voltage_sensor(index, sens))
