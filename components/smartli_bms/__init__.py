import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@local"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

CONF_ADDRESS = "address"
CONF_FLOW_CONTROL_PIN = "flow_control_pin"

smartli_bms_ns = cg.esphome_ns.namespace("smartli_bms")
SmartliBms = smartli_bms_ns.class_(
    "SmartliBms", cg.PollingComponent, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SmartliBms),
            cv.Optional(CONF_ADDRESS, default=1): cv.int_range(min=1, max=247),
            cv.Optional(CONF_FLOW_CONTROL_PIN): pins.internal_gpio_output_pin_schema,
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_address(config[CONF_ADDRESS]))

    if flow_control_config := config.get(CONF_FLOW_CONTROL_PIN):
        pin = await cg.gpio_pin_expression(flow_control_config)
        cg.add(var.set_flow_control_pin(pin))

