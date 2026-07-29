import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@local"]
DEPENDENCIES = ["uart"]
MULTI_CONF = False

CONF_PACKS = "packs"
CONF_ADDRESS = "address"
CONF_MODBUS_ADDRESS = "modbus_address"
CONF_FLOW_CONTROL_PIN = "flow_control_pin"
CONF_DCDC_UPDATE_INTERVAL = "dcdc_update_interval"
CONF_RESPONSE_TIMEOUT = "response_timeout"

smartli_bms_ns = cg.esphome_ns.namespace("smartli_bms")
SmartliBms = smartli_bms_ns.class_(
    "SmartliBms", cg.PollingComponent, uart.UARTDevice
)

PACK_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ADDRESS): cv.int_range(min=1, max=247),
        cv.Required(CONF_MODBUS_ADDRESS): cv.int_range(min=1, max=247),
    }
)


def validate_unique_packs(config):
    addresses = [pack[CONF_ADDRESS] for pack in config[CONF_PACKS]]
    if len(addresses) != len(set(addresses)):
        raise cv.Invalid("Pack addresses must be unique")
    manual = [
        pack[CONF_MODBUS_ADDRESS]
        for pack in config[CONF_PACKS]
        if CONF_MODBUS_ADDRESS in pack
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
    for pack in config[CONF_PACKS]:
        cg.add(var.add_pack(pack[CONF_ADDRESS], pack[CONF_MODBUS_ADDRESS]))

    if flow_control_config := config.get(CONF_FLOW_CONTROL_PIN):
        pin = await cg.gpio_pin_expression(flow_control_config)
        cg.add(var.set_flow_control_pin(pin))
