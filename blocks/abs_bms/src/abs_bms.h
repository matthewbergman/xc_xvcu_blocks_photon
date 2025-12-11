#ifndef DEVICES_ABS_BMS_H_
#define DEVICES_ABS_BMS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct abs_bms_inputs_t {
    bool sleep_cmd; // Notes: Active high sleep command
    bool run_cmd; // Notes: Close contactors to enable discharge
    bool charge_cmd; // Notes: Close contactors to enable charge
    bool isolation_monitor_cmd; // Notes: Enable isolation monitoring
    bool balance_cmd; // Notes: Enable cell balancing
};

struct abs_bms_outputs_t {
	float max_charge_current;     // Unit:A
    float max_charge_voltage;     // Unit:V
    float pack_voltage;           // Unit:V
    float pack_current;           // Unit:A
    float max_discharge_current;  // Unit:A
    float max_discharge_power;    // Unit:kW

    float soh;            // Unit:%
    float soc;            // Unit:%
    float energy_remain;  // Unit:kWh

    float temp_inlet;      // Unit:C
    float temp_outlet;     // Unit:C
    float pressure_inlet;  // Unit:mbar

    uint16_t isolation;     // Unit:KOhms

    float min_cell_temp;  // Unit:C
    float max_cell_temp;  // Unit:C

    float min_cell_voltage;  // Unit:V
    float max_cell_voltage;  // Unit:V

    bool state_ready;   // Notes: BMS ready to close contactors
    bool state_drive;   // Notes: BMS in drive state
    bool state_charge;  // Notes: BMS in charge state
    bool state_disconnect; // Notes: BMS requests disconnection of the pack

    bool bms_hvil_closed;   // Notes: BMS HVIL status
    bool isolation_monitor_enabled; // Notes: Isolation monitoring enabled
    bool timeout;  // Notes: True if no CAN messages have been received recently
    bool bms_fault; 
};


struct abs_bms_internal_t {
	uint16_t can_tick_counter;
    uint16_t startup_counter;
    uint8_t alive_counter;
};

struct abs_bms_config_t {
	uint8_t ticks_per_s;
	void (*can_send)(uint32_t, bool, uint8_t, uint8_t*); // ID, len, data buffer
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
