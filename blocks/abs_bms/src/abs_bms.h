#ifndef DEVICES_ABS_BMS_H_
#define DEVICES_ABS_BMS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*fp_can_send)(uint32_t, bool, uint8_t, uint8_t*);

struct abs_bms_inputs_t {
	float charge_power_available;  // kW
    bool isolation_monitor_cmd;

    uint8_t state_cmd;
    bool close_contactors_cmd;

    bool sleep_cmd;
};

struct abs_bms_outputs_t {
	float max_charge_current;     // A
    float max_charge_voltage;     // V
    float pack_voltage;           // V
    float pack_current;           // A
    float max_discharge_current;  // A
    float max_discharge_power;    // kW

    float soh;            // %
    float soc;            // %
    float energy_remain;  // kWh

    float temp_inlet;      // C
    float temp_outlet;     // C
    float pressure_inlet;  // mbar

    uint8_t iso_min_str_id;
    uint16_t iso_min;

    uint8_t iso_max_str_id;
    uint16_t iso_max;

    float min_cell_temp;  // C
    float max_cell_temp;  // C

    float min_cell_voltage;  // V
    float max_cell_voltage;  // V

    uint8_t bms_state;
    bool bms_hvil_closed;
    bool isolation_monitor_enabled;
    bool timeout;
    bool bms_fault;
};


struct abs_bms_internal_t {
	uint16_t can_tick_counter;
    uint16_t startup_counter;
    uint8_t alive_counter;
};

struct abs_bms_config_t {
	uint8_t ticks_per_s;
    fp_can_send can_send;
};

struct abs_bms_data_t {
	struct abs_bms_inputs_t inputs;
	struct abs_bms_outputs_t outputs;
    struct abs_bms_internal_t internal;
	struct abs_bms_config_t config;
};

void abs_bms_init(struct abs_bms_data_t* data);
void abs_bms_parse_can(uint32_t can_id, uint8_t* buf, struct abs_bms_data_t* data);
void abs_bms_tick(struct abs_bms_data_t* data);

#endif
