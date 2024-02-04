import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
    CONF_ID,
    UNIT_PERCENT,
    ICON_WATER_PERCENT,
    ICON_WATER,
    CONF_TEMPERATURE,
    DEVICE_CLASS_PRECIPITATION_INTENSITY,
    STATE_CLASS_MEASUREMENT,
)

from . import RS200Component

UNIT_INTENSITY = "intensity"
UNIT_MILLIMETERS = "mm"
UNIT_MILLIMETERS_PER_HOUR = "mm/h"

CONF_R_INT = "rain_intensity"
CONF_R_RAW = "rain_raw"
CONF_DISABLE_LED = "disable_led"


def _validate(config):
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RS200Component),
            cv.Optional(CONF_R_INT): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_WATER_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_PRECIPITATION_INTENSITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_R_RAW): sensor.sensor_schema(
                icon=ICON_WATER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_PRECIPITATION_INTENSITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DISABLE_LED): cv.boolean,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA),
    _validate,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    sens = await sensor.new_sensor(config[CONF_R_INT])
    cg.add(var.set_rain(sens))

    if CONF_TEMPERATURE in config:
        cg.add(var.set_request_temperature(CONF_TEMPERATURE in config))

    if CONF_R_RAW in config:
        cg.add(var.set_realtime_rain(CONF_R_RAW in config))

    if CONF_DISABLE_LED in config:
        cg.add(var.set_disable_led(config[CONF_DISABLE_LED]))
