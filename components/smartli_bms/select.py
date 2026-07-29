import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import SmartliBms, smartli_bms_ns

DEPENDENCIES = ["smartli_bms"]

CONF_SMARTLI_BMS_ID = "smartli_bms_id"
CONF_ADDRESS = "address"
CONF_VBUS_DISCHARGE_SET = "vbus_discharge_set"
CONF_VBUS_DOD = "vbus_dod"
CONF_IBUS_PERCENT = "ibus_percent"
CONF_PBUS_PERCENT = "pbus_percent"
CONF_AVG_CHARGE_PERCENT = "avg_charge_percent"
CONF_DOD_PERCENT = "dod_percent"
CONF_CHARGING_LOOP = "charging_loop"
CONF_DISCHARGE_LOOP = "discharge_loop"
CONF_MODE = "mode"

SmartliBmsSelect = smartli_bms_ns.class_("SmartliBmsSelect", select.Select)
SmartliSelectType = smartli_bms_ns.enum("SmartliSelectType")

SELECTS = {
    CONF_VBUS_DISCHARGE_SET: (
        SmartliSelectType.VBUS_DISCHARGE,
        ["49.0V", "49.5V", "50.0V", "50.5V", "51.0V"],
    ),
    CONF_VBUS_DOD: (
        SmartliSelectType.VBUS_DOD,
        [f"{value / 10:.1f}V" for value in range(480, 501)],
    ),
    CONF_IBUS_PERCENT: (
        SmartliSelectType.IBUS_PERCENT,
        [f"{value}%" for value in range(0, 101, 5)],
    ),
    CONF_PBUS_PERCENT: (
        SmartliSelectType.PBUS_PERCENT,
        ["0%", "3%"] + [f"{value}%" for value in range(5, 101, 5)],
    ),
    CONF_AVG_CHARGE_PERCENT: (
        SmartliSelectType.AVG_CHARGE_PERCENT,
        [f"{value}%" for value in range(0, 101, 5)],
    ),
    CONF_DOD_PERCENT: (
        SmartliSelectType.DOD_PERCENT,
        [f"{value}%" for value in range(0, 101, 5)],
    ),
    CONF_CHARGING_LOOP: (
        SmartliSelectType.CHARGING_LOOP,
        ["Enable", "Disable"],
    ),
    CONF_DISCHARGE_LOOP: (
        SmartliSelectType.DISCHARGE_LOOP,
        ["Enable", "Disable"],
    ),
    CONF_MODE: (
        SmartliSelectType.MODE_ALL,
        ["Constant", "Battery"],
    ),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SMARTLI_BMS_ID): cv.use_id(SmartliBms),
        cv.Required(CONF_ADDRESS): cv.int_range(min=1, max=247),
        **{
            cv.Optional(key): select.select_schema(
                SmartliBmsSelect,
                entity_category=ENTITY_CATEGORY_CONFIG,
            )
            for key in SELECTS
        },
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SMARTLI_BMS_ID])
    address = config[CONF_ADDRESS]
    for key, (select_type, options) in SELECTS.items():
        if select_config := config.get(key):
            var = await select.new_select(select_config, options=options)
            cg.add(var.set_parent(parent))
            cg.add(var.set_address(address))
            cg.add(var.set_type(select_type))
            cg.add(parent.set_config_select(address, select_type, var))
