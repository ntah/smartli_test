import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import SmartliBmsPackConfig, smartli_bms_ns

DEPENDENCIES = ["smartli_bms"]

CONF_SMARTLI_BMS_ID = "smartli_bms_id"

SmartliBmsSelect = smartli_bms_ns.class_("SmartliBmsSelect", select.Select)
SmartliSelectType = smartli_bms_ns.enum("SmartliSelectType")

SELECTS = {
    "vbus_discharge_set": (SmartliSelectType.VBUS_DISCHARGE, ["49.0V", "49.5V", "50.0V", "50.5V", "51.0V"]),
    "vbus_dod": (SmartliSelectType.VBUS_DOD, [f"{v / 10:.1f}V" for v in range(480, 501)]),
    "ibus_percent": (SmartliSelectType.IBUS_PERCENT, [f"{v}%" for v in range(0, 101, 5)]),
    "pbus_percent": (SmartliSelectType.PBUS_PERCENT, ["0%", "3%"] + [f"{v}%" for v in range(5, 101, 5)]),
    "avg_charge_percent": (SmartliSelectType.AVG_CHARGE_PERCENT, [f"{v}%" for v in range(0, 101, 5)]),
    "dod_percent": (SmartliSelectType.DOD_PERCENT, [f"{v}%" for v in range(0, 101, 5)]),
    "charging_loop": (SmartliSelectType.CHARGING_LOOP, ["Enable", "Disable"]),
    "discharge_loop": (SmartliSelectType.DISCHARGE_LOOP, ["Enable", "Disable"]),
    "mode": (SmartliSelectType.MODE_ALL, ["Constant", "Battery"]),
}

SELECT_ENTITY_SCHEMA = cv.Schema({
    **{
        cv.Optional(key): select.select_schema(
            SmartliBmsSelect, entity_category=ENTITY_CATEGORY_CONFIG
        )
        for key in SELECTS
    },
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_SMARTLI_BMS_ID): cv.use_id(SmartliBmsPackConfig),
}).extend(SELECT_ENTITY_SCHEMA)


async def register_selects(config, pack):
    parent = pack.get_parent()
    address = pack.get_address()
    for key, (select_type, options) in SELECTS.items():
        if select_config := config.get(key):
            var = await select.new_select(select_config, options=options)
            cg.add(var.set_parent(parent))
            cg.add(var.set_address(address))
            cg.add(var.set_type(select_type))
            cg.add(parent.set_config_select(address, select_type, var))


async def to_code(config):
    pack = await cg.get_variable(config[CONF_SMARTLI_BMS_ID])
    await register_selects(config, pack)
