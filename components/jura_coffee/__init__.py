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

SendCommandAction   = jura_coffee_ns.class_("SendCommandAction", automation.Action)
StartDebugAction    = jura_coffee_ns.class_("StartDebugAction", automation.Action)
StopDebugAction     = jura_coffee_ns.class_("StopDebugAction", automation.Action)
AnnotateDebugAction = jura_coffee_ns.class_("AnnotateDebugAction", automation.Action)

CONF_MACHINE_TYPE     = "machine_type"
CONF_IC_TRAY_BIT             = "ic_tray_bit"
CONF_IC_TANK_BIT             = "ic_tank_bit"
CONF_IC_NEED_CLEAN_BIT       = "ic_need_clean_bit"
CONF_IC_NEEDS_RINSE_BIT      = "ic_needs_rinse_bit"
CONF_IC_TRAY_INVERTED        = "ic_tray_inverted"
CONF_IC_TANK_INVERTED        = "ic_tank_inverted"
CONF_IC_NEED_CLEAN_INVERTED  = "ic_need_clean_inverted"
CONF_IC_NEEDS_RINSE_INVERTED = "ic_needs_rinse_inverted"

CONF_TRAY_MISSING        = "tray_missing"
CONF_TANK_EMPTY          = "tank_empty"
CONF_NEED_CLEAN          = "need_clean"
CONF_READY               = "ready"
CONF_NEEDS_RINSE         = "needs_rinse"
CONF_NUM_SINGLE_ESPRESSO = "num_single_espresso"
CONF_NUM_DOUBLE_ESPRESSO = "num_double_espresso"
CONF_NUM_COFFEE          = "num_coffee"
CONF_NUM_DOUBLE_COFFEE   = "num_double_coffee"
CONF_NUM_CLEAN               = "num_clean"
CONF_NUM_RINSE               = "num_rinse"
CONF_NUM_DESCALE             = "num_descale"
CONF_NUM_COFFEES_SINCE_CLEAN = "num_coffees_since_clean"
CONF_LAST_RESPONSE           = "last_response"

