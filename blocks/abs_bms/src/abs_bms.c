#include "abs_bms.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../../utils/counter.h"
#include "can/abs_bms_nf.h"

/* sets command values from system state and sends command message to bms */
static void abs_bms_app_command_send(const struct abs_bms_data_t* data);

/* deserialize raw bytes in to DBC defined structured data */
static void abs_bms_nf_batt_state_deserialize(struct abs_bms_data_t* dst, const uint8_t* src);
static void abs_bms_nf_batt_sys_temp_module_deserialize(struct abs_bms_data_t* dst,
                                                        const uint8_t* src);
static void abs_bms_nf_batt_sys_sohc_deserialize(struct abs_bms_data_t* dst, const uint8_t* src);
static void abs_bms_nf_batt_sys_soc_deserialize(struct abs_bms_data_t* dst, const uint8_t* src);
static void abs_bms_nf_batt_sys_volt_cell_deserialize(struct abs_bms_data_t* dst,
                                                      const uint8_t* src);
static void abs_bms_nf_batt_limits_charger_deserialize(struct abs_bms_data_t* dst,
                                                       const uint8_t* src);
static void abs_bms_nf_batt_limits_discharge_deserialize(struct abs_bms_data_t* dst,
                                                         const uint8_t* src);
static void abs_bms_nf_batt_current_deserialize(struct abs_bms_data_t* dst, const uint8_t* src);
static void abs_bms_nf_batt_sys_volt_string_deserialize(struct abs_bms_data_t* dst,
                                                        const uint8_t* src);
static void abs_bms_nf_batt_iso_res_frame_deserialize(struct abs_bms_data_t* dst,
                                                      const uint8_t* src);

static const uint8_t toc_soc = 91u;
static const uint8_t boc_soc = 5u;
static float abs_bms_user_soc_scale(const float soc);

void abs_bms_init(struct abs_bms_data_t* data) {
    memset(data, 0, sizeof(struct abs_bms_data_t));

    // We are timed out until we get a message
    data->outputs.timeout = true;
    data->internal.can_tick_counter = (data->config.ticks_per_s * 2) + 1;

    // active by default - user control not enabled.
    data->outputs.isolation_monitor_enabled = true;
}

void abs_bms_parse_can(uint32_t can_id, uint8_t* buf, struct abs_bms_data_t* data) {
    switch (can_id) {
        case ABS_BMS_NF_BATT_STATE_FRAME_ID:
            abs_bms_nf_batt_state_deserialize(data, buf);
            data->internal.can_tick_counter = 0;
            break;

        case ABS_BMS_NF_BATT_SYS_TEMP_MODULE_FRAME_ID:
            abs_bms_nf_batt_sys_temp_module_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_SYS_SOHC_FRAME_ID:
            abs_bms_nf_batt_sys_sohc_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_SYS_SOC_FRAME_ID:
            abs_bms_nf_batt_sys_soc_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_SYS_VOLT_CELL_FRAME_ID:
            abs_bms_nf_batt_sys_volt_cell_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_LIMITS_CHARGER_FRAME_ID:
            abs_bms_nf_batt_limits_charger_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_LIMITS_DISCHARGE_FRAME_ID:
            abs_bms_nf_batt_limits_discharge_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_CURRENT_FRAME_ID:
            abs_bms_nf_batt_current_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_SYS_VOLT_STRING_FRAME_ID:
            abs_bms_nf_batt_sys_volt_string_deserialize(data, buf);
            break;

        case ABS_BMS_NF_BATT_ISO_RES_FRAME_ID:
            abs_bms_nf_batt_iso_res_frame_deserialize(data, buf);
            break;

        default:
            break;
    }
}

void abs_bms_tick(struct abs_bms_data_t* data) {
    abs_bms_app_command_send(data);
    
    counter_inc(&(data->internal.can_tick_counter));
    data->internal.startup_counter++;
    data->outputs.timeout = false;

    // Reset the data if the OBC has timed out
    if (data->internal.can_tick_counter > (data->config.ticks_per_s * 2)) {
        data->outputs.max_charge_current = 0;
        data->outputs.max_charge_voltage = 0;
        data->outputs.pack_voltage = 0;
        data->outputs.pack_current = 0;
        data->outputs.max_discharge_current = 0;
        data->outputs.soh = 0;
        data->outputs.soc = 0;
        data->outputs.energy_remain = 0;
        data->outputs.temp_inlet = 0;
        data->outputs.temp_outlet = 0;
        data->outputs.pressure_inlet = 0;
        data->outputs.isolation = 0;
        data->outputs.min_cell_temp = 0;
        data->outputs.max_cell_temp = 0;
        data->outputs.state_ready = 0;
        data->outputs.state_drive = 0;
        data->outputs.state_charge = 0;
        data->outputs.state_disconnect = 0;
        data->outputs.bms_hvil_closed = false;
        data->outputs.timeout = true;
    } else {
        data->outputs.timeout = false;
    }
}

