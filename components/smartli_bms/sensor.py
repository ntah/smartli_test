import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from . import SmartliBms

DEPENDENCIES = ["smartli_bms"]

CONF_SMARTLI_BMS_ID = "smartli_bms_id"
CONF_ADDRESS = "address"
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
CONF_CELL_AVERAGE_VOLTAGE = "cell_average_voltage"
CONF_POWER = "power"
CONF_MAX_TEMPERATURE = "max_temperature"
CONF_MOS_TEMPERATURE = "mos_temperature"
CONF_CYCLE_COUNT = "cycle_count"
CONF_DCDC_BUS_CURRENT = "dcdc_bus_current"
CONF_DCDC_DISCHARGE_BUS_VOLTAGE_SET = "dcdc_discharge_bus_voltage_set"
CONF_DCDC_DISCHARGE_BUS_CURRENT_SET = "dcdc_discharge_bus_current_set"
CONF_DCDC_DISCHARGE_BUS_POWER_SET = "dcdc_discharge_bus_power_set"
CONF_DCDC_CHARGING_BATTERY_VOLTAGE_SET = "dcdc_charging_battery_voltage_set"
CONF_DCDC_CHARGE_CURRENT_SET = "dcdc_charge_current_set"
CONF_DCDC_CHARGING_BATTERY_POWER_SET = "dcdc_charging_battery_power_set"
CONF_DCDC_BUS_VOLTAGE_LADDER = "dcdc_bus_voltage_ladder"
CONF_DCDC_DEPTH_DOD = "dcdc_depth_dod"
CONF_DCDC_VBUS_SET_MAX_AUTOSELF = "dcdc_vbus_set_max_autoself"
ALARM_KEYS = [f"alarm_status_{number}" for number in range(1, 6)]
TEMPERATURE_KEYS = [
    "battery_temperature_1",
    "battery_temperature_2",
    "battery_temperature_3",
    "battery_temperature_4",
    "environment_temperature",
    "mos_temperature_1",
    "mos_temperature_2",
    "balance_temperature",
]


def voltage_sensor_schema(accuracy_decimals):
    return sensor.sensor_schema(
        unit_of_measurement="V",
        accuracy_decimals=accuracy_decimals,
        device_class="voltage",
        state_class="measurement",
    )


CELL_KEYS = [f"cell_voltage_{number}" for number in range(1, 16)]


def current_sensor_schema():
    return sensor.sensor_schema(
        unit_of_measurement="A",
        accuracy_decimals=2,
        device_class="current",
        state_class="measurement",
    )


def percent_sensor_schema():
    return sensor.sensor_schema(
        unit_of_measurement="%",
        accuracy_decimals=2,
        state_class="measurement",
    )

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SMARTLI_BMS_ID): cv.use_id(SmartliBms),
        cv.Required(CONF_ADDRESS): cv.int_range(min=1, max=247),
        cv.Optional(CONF_CURRENT): current_sensor_schema(),
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
        cv.Optional(CONF_CELL_AVERAGE_VOLTAGE): voltage_sensor_schema(3),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement="W", accuracy_decimals=2,
            device_class="power", state_class="measurement",
        ),
        cv.Optional(CONF_MAX_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C", accuracy_decimals=0,
            device_class="temperature", state_class="measurement",
        ),
        cv.Optional(CONF_MOS_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C", accuracy_decimals=0,
            device_class="temperature", state_class="measurement",
        ),
        cv.Optional(CONF_CYCLE_COUNT): sensor.sensor_schema(
            unit_of_measurement="n", accuracy_decimals=0,
            state_class="total_increasing",
        ),
        cv.Optional(CONF_DCDC_BUS_CURRENT): current_sensor_schema(),
        cv.Optional(CONF_DCDC_DISCHARGE_BUS_VOLTAGE_SET): voltage_sensor_schema(2),
        cv.Optional(CONF_DCDC_DISCHARGE_BUS_CURRENT_SET): percent_sensor_schema(),
        cv.Optional(CONF_DCDC_DISCHARGE_BUS_POWER_SET): percent_sensor_schema(),
        cv.Optional(CONF_DCDC_CHARGING_BATTERY_VOLTAGE_SET): voltage_sensor_schema(2),
        cv.Optional(CONF_DCDC_CHARGE_CURRENT_SET): percent_sensor_schema(),
        cv.Optional(CONF_DCDC_CHARGING_BATTERY_POWER_SET): percent_sensor_schema(),
        cv.Optional(CONF_DCDC_BUS_VOLTAGE_LADDER): voltage_sensor_schema(2),
        cv.Optional(CONF_DCDC_DEPTH_DOD): percent_sensor_schema(),
        cv.Optional(CONF_DCDC_VBUS_SET_MAX_AUTOSELF): voltage_sensor_schema(2),
        **{
            cv.Optional(key): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category="diagnostic",
            )
            for key in ALARM_KEYS
        },
        **{
            cv.Optional(cell_key): voltage_sensor_schema(3)
            for cell_key in CELL_KEYS
        },
        **{
            cv.Optional(key): sensor.sensor_schema(
                unit_of_measurement="°C",
                accuracy_decimals=0,
                device_class="temperature",
                state_class="measurement",
            )
            for key in TEMPERATURE_KEYS
        },
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SMARTLI_BMS_ID])
    address = config[CONF_ADDRESS]

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
        CONF_CELL_AVERAGE_VOLTAGE: "set_cell_average_voltage_sensor",
        CONF_POWER: "set_power_sensor",
        CONF_MAX_TEMPERATURE: "set_max_temperature_sensor",
        CONF_MOS_TEMPERATURE: "set_mos_temperature_sensor",
        CONF_CYCLE_COUNT: "set_cycle_count_sensor",
        CONF_DCDC_BUS_CURRENT: "set_dcdc_bus_current_sensor",
        CONF_DCDC_DISCHARGE_BUS_VOLTAGE_SET: "set_dcdc_discharge_bus_voltage_set_sensor",
        CONF_DCDC_DISCHARGE_BUS_CURRENT_SET: "set_dcdc_discharge_bus_current_set_sensor",
        CONF_DCDC_DISCHARGE_BUS_POWER_SET: "set_dcdc_discharge_bus_power_set_sensor",
        CONF_DCDC_CHARGING_BATTERY_VOLTAGE_SET: "set_dcdc_charging_battery_voltage_set_sensor",
        CONF_DCDC_CHARGE_CURRENT_SET: "set_dcdc_charge_current_set_sensor",
        CONF_DCDC_CHARGING_BATTERY_POWER_SET: "set_dcdc_charging_battery_power_set_sensor",
        CONF_DCDC_BUS_VOLTAGE_LADDER: "set_dcdc_bus_voltage_ladder_sensor",
        CONF_DCDC_DEPTH_DOD: "set_dcdc_depth_dod_sensor",
        CONF_DCDC_VBUS_SET_MAX_AUTOSELF: "set_dcdc_vbus_set_max_autoself_sensor",
    }

    for key, setter in setters.items():
        if sensor_config := config.get(key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(getattr(parent, setter)(address, sens))

    for index, cell_key in enumerate(CELL_KEYS):
        if sensor_config := config.get(cell_key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(parent.set_cell_voltage_sensor(address, index, sens))

    for index, alarm_key in enumerate(ALARM_KEYS):
        if sensor_config := config.get(alarm_key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(parent.set_alarm_status_sensor(address, index, sens))

    for index, key in enumerate(TEMPERATURE_KEYS):
        if sensor_config := config.get(key):
            sens = await sensor.new_sensor(sensor_config)
            cg.add(parent.set_temperature_sensor(address, index, sens))
