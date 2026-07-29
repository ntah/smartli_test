import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from . import SmartliBms

DEPENDENCIES = ["smartli_bms"]

CONF_SMARTLI_BMS_ID = "smartli_bms_id"
CONF_ADDRESS = "address"
CONF_PCB_BARCODE = "pcb_barcode"
CONF_PACK_BARCODE = "pack_barcode"
CONF_MODBUS_PCB_BARCODE = "modbus_pcb_barcode"
CONF_MODBUS_PACK_BARCODE = "modbus_pack_barcode"
CONF_STATUS = "status"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SMARTLI_BMS_ID): cv.use_id(SmartliBms),
        cv.Required(CONF_ADDRESS): cv.int_range(min=1, max=247),
        cv.Optional(CONF_PCB_BARCODE): text_sensor.text_sensor_schema(
            entity_category="diagnostic",
        ),
        cv.Optional(CONF_PACK_BARCODE): text_sensor.text_sensor_schema(
            entity_category="diagnostic",
        ),
        cv.Optional(CONF_MODBUS_PCB_BARCODE): text_sensor.text_sensor_schema(
            entity_category="diagnostic",
        ),
        cv.Optional(CONF_MODBUS_PACK_BARCODE): text_sensor.text_sensor_schema(
            entity_category="diagnostic",
        ),
        cv.Optional(CONF_STATUS): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SMARTLI_BMS_ID])
    address = config[CONF_ADDRESS]

    if sensor_config := config.get(CONF_PCB_BARCODE):
        sens = await text_sensor.new_text_sensor(sensor_config)
        cg.add(parent.set_pcb_barcode_text_sensor(address, sens))

    if sensor_config := config.get(CONF_PACK_BARCODE):
        sens = await text_sensor.new_text_sensor(sensor_config)
        cg.add(parent.set_pack_barcode_text_sensor(address, sens))

    if sensor_config := config.get(CONF_MODBUS_PCB_BARCODE):
        sens = await text_sensor.new_text_sensor(sensor_config)
        cg.add(parent.set_modbus_pcb_barcode_text_sensor(address, sens))

    if sensor_config := config.get(CONF_MODBUS_PACK_BARCODE):
        sens = await text_sensor.new_text_sensor(sensor_config)
        cg.add(parent.set_modbus_pack_barcode_text_sensor(address, sens))

    if sensor_config := config.get(CONF_STATUS):
        sens = await text_sensor.new_text_sensor(sensor_config)
        cg.add(parent.set_status_text_sensor(address, sens))
