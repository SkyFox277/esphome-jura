import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart, sensor, text_sensor, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    UNIT_EMPTY,
    ICON_EMPTY,
    STATE_CLASS_TOTAL_INCREASING,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor"]

jura_coffee_ns = cg.esphome_ns.namespace("jura_coffee")
JuraCoffee = jura_coffee_ns.class_("JuraCoffee", cg.PollingComponent, uart.UARTDevice)

SendCommandAction = jura_coffee_ns.class_("SendCommandAction", automation.Action)

CONF_IC_TRAY_BIT = "ic_tray_bit"
CONF_IC_TANK_BIT = "ic_tank_bit"
CONF_IC_NEED_CLEAN_BIT = "ic_need_clean_bit"
CONF_IC_TRAY_INVERTED = "ic_tray_inverted"

CONF_TRAY_MISSING = "tray_missing"
CONF_TANK_EMPTY = "tank_empty"
CONF_NEED_CLEAN = "need_clean"
CONF_NUM_SINGLE_ESPRESSO = "num_single_espresso"
CONF_NUM_DOUBLE_ESPRESSO = "num_double_espresso"
CONF_NUM_COFFEE = "num_coffee"
CONF_NUM_DOUBLE_COFFEE = "num_double_coffee"
CONF_NUM_CLEAN = "num_clean"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(JuraCoffee),
            # IC: byte bit positions — adjust for non-standard models
            cv.Optional(CONF_IC_TRAY_BIT, default=4): cv.int_range(min=0, max=7),
            cv.Optional(CONF_IC_TANK_BIT, default=5): cv.int_range(min=0, max=7),
            cv.Optional(CONF_IC_NEED_CLEAN_BIT, default=0): cv.int_range(min=0, max=7),
            # Set true when bit=1 means tray is PRESENT (e.g. Impressa F50)
            cv.Optional(CONF_IC_TRAY_INVERTED, default=False): cv.boolean,
            cv.Optional(CONF_TRAY_MISSING): binary_sensor.binary_sensor_schema(
                icon="mdi:tray-alert",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_TANK_EMPTY): binary_sensor.binary_sensor_schema(
                icon="mdi:water-alert",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_NEED_CLEAN): binary_sensor.binary_sensor_schema(
                icon="mdi:broom",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_NUM_SINGLE_ESPRESSO): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:coffee-outline",
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_NUM_DOUBLE_ESPRESSO): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:coffee-outline",
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_NUM_COFFEE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:coffee",
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_NUM_DOUBLE_COFFEE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:coffee",
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_NUM_CLEAN): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:broom",
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_ic_tray_bit(config[CONF_IC_TRAY_BIT]))
    cg.add(var.set_ic_tank_bit(config[CONF_IC_TANK_BIT]))
    cg.add(var.set_ic_need_clean_bit(config[CONF_IC_NEED_CLEAN_BIT]))
    cg.add(var.set_ic_tray_inverted(config[CONF_IC_TRAY_INVERTED]))

    for key, setter in [
        (CONF_TRAY_MISSING, "set_tray_missing_sensor"),
        (CONF_TANK_EMPTY, "set_tank_empty_sensor"),
        (CONF_NEED_CLEAN, "set_need_clean_sensor"),
    ]:
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(var, setter)(sens))

    for key, setter in [
        (CONF_NUM_SINGLE_ESPRESSO, "set_num_single_espresso_sensor"),
        (CONF_NUM_DOUBLE_ESPRESSO, "set_num_double_espresso_sensor"),
        (CONF_NUM_COFFEE, "set_num_coffee_sensor"),
        (CONF_NUM_DOUBLE_COFFEE, "set_num_double_coffee_sensor"),
        (CONF_NUM_CLEAN, "set_num_clean_sensor"),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))


JURA_COFFEE_SEND_COMMAND_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(JuraCoffee),
        cv.Required("command"): cv.templatable(cv.string),
    }
)


@automation.register_action(
    "jura_coffee.send_command", SendCommandAction, JURA_COFFEE_SEND_COMMAND_SCHEMA,
    synchronous=True,
)
async def send_command_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config["command"], args, cg.std_string)
    cg.add(var.set_command(templ))
    return var