# IC: bit profiles per known machine type.
# ic_tray_bit / ic_tank_bit / ic_need_clean_bit: bit position in IC: byte 0.
# ic_tray_inverted: True when bit=1 means tray PRESENT (F50 quirk).
#
# Sources: community reverse-engineering + NotebookLM analysis of 15 reference projects.
# Confirmed = verified with real hardware. Unconfirmed = from code/docs analysis only.
MACHINE_PROFILES = {
    # ── Impressa F50 ─────────────────────────────────────────────────────────
    # Confirmed from hardware samples (F50.md):
    #   0xDF=normal, 0xD7=tank missing (bit3→0), 0xCF=tray missing (bit4→0),
    #   0xD9=clean needed (bit1→0). All F50 status bits: 0=problem, 1=ok.
    "f50": {
        CONF_IC_TRAY_BIT:            4,
        CONF_IC_TANK_BIT:            3,     # confirmed: bit3=0 → tank empty
        CONF_IC_NEED_CLEAN_BIT:      1,     # confirmed: bit1=0 → clean needed
        CONF_IC_NEEDS_RINSE_BIT:     0,     # confirmed: bit0=1 → needs rinse on power off
        CONF_IC_TRAY_INVERTED:       True,
        CONF_IC_TANK_INVERTED:       True,
        CONF_IC_NEED_CLEAN_INVERTED: True,
        CONF_IC_NEEDS_RINSE_INVERTED: False,
    },
    # ── E-series (E6, E8, E65) ───────────────────────────────────────────────
    # Confirmed from multiple sources: bit0=tray missing, bit1=tank empty.
    "e6": {
        CONF_IC_TRAY_BIT:       0,
        CONF_IC_TANK_BIT:       1,
        CONF_IC_NEED_CLEAN_BIT: 2,   # unconfirmed — verify with IC: logs
        CONF_IC_TRAY_INVERTED:  False,
    },
    # ── Impressa J6 ──────────────────────────────────────────────────────────
    # Assumed same IC: layout as E6. FA: commands differ significantly from F50.
    "j6": {
        CONF_IC_TRAY_BIT:       0,
        CONF_IC_TANK_BIT:       1,
        CONF_IC_NEED_CLEAN_BIT: 2,   # unconfirmed — verify with IC: logs
        CONF_IC_TRAY_INVERTED:  False,
    },
    # ── Impressa F7 / F8 / S95 / S90 ─────────────────────────────────────────
    # Based on F-series code analysis. Likely same as E6 (bit0/bit1).
    "f7": {
        CONF_IC_TRAY_BIT:       0,
        CONF_IC_TANK_BIT:       1,
        CONF_IC_NEED_CLEAN_BIT: 2,   # unconfirmed
        CONF_IC_TRAY_INVERTED:  False,
    },
    "s95": {
        CONF_IC_TRAY_BIT:       0,
        CONF_IC_TANK_BIT:       1,
        CONF_IC_NEED_CLEAN_BIT: 2,   # unconfirmed
        CONF_IC_TRAY_INVERTED:  False,
    },
    # ── ENA series (ENA 5, ENA 7, ENA Micro 90) ──────────────────────────────
    # Confirmed: bit0=tray, bit1=tank (same pattern as E6).
    # Note: ENA Micro 90 service port sleeps a few minutes after power-off.
    "ena": {
        CONF_IC_TRAY_BIT:       0,
        CONF_IC_TANK_BIT:       1,
        CONF_IC_NEED_CLEAN_BIT: 2,   # unconfirmed
        CONF_IC_TRAY_INVERTED:  False,
    },
    # ── X7 / Franke Saphira ───────────────────────────────────────────────────
    # Unconfirmed. Professional model with additional hardware (dual grinders,
    # cappuccino valve). IC: bit layout unknown — verify with diagnostic logs.
    "x7": {
        CONF_IC_TRAY_BIT:       4,
        CONF_IC_TANK_BIT:       5,
        CONF_IC_NEED_CLEAN_BIT: 0,
        CONF_IC_TRAY_INVERTED:  False,
    },
}

MACHINE_TYPES = list(MACHINE_PROFILES.keys())

# Global IC: defaults used when no machine_type and no explicit ic_* keys are set.
IC_DEFAULTS = {
    CONF_IC_TRAY_BIT:            4,
    CONF_IC_TANK_BIT:            5,
    CONF_IC_NEED_CLEAN_BIT:      0,
    CONF_IC_NEEDS_RINSE_BIT:     0,
    CONF_IC_TRAY_INVERTED:       False,
    CONF_IC_TANK_INVERTED:       False,
    CONF_IC_NEED_CLEAN_INVERTED: False,
    CONF_IC_NEEDS_RINSE_INVERTED: False,
}


