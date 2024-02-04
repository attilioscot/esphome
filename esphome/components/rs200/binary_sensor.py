import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, DEVICE_CLASS_PROBLEM, CONF_MOISTURE

from . import rs200_ns, RS200Component

CONF_RS200_ID = "rs200_id"
CONF_LENS_BAD = "lens_bad"
CONF_R_SENSOR = "raining"

RS200BinarySensor = rs200_ns.class_("RS200BinaryComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RS200BinarySensor),
        cv.GenerateID(CONF_RS200_ID): cv.use_id(RS200Component),
        cv.Optional(CONF_LENS_BAD): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
        ),
        cv.Optional(CONF_R_SENSOR): binary_sensor.binary_sensor_schema(
            device_class=CONF_MOISTURE
        ),
    }
)


async def to_code(config):
    main_sensor = await cg.get_variable(config[CONF_RS200_ID])
    bin_component = cg.new_Pvariable(config[CONF_ID], main_sensor)
    await cg.register_component(bin_component, config)

    mapping = {
        CONF_LENS_BAD: main_sensor.set_lens_bad_sensor,
        CONF_R_SENSOR: main_sensor.set_raining_sensor,
    }

    for key, value in mapping.items():
        if key in config:
            sensor = await binary_sensor.new_binary_sensor(config[key])
            cg.add(value(sensor))
