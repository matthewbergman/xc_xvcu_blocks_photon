/*
 * dometic_helm.c
 *
 *  Created on: Apr 24, 2024
 *      Author: mbergman
 *
 *  Version: 0.1
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "dometic_helm.h"
#include "can/dom.h"
#include "../../../utils/defines.h"

void dometic_helm_dummy_can_send(uint32_t id, bool extended, uint8_t len, uint8_t* buf)
{
	(void)id;
	(void)extended;
	(void)len;
	(void)buf;
}

void dometic_helm_init(struct dometic_helm_data_t* data)
{
	data->config.can_send = dometic_helm_dummy_can_send;
}

void dometic_helm_parse_can(uint32_t can_id, uint8_t* buf, struct dometic_helm_data_t* data)
{
	// Data page: 1 bit
	// PDU format: 8 bits
	// PDU specific: 8 bits
	uint32_t pgn = (can_id >> 8) & 0x1FFFF;
	//uint8_t source_id = can_id & 0xFF;

	if (buf[0] == 0 && buf[1] == 0xEE && buf[2] == 0)
	{
		data->internal.address_claim_send = 1;
	}
	else if (pgn == 0xFF64)
	{
		// port (either from control head: source ID 0x01, or joystick source ID 0x24)
		struct dom_actuator_cmd_port_t actuator_cmd;

		dom_actuator_cmd_port_unpack(&actuator_cmd, buf, DOM_ACTUATOR_CMD_PORT_LENGTH);

		if (actuator_cmd.throttle_command < DOM_ACTUATOR_CMD_STBD_THROTTLE_COMMAND_ERROR_CHOICE)
			data->outputs.throttle_cmd_port = dom_actuator_cmd_port_throttle_command_decode(actuator_cmd.throttle_command);
		else
			data->outputs.throttle_cmd_port = 0.0f;

		if (actuator_cmd.gear_command == DOM_ACTUATOR_CMD_STBD_GEAR_COMMAND_FOWARD_CHOICE)
			data->outputs.gear_cmd_port = FORWARD;
		else if (actuator_cmd.gear_command == DOM_ACTUATOR_CMD_STBD_GEAR_COMMAND_REVERSE_CHOICE)
			data->outputs.gear_cmd_port = REVERSE;
		else
			data->outputs.gear_cmd_port = NEUTRAL;

		data->internal.can_tick_counter = 0;
	}
	else if (pgn == 0xFF65)
	{
		// stbd (either from control head: source ID 0x01, or joystick source id 0x24)
		struct dom_actuator_cmd_stbd_t actuator_cmd;

		dom_actuator_cmd_stbd_unpack(&actuator_cmd, buf, DOM_ACTUATOR_CMD_STBD_LENGTH);

		if (actuator_cmd.throttle_command < DOM_ACTUATOR_CMD_STBD_THROTTLE_COMMAND_ERROR_CHOICE)
			data->outputs.throttle_cmd_stbd = dom_actuator_cmd_port_throttle_command_decode(actuator_cmd.throttle_command);
		else
			data->outputs.throttle_cmd_stbd = 0.0f;

		if (actuator_cmd.gear_command == DOM_ACTUATOR_CMD_STBD_GEAR_COMMAND_FOWARD_CHOICE)
			data->outputs.gear_cmd_stbd = FORWARD;
		else if (actuator_cmd.gear_command == DOM_ACTUATOR_CMD_STBD_GEAR_COMMAND_REVERSE_CHOICE)
			data->outputs.gear_cmd_stbd = REVERSE;
		else
			data->outputs.gear_cmd_stbd = NEUTRAL;

		data->internal.can_tick_counter = 0;
	}
	else if (pgn == 0xFF6E)
	{
		// control head source ID 0x01
		struct dom_control_head_feedback_t control_head;

		dom_control_head_feedback_unpack(&control_head, buf, DOM_CONTROL_HEAD_FEEDBACK_LENGTH);

		data->outputs.ch_controlling = control_head.ch_controlling;
		data->outputs.danger_fault = control_head.danger_fault;
	}
}

void dometic_helm_tick(struct dometic_helm_data_t* data)
{
	uint8_t buf[8];

	// Send address claim occasionally
	if (data->internal.startup_counter % 10 == 9)
	{
		data->internal.address_claim_send = 1;
	}

	// Send address claim every now and then
	if (data->internal.address_claim_send)
	{
		struct dom_address_claim_t claim;

		claim.identity_number = 666;
		claim.manufacturer_code = 1850;
		claim.ecu_instance = 0;
		claim.function_instance = 0;
		claim.function = 181;
		claim.reserved_bit = 0;
		claim.vehicle_system = 50;
		claim.vehicle_system_instance = 0;
		claim.industry_group = 4;
		claim.arbitrary_address_bit = 1;

		dom_address_claim_pack(buf, &claim, 8);

		data->config.can_send(0x18EEFF60, true, 8, buf);

		claim.identity_number = 777;
		claim.manufacturer_code = 1850;
		claim.ecu_instance = 1;
		claim.function_instance = 0;
		claim.function = 181;
		claim.reserved_bit = 0;
		claim.vehicle_system = 50;
		claim.vehicle_system_instance = 0;
		claim.industry_group = 4;
		claim.arbitrary_address_bit = 1;

		 dom_address_claim_pack(buf, &claim, 8);

		 data->config.can_send(0x18EEFF61, true, 8, buf);

		 data->internal.address_claim_send = 0;
	}

	if (data->internal.startup_counter % (data->config.ticks_per_s / 10) == 0)
	{
		struct dom_actuator_feedback_port_t actuator_feedback;

		actuator_feedback.manufacturer_id = 1850;
		actuator_feedback.industry_group = 4;
		actuator_feedback.engine_instance = 0; // Port
		actuator_feedback.actuator_type = 2; // Combined
		actuator_feedback.control_state = 1; // Active

		if (data->inputs.gear_actual_port == FORWARD)
			actuator_feedback.actual_gear_value = DOM_ACTUATOR_FEEDBACK_STBD_ACTUAL_GEAR_VALUE_FOWARD_CHOICE;
		else if (data->inputs.gear_actual_port == REVERSE)
			actuator_feedback.actual_gear_value = DOM_ACTUATOR_FEEDBACK_STBD_ACTUAL_GEAR_VALUE_REVERSE_CHOICE;
		else
			actuator_feedback.actual_gear_value = DOM_ACTUATOR_FEEDBACK_STBD_ACTUAL_GEAR_VALUE_NEUTRAL_CHOICE;
		actuator_feedback.actual_throttle_value = dom_actuator_feedback_port_actual_throttle_value_encode(data->inputs.throttle_actual_port);
		actuator_feedback.danger_fault = data->inputs.danger_fault;

		actuator_feedback.reserved = 0;
		actuator_feedback.reserved_1 = 3;
		actuator_feedback.reserved_2 = 0;
		actuator_feedback.warning_fault = 0;
		actuator_feedback.shift_forward_fault = 0;
		actuator_feedback.shift_neutral_fault = 0;
		actuator_feedback.shift_reverse_fault = 0;
		actuator_feedback.throttle_up_fault = 0;
		actuator_feedback.throttle_down_fault = 0;
		actuator_feedback.reserved_3 = 0;

		dom_actuator_feedback_port_pack(buf, &actuator_feedback, 8);

		data->config.can_send(DOM_ACTUATOR_FEEDBACK_PORT_FRAME_ID, true, 8, buf);
	}
	else if (data->internal.startup_counter % (data->config.ticks_per_s / 10) == 1)
	{
		struct dom_actuator_feedback_stbd_t actuator_feedback;

		actuator_feedback.manufacturer_id = 1850;
		actuator_feedback.industry_group = 4;
		actuator_feedback.engine_instance = 1; // Stbd
		actuator_feedback.actuator_type = 2; // Combined
		actuator_feedback.control_state = 1; // Active

		if (data->inputs.gear_actual_stbd == FORWARD)
			actuator_feedback.actual_gear_value = DOM_ACTUATOR_FEEDBACK_STBD_ACTUAL_GEAR_VALUE_FOWARD_CHOICE;
		else if (data->inputs.gear_actual_stbd== REVERSE)
			actuator_feedback.actual_gear_value = DOM_ACTUATOR_FEEDBACK_STBD_ACTUAL_GEAR_VALUE_REVERSE_CHOICE;
		else
			actuator_feedback.actual_gear_value = DOM_ACTUATOR_FEEDBACK_STBD_ACTUAL_GEAR_VALUE_NEUTRAL_CHOICE;
		actuator_feedback.actual_throttle_value = dom_actuator_feedback_stbd_actual_throttle_value_encode(data->inputs.throttle_actual_stbd);
		actuator_feedback.danger_fault = data->inputs.danger_fault;

		actuator_feedback.reserved = 0;
		actuator_feedback.reserved_1 = 3;
		actuator_feedback.reserved_2 = 0;
		actuator_feedback.warning_fault = 0;
		actuator_feedback.shift_forward_fault = 0;
		actuator_feedback.shift_neutral_fault = 0;
		actuator_feedback.shift_reverse_fault = 0;
		actuator_feedback.throttle_up_fault = 0;
		actuator_feedback.throttle_down_fault = 0;
		actuator_feedback.reserved_3 = 0;

		dom_actuator_feedback_stbd_pack(buf, &actuator_feedback, 8);

		data->config.can_send(DOM_ACTUATOR_FEEDBACK_STBD_FRAME_ID, true, 8, buf);
	}
	else if (data->internal.startup_counter % (data->config.ticks_per_s / 10) == 2)
	{
		struct dom_engine_parameters_rapid_update_t engine;

		float engine_speed = fabs(data->inputs.engine_speed_port / 2.0f) + data->config.idle_speed;

		engine.engine_instance = 0; // Port
		engine.engine_speed = dom_engine_parameters_rapid_update_engine_speed_encode(engine_speed);
		engine.engine_boost_pressure = 0;
		engine.engine_tilt_trim = 0;

		dom_engine_parameters_rapid_update_pack(buf, &engine, 8);

		data->config.can_send(0x9F20001, true, 8, buf);
	}
	else if (data->internal.startup_counter % (data->config.ticks_per_s / 10) == 3)
	{
		struct dom_engine_parameters_rapid_update_t engine;

		float engine_speed = fabs(data->inputs.engine_speed_stbd / 2.0f) + data->config.idle_speed;

		engine.engine_instance = 1; // Stbd
		engine.engine_speed = dom_engine_parameters_rapid_update_engine_speed_encode(engine_speed);
		engine.engine_boost_pressure = 0;
		engine.engine_tilt_trim = 0;

		dom_engine_parameters_rapid_update_pack(buf, &engine, 8);

		data->config.can_send(0x9F20001, true, 8, buf);
	}

	if (data->internal.can_tick_counter < 0xFFF0)
		data->internal.can_tick_counter++;
	data->internal.startup_counter++;

	// Reset the data if the OBC has timed out
	if (data->internal.can_tick_counter > (data->config.ticks_per_s * 2))
	{
		data->outputs.gear_cmd_port = NEUTRAL;
		data->outputs.gear_cmd_stbd = NEUTRAL;
		data->outputs.throttle_cmd_port = 0.0f;
		data->outputs.throttle_cmd_stbd = 0.0f;
		data->outputs.danger_fault = 0;
		data->outputs.ch_controlling = 0;

		data->outputs.timeout = true;
	}
	else
	{
		data->outputs.timeout = false;
	}
}
