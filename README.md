# ESPHome SmartLi BMS

An ESPHome external component for monitoring and controlling multiple SmartLi
BMS battery packs over one RS485 bus.

## Quick start

1. Connect the ESP32 to an RS485 transceiver.
2. Copy `smartli-example.yaml` to your ESPHome configuration directory.
3. Set the UART and flow-control pins in `substitutions`.
4. Add one entry under `packs` for every connected battery.
5. Add your Wi-Fi, API, and OTA credentials, then compile and install.

```yaml
substitutions:
  name: smartli-bms
  external_components_source: github://ntah/esphome-smartli-bms@main
  tx_pin: GPIO17
  rx_pin: GPIO16
  flow_control_pin: GPIO4

external_components:
  - source: ${external_components_source}
    refresh: 0s

uart:
  id: smartli_uart
  baud_rate: 9600
  tx_pin: ${tx_pin}
  rx_pin: ${rx_pin}
  rx_buffer_size: 512

smartli_bms:
  id: smartli_hub
  uart_id: smartli_uart
  flow_control_pin: ${flow_control_pin}
  update_interval: never
  continuous_polling: true
  dcdc_update_interval: 60s
  response_timeout: 2s
  pack_delay: 2s
  request_delay: 1s

  packs:
    - id: battery_bank1
      address: 1
      sensors:
        pack_voltage:
          name: "Bank 1 Pack Voltage"
        current:
          name: "Bank 1 Current"
        state_of_charge:
          name: "Bank 1 State of Charge"
      text_sensors:
        status:
          name: "Bank 1 Status"
        modbus_address:
          name: "Bank 1 Modbus Address"

    - id: battery_bank2
      address: 2
      sensors:
        pack_voltage:
          name: "Bank 2 Pack Voltage"
        current:
          name: "Bank 2 Current"
        state_of_charge:
          name: "Bank 2 State of Charge"
```

Use the complete [`smartli-example.yaml`](smartli-example.yaml) when you need
all available sensors, selects, barcode fields, and DCDC information.

## Pack addresses

`address` is the SmartLi communication address used for telemetry. Add only the
packs that are physically connected. The polling order follows the order of the
entries under `packs`.

The Modbus slave address is different. When `modbus_address` is not set
manually, the component reads both SmartLi barcodes and matches them against
the Modbus barcode registers to discover the correct slave address.

## Polling settings

- `update_interval`: starts scheduled polling cycles. Use `never` together with
  `continuous_polling: true`.
- `continuous_polling`: starts another complete cycle after all packs finish.
- `dcdc_update_interval`: controls how often DCDC settings and operating data
  are refreshed.
- `response_timeout`: maximum time to wait for a battery response.
- `pack_delay`: delay between two battery packs.
- `request_delay`: delay between different requests to the same pack.
- `flow_control_pin`: controls RS485 transmit/receive direction. It may be
  omitted only when the transceiver controls direction automatically.

## Global BMS mode

SmartLi packs connected in parallel must use the same operating mode. Add the
mode select only once under the `selects` section of any pack:

```yaml
selects:
  # Global control: writes register 0x1016 to every configured pack.
  # Constant writes 0x0101; Battery writes 0x0303.
  mode:
    name: "Mode BMS"
```

The component sends the selected value to every configured pack whose Modbus
address is available. With two configured packs it sends two writes; with five
configured packs it sends five writes.

## Available data

The full example includes:

- Pack, bus, and individual cell voltages
- Current, power, SOC, SOH, and capacity
- Minimum, maximum, average, and delta cell voltage
- Battery, MOS, environment, and balancing temperatures
- Charge/discharge totals and cycle count
- DCDC settings and operating state
- Alarm status, PCB barcode, pack barcode, and discovered Modbus address
- Per-pack DCDC controls and one global BMS mode control

## Troubleshooting

Temporarily set:

```yaml
logger:
  level: DEBUG
  baud_rate: 0
```

Keep `baud_rate: 0` so ESPHome logging does not interfere with the RS485 UART.