static bool quiet;
void abs_bms_app_command_send(const struct abs_bms_data_t* data) {
    uint8_t buffer[ABS_BMS_NF_APP_COMMAND_LENGTH] = {0};
    struct abs_bms_nf_app_command_t bms_command;
    abs_bms_nf_app_command_init(&bms_command);

    if (data->inputs.sleep_cmd) {
        bms_command.app_state_req_mode = ABS_BMS_NF_APP_COMMAND_APP_STATE_REQ_MODE_OFF_CHOICE;
    } else if (data->inputs.charge_cmd) {
        bms_command.app_state_req_mode = ABS_BMS_NF_APP_COMMAND_APP_STATE_REQ_MODE_CHARGE_CHOICE;
        quiet = false;
    } else if (data->inputs.run_cmd) {
        bms_command.app_state_req_mode = ABS_BMS_NF_APP_COMMAND_APP_STATE_REQ_MODE_DISCHARGE_CHOICE;
        quiet = false;
    } else {
        bms_command.app_state_req_mode = ABS_BMS_NF_APP_COMMAND_APP_STATE_REQ_MODE_IDLE_CHOICE;
        quiet = false;
    }

    bms_command.app_comm_rolling_count = data->internal.can_tick_counter;
    bms_command.app_balancing_enable_f = data->inputs.balance_cmd;
    bms_command.app_contactor_command = (data->inputs.charge_cmd || data->inputs.run_cmd);

    abs_bms_nf_app_command_pack(buffer, &bms_command, ABS_BMS_NF_APP_COMMAND_LENGTH);
    if (!quiet)
    {
        data->config.can_send(ABS_BMS_NF_APP_COMMAND_FRAME_ID, ABS_BMS_NF_APP_COMMAND_IS_EXTENDED, ABS_BMS_NF_APP_COMMAND_LENGTH, buffer);
    }

     if (data->inputs.sleep_cmd)
     {
        quiet = true;
    }
}

void abs_bms_nf_batt_state_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_state_t state;
    abs_bms_nf_batt_state_init(&state);
    abs_bms_nf_batt_state_unpack(&state, src, ABS_BMS_NF_BATT_STATE_LENGTH);

    switch (state.batt_sys_state)
    {
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_STAND_BY_READY_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_CONNECT_DRIVE_PRECHARGE_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_CONNECT_DRIVE_BUS_JOIN_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_CONNECT_DRIVE_STAGGER_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_CONNECT_CHARGE_PRECHARGE_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_CONNECT_CHARGE_BUS_JOIN_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_CONNECT_CHARGE_STAGGER_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_EXTERNAL_ISOLATION_TEST_DRIVE_CHOICE:
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_EXTERNAL_ISOLATION_TEST_CHARGE_CHOICE:
            dst->outputs.state_ready = true;
            dst->outputs.state_drive = false;
            dst->outputs.state_charge = false;
            dst->outputs.state_disconnect = false;
            break;
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_DRIVE_CHOICE:
            dst->outputs.state_ready = false;
            dst->outputs.state_drive = true;
            dst->outputs.state_charge = false;
            dst->outputs.state_disconnect = false;
            break;
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_CHARGE_CHOICE:
            dst->outputs.state_ready = false;
            dst->outputs.state_drive = false;
            dst->outputs.state_charge = true;
            dst->outputs.state_disconnect = false;
            break;
        case ABS_BMS_NF_BATT_STATE_BATT_SYS_STATE_DISCONNECT_CHOICE:
            dst->outputs.state_ready = false;
            dst->outputs.state_drive = false;
            dst->outputs.state_charge = false;
            dst->outputs.state_disconnect = true;
            break;
        default:
            dst->outputs.state_ready = false;
            dst->outputs.state_drive = false;
            dst->outputs.state_charge = false;
            dst->outputs.state_disconnect = false;
            break;
    }

    dst->outputs.bms_hvil_closed =
        ABS_BMS_NF_BATT_STATE_BATT_HVIL_STATUS_VALID_CHOICE == state.batt_hvil_status;
}

void abs_bms_nf_batt_sys_temp_module_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_sys_temp_module_t state;
    abs_bms_nf_batt_sys_temp_module_init(&state);
    abs_bms_nf_batt_sys_temp_module_unpack(&state, src, ABS_BMS_NF_BATT_SYS_TEMP_MODULE_LENGTH);
    dst->outputs.min_cell_temp = abs_bms_nf_batt_sys_temp_module_batt_sys_temp_module_min_decode(
        state.batt_sys_temp_module_min);
    dst->outputs.max_cell_temp = abs_bms_nf_batt_sys_temp_module_batt_sys_temp_module_max_decode(
        state.batt_sys_temp_module_max);
}