def _apply_machine_profile(config):
    """Merge machine profile into config, explicit ic_* keys take precedence."""
    profile = MACHINE_PROFILES.get(config.get(CONF_MACHINE_TYPE, ""), {})
    merged = {**IC_DEFAULTS, **profile}
    for key, value in merged.items():
        if key not in config:
            config[key] = value
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(JuraCoffee),
            cv.Optional(CONF_MACHINE_TYPE): cv.one_of(*MACHINE_TYPES, lower=True),
            # IC: bit positions — set automatically by machine_type, or override manually
            cv.Optional(CONF_IC_TRAY_BIT):            cv.int_range(min=0, max=7),
            cv.Optional(CONF_IC_TANK_BIT):            cv.int_range(min=0, max=7),
            cv.Optional(CONF_IC_NEED_CLEAN_BIT):      cv.int_range(min=0, max=7),
            cv.Optional(CONF_IC_NEEDS_RINSE_BIT):     cv.int_range(min=0, max=7),
            cv.Optional(CONF_IC_TRAY_INVERTED):       cv.boolean,
            cv.Optional(CONF_IC_TANK_INVERTED):       cv.boolean,
            cv.Optional(CONF_IC_NEED_CLEAN_INVERTED): cv.boolean,
            cv.Optional(CONF_IC_NEEDS_RINSE_INVERTED): cv.boolean,
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
            cv.Optional(CONF_READY): binary_sensor.binary_sensor_schema(
                icon="mdi:coffee-maker-check",
            ),
            cv.Optional(CONF_NEEDS_RINSE): binary_sensor.binary_sensor_schema(
                icon="mdi:water-sync",
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
            cv.Optional(CONF_NUM_RINSE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:water-sync",
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_NUM_DESCALE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:water-minus",
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_NUM_COFFEES_SINCE_CLEAN): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:counter",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_LAST_RESPONSE): text_sensor.text_sensor_schema(
                icon="mdi:console",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA),
    _apply_machine_profile,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_ic_tray_bit(config[CONF_IC_TRAY_BIT]))
    cg.add(var.set_ic_tank_bit(config[CONF_IC_TANK_BIT]))
    cg.add(var.set_ic_need_clean_bit(config[CONF_IC_NEED_CLEAN_BIT]))
    cg.add(var.set_ic_needs_rinse_bit(config[CONF_IC_NEEDS_RINSE_BIT]))
    cg.add(var.set_ic_tray_inverted(config[CONF_IC_TRAY_INVERTED]))
    cg.add(var.set_ic_tank_inverted(config[CONF_IC_TANK_INVERTED]))
    cg.add(var.set_ic_need_clean_inverted(config[CONF_IC_NEED_CLEAN_INVERTED]))
    cg.add(var.set_ic_needs_rinse_inverted(config[CONF_IC_NEEDS_RINSE_INVERTED]))

    for key, setter in [
        (CONF_TRAY_MISSING, "set_tray_missing_sensor"),
        (CONF_TANK_EMPTY, "set_tank_empty_sensor"),
        (CONF_NEED_CLEAN, "set_need_clean_sensor"),
        (CONF_READY, "set_ready_sensor"),
        (CONF_NEEDS_RINSE, "set_needs_rinse_sensor"),
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
        (CONF_NUM_RINSE, "set_num_rinse_sensor"),
        (CONF_NUM_DESCALE, "set_num_descale_sensor"),
        (CONF_NUM_COFFEES_SINCE_CLEAN, "set_num_coffees_since_clean_sensor"),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))

    if CONF_LAST_RESPONSE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_RESPONSE])
        cg.add(var.set_last_response_sensor(sens))


# ── Actions ───────────────────────────────────────────────────────────────────

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


JURA_COFFEE_START_DEBUG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(JuraCoffee),
        cv.Optional("rr_start", default=0x00): cv.int_range(min=0, max=255),
        cv.Optional("rr_end", default=0x23):   cv.int_range(min=0, max=255),
        cv.Optional("interval_ms", default=5000): cv.positive_int,
        cv.Optional("poll_ic", default=True):  cv.boolean,
        cv.Optional("poll_rt", default=True):  cv.boolean,
    }
)


@automation.register_action(
    "jura_coffee.start_debug", StartDebugAction, JURA_COFFEE_START_DEBUG_SCHEMA,
    synchronous=True,
)
async def start_debug_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_rr_start(config["rr_start"]))
    cg.add(var.set_rr_end(config["rr_end"]))
    cg.add(var.set_interval_ms(config["interval_ms"]))
    cg.add(var.set_poll_ic(config["poll_ic"]))
    cg.add(var.set_poll_rt(config["poll_rt"]))
    return var


JURA_COFFEE_STOP_DEBUG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(JuraCoffee),
    }
)


@automation.register_action(
    "jura_coffee.stop_debug", StopDebugAction, JURA_COFFEE_STOP_DEBUG_SCHEMA,
    synchronous=True,
)
async def stop_debug_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


JURA_COFFEE_ANNOTATE_DEBUG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(JuraCoffee),
        cv.Required("message"): cv.templatable(cv.string),
    }
)


@automation.register_action(
    "jura_coffee.annotate_debug", AnnotateDebugAction, JURA_COFFEE_ANNOTATE_DEBUG_SCHEMA,
    synchronous=True,
)
async def annotate_debug_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config["message"], args, cg.std_string)
    cg.add(var.set_message(templ))
    return var
