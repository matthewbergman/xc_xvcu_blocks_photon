/*
 * dometic_helm.h
 *
 *  Created on: Apr 25, 2024
 *      Author: mbergman
 *
 *  Version: 0.1
 */

#ifndef DEVICES_DOMETIC_HELM_H_
#define DEVICES_DOMETIC_HELM_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../../utils/defines.h"

struct dometic_helm_outputs_t {
	Direction gear_cmd_port;
	Direction gear_cmd_stbd;
	float throttle_cmd_port;	// Unit: %
	float throttle_cmd_stbd;	// Unit: %
	bool danger_fault;
	bool ch_controlling;	// Notes: Control head is controlling vs joystick
	bool timeout;
};

struct dometic_helm_inputs_t {
	Direction gear_actual_port;
	Direction gear_actual_stbd;
	float throttle_actual_port;	// Unit: %
	float throttle_actual_stbd; // Unit: %
	float engine_speed_port; // Unit: RPM
	float engine_speed_stbd; // Unit: RPM
	bool danger_fault;
};

struct dometic_helm_internal_t {
	uint16_t can_tick_counter;
	uint16_t startup_counter;
	bool address_claim_send;
};

struct dometic_helm_config_t {
	uint8_t ticks_per_s;
	float idle_speed; // Unit: RPM
	void (*can_send)(uint32_t, bool, uint8_t, uint8_t*); // ID, len, data buffer
};

struct dometic_helm_data_t {
	struct dometic_helm_outputs_t outputs;
	struct dometic_helm_inputs_t inputs;
	struct dometic_helm_internal_t internal;
	struct dometic_helm_config_t config;
};

void dometic_helm_init(struct dometic_helm_data_t* data);
void dometic_helm_parse_can(uint32_t can_id, uint8_t* buf, struct dometic_helm_data_t* data);
void dometic_helm_tick(struct dometic_helm_data_t* data);

#endif