void abs_bms_nf_batt_sys_sohc_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_sys_sohc_t state;
    abs_bms_nf_batt_sys_sohc_init(&state);
    abs_bms_nf_batt_sys_sohc_unpack(&state, src, ABS_BMS_NF_BATT_SYS_SOHC_LENGTH);
    dst->outputs.soh =
        abs_bms_nf_batt_sys_sohc_batt_sys_sohc_module_avg_decode(state.batt_sys_sohc_module_avg);
}

void abs_bms_nf_batt_sys_soc_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_sys_soc_t state;
    float soc;

    abs_bms_nf_batt_sys_soc_init(&state);
    abs_bms_nf_batt_sys_soc_unpack(&state, src, ABS_BMS_NF_BATT_SYS_SOC_LENGTH);
    soc = abs_bms_nf_batt_sys_soc_batt_sys_soc_string_avg_decode(state.batt_sys_soc_string_avg);
    dst->outputs.soc = abs_bms_user_soc_scale(soc);
    dst->outputs.energy_remain =
        abs_bms_nf_batt_sys_soc_batt_sys_energy_system_decode(state.batt_sys_energy_system);
}

void abs_bms_nf_batt_sys_volt_cell_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_sys_volt_cell_t state;
    abs_bms_nf_batt_sys_volt_cell_init(&state);
    abs_bms_nf_batt_sys_volt_cell_unpack(&state, src, ABS_BMS_NF_BATT_SYS_VOLT_CELL_LENGTH);
    dst->outputs.max_cell_voltage =
        abs_bms_nf_batt_sys_volt_cell_batt_sys_volt_cell_max_decode(state.batt_sys_volt_cell_max);
    dst->outputs.min_cell_voltage =
        abs_bms_nf_batt_sys_volt_cell_batt_sys_volt_cell_min_decode(state.batt_sys_volt_cell_min);
}

void abs_bms_nf_batt_limits_charger_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_limits_charger_t state;
    abs_bms_nf_batt_limits_charger_init(&state);
    abs_bms_nf_batt_limits_charger_unpack(&state, src, ABS_BMS_NF_BATT_LIMITS_CHARGER_LENGTH);
    dst->outputs.max_charge_current =
        abs_bms_nf_batt_limits_charger_batt_limit_chrgr_crrnt_decode(state.batt_limit_chrgr_crrnt);
    dst->outputs.max_charge_voltage =
        abs_bms_nf_batt_limits_charger_batt_limit_chrgr_volt_decode(state.batt_limit_chrgr_volt);
}

void abs_bms_nf_batt_limits_discharge_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_limits_discharge_t state;
    abs_bms_nf_batt_limits_discharge_init(&state);
    abs_bms_nf_batt_limits_discharge_unpack(&state, src, ABS_BMS_NF_BATT_LIMITS_DISCHARGE_LENGTH);
    dst->outputs.max_discharge_current = abs_bms_nf_batt_limits_discharge_batt_limit_dschrg_crrnt_decode(
        state.batt_limit_dschrg_crrnt);
    dst->outputs.max_discharge_power =
        abs_bms_nf_batt_limits_discharge_batt_limit_dschrg_pwr_short_term_decode(
            state.batt_limit_dschrg_pwr_short_term);
}

void abs_bms_nf_batt_current_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_current_t state;
    abs_bms_nf_batt_current_init(&state);
    abs_bms_nf_batt_current_unpack(&state, src, ABS_BMS_NF_BATT_CURRENT_LENGTH);
    dst->outputs.pack_current =
        abs_bms_nf_batt_current_batt_current_system_decode(state.batt_current_system);
}

void abs_bms_nf_batt_sys_volt_string_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_sys_volt_string_t state;
    abs_bms_nf_batt_sys_volt_string_init(&state);
    abs_bms_nf_batt_sys_volt_string_unpack(&state, src, ABS_BMS_NF_BATT_SYS_VOLT_STRING_LENGTH);
    dst->outputs.pack_voltage = abs_bms_nf_batt_sys_volt_string_batt_sys_volt_string_max_decode(
        state.batt_sys_volt_string_max);
}

void abs_bms_nf_batt_iso_res_frame_deserialize(struct abs_bms_data_t* dst, const uint8_t* src) {
    struct abs_bms_nf_batt_iso_res_t state;
    abs_bms_nf_batt_iso_res_init(&state);
    abs_bms_nf_batt_iso_res_unpack(&state, src, ABS_BMS_NF_BATT_ISO_RES_LENGTH);
    dst->outputs.isolation = abs_bms_nf_batt_iso_res_batt_iso_res_max_decode(state.batt_iso_res_max);
}

float abs_bms_user_soc_scale(const float soc) {
    float user_soc;
    if (soc >= toc_soc) {
        user_soc = 100.0f;
    } else if (soc <= boc_soc) {
        user_soc = 0.0f;
    } else {
        user_soc = ((soc - boc_soc) / (toc_soc - boc_soc)) * 100.0f;
    }
    return user_soc;
}
