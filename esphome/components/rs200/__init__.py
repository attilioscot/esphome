import esphome.codegen as cg
from esphome.components import uart

CODEOWNERS = ["@attilioscot"]
DEPENDENCIES = ["uart"]

rs200_ns = cg.esphome_ns.namespace("rs200")
RS200Component = rs200_ns.class_("RS200Component", cg.PollingComponent, uart.UARTDevice)
