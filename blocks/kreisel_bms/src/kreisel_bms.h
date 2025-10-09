/*
 * kreisel_bms.h
 *
 *  Created on: Jan 11, 2023
 *      Author: mbergman
 *
 *  Version: 0.1
 */

#ifndef DEVICES_KREISEL_BMS_H_
#define DEVICES_KREISEL_BMS_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct kreisel_bms_inputs_t {
	float charge_power_available; // kW
	bool isolation_monitor_cmd;
	uint8_t state_cmd;
	bool sleep_cmd;
};

struct kreisel_bms_outputs_t {
	float max_charge_current; // A
	float max_charge_voltage; // V
	float pack_voltage; // V
	float pack_current; // A
	float max_discharge_current; // A
	float max_discharge_power; // kW

	float soh; // %
	float soc; // %
	float energy_remain; // kWh

	float temp_inlet; // C
	float temp_outlet; // C
	float pressure_inlet; // mbar

	uint16_t iso_int; // ohm/V
	uint16_t iso_ext; // ohm/V

	float min_cell_temp; // C
	float max_cell_temp; // C

	float min_cell_voltage; // V
	float max_cell_voltage; // V

	uint8_t bms_state;
	bool bms_hvil_closed;
	bool isolation_monitor_enabled;

	bool timeout;
};

struct kreisel_bms_internal_t {
	uint16_t can_tick_counter;
	uint16_t startup_counter;
	uint8_t alive_counter;
};

struct kreisel_bms_config_t {
	uint8_t ticks_per_s;
	void (*can_send)(uint32_t, bool, uint8_t, uint8_t*); // ID, len, data buffer
};

struct kreisel_bms_data_t {
	struct kreisel_bms_inputs_t inputs;
	struct kreisel_bms_outputs_t outputs;
	struct kreisel_bms_internal_t internal;
	struct kreisel_bms_config_t config;
};

void kreisel_bms_init(struct kreisel_bms_data_t* data);
void kreisel_bms_parse_can(uint32_t can_id, uint8_t* buf, struct kreisel_bms_data_t* data);
void kreisel_bms_tick(struct kreisel_bms_data_t* data);

#endif
