import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@local"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "select", "text_sensor"]
MULTI_CONF = False

CONF_PACKS = "packs"
CONF_ADDRESS = "address"
CONF_MODBUS_ADDRESS = "modbus_address"
CONF_FLOW_CONTROL_PIN = "flow_control_pin"
CONF_DCDC_UPDATE_INTERVAL = "dcdc_update_interval"
CONF_RESPONSE_TIMEOUT = "response_timeout"
CONF_PACK_DELAY = "pack_delay"
CONF_REQUEST_DELAY = "request_delay"
CONF_CONTINUOUS_POLLING = "continuous_polling"
CONF_SENSORS = "sensors"
CONF_SELECTS = "selects"
CONF_TEXT_SENSORS = "text_sensors"

smartli_bms_ns = cg.esphome_ns.namespace("smartli_bms")
SmartliBms = smartli_bms_ns.class_(
    "SmartliBms", cg.PollingComponent, uart.UARTDevice
)
SmartliBmsPackConfig = smartli_bms_ns.class_("SmartliBmsPackConfig")

from .sensor import SENSOR_ENTITY_SCHEMA, register_sensors
from .select import SELECT_ENTITY_SCHEMA, register_selects
from .text_sensor import TEXT_SENSOR_ENTITY_SCHEMA, register_text_sensors

PACK_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SmartliBmsPackConfig),
        cv.Required(CONF_ADDRESS): cv.int_range(min=1, max=247),
        cv.Optional(CONF_MODBUS_ADDRESS, default=0): cv.int_range(min=0, max=247),
        cv.Optional(CONF_SENSORS): SENSOR_ENTITY_SCHEMA,
        cv.Optional(CONF_SELECTS): SELECT_ENTITY_SCHEMA,
        cv.Optional(CONF_TEXT_SENSORS): TEXT_SENSOR_ENTITY_SCHEMA,
    }
)


def validate_unique_packs(config):
    addresses = [pack[CONF_ADDRESS] for pack in config[CONF_PACKS]]
    if len(addresses) != len(set(addresses)):
        raise cv.Invalid("Pack addresses must be unique")
    manual = [
        pack[CONF_MODBUS_ADDRESS]
        for pack in config[CONF_PACKS]
        if pack[CONF_MODBUS_ADDRESS] != 0
    ]
    if len(manual) != len(set(manual)):
        raise cv.Invalid("Configured Modbus addresses must be unique")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SmartliBms),
            cv.Required(CONF_PACKS): cv.All(
                cv.ensure_list(PACK_SCHEMA), cv.Length(min=1, max=8)
            ),
            cv.Optional(CONF_FLOW_CONTROL_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(
                CONF_DCDC_UPDATE_INTERVAL, default="60s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_RESPONSE_TIMEOUT, default="700ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_PACK_DELAY, default="2s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_REQUEST_DELAY, default="1s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_CONTINUOUS_POLLING, default=False): cv.boolean,
        }
    )
    .extend(cv.polling_component_schema("30s"))
    .extend(uart.UART_DEVICE_SCHEMA),
    validate_unique_packs,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(
        var.set_dcdc_update_interval(
            config[CONF_DCDC_UPDATE_INTERVAL].total_milliseconds
        )
    )
    cg.add(
        var.set_response_timeout(config[CONF_RESPONSE_TIMEOUT].total_milliseconds)
    )
    cg.add(var.set_pack_delay(config[CONF_PACK_DELAY].total_milliseconds))
    cg.add(var.set_request_delay(config[CONF_REQUEST_DELAY].total_milliseconds))
    cg.add(var.set_continuous_polling(config[CONF_CONTINUOUS_POLLING]))
    for pack in config[CONF_PACKS]:
        pack_var = cg.new_Pvariable(pack[CONF_ID])
        cg.add(pack_var.set_parent(var))
        cg.add(pack_var.set_address(pack[CONF_ADDRESS]))
        cg.add(var.add_pack(pack[CONF_ADDRESS], pack[CONF_MODBUS_ADDRESS]))
        if sensor_config := pack.get(CONF_SENSORS):
            await register_sensors(sensor_config, pack_var)
        if select_config := pack.get(CONF_SELECTS):
            await register_selects(select_config, pack_var)
        if text_sensor_config := pack.get(CONF_TEXT_SENSORS):
            await register_text_sensors(text_sensor_config, pack_var)

    if flow_control_config := config.get(CONF_FLOW_CONTROL_PIN):
        pin = await cg.gpio_pin_expression(flow_control_config)
        cg.add(var.set_flow_control_pin(pin))
