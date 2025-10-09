/*
 * kreisel_bms.c
 *
 *  Created on: Jan 11, 2023
 *      Author: mbergman
 *
 *  Version: 0.1
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "kreisel_bms.h"
#include "can/ke20_bms.h"
#include "../../../utils/counter.h"

/*
typedef struct
{
	unsigned char state_req : 4; // 0
	unsigned char isolation_req : 4; // 4
	unsigned char sleep_req : 4; // 8
	unsigned char range_req : 4; // 12
}KREISEL_PTREQUEST_REQUEST_TYPE;

typedef struct
{
	unsigned char ctrl_aux1 : 1;
	unsigned char ctrl_aux2 : 1;
	unsigned char ctrl_aux3 : 1;
	unsigned char ctrl_aux4 : 1;
	unsigned char alivecounter : 4;
}KREISEL_PTREQUEST_COMM_TYPE;

// CAN Msg 0x11B => PT Request
typedef struct
{
	KREISEL_PTREQUEST_REQUEST_TYPE request; // 0
	unsigned char chargepoweravail[2]; // 16
	unsigned char rsvd; // 32 => set to 0
	KREISEL_PTREQUEST_COMM_TYPE comm; // 40
	unsigned char crc; // 48
}KREISEL_PTREQUEST_TYPE; // 7 Bytes Total

static KREISEL_PTREQUEST_TYPE Kreisel_PTRequest;
*/

static const unsigned char KREISELCRCTABLEDEF[] = {
0x00, 0x2F, 0x5E, 0x71, 0xBC, 0x93, 0xE2, 0xCD, 0x57, 0x78, 0x09, 0x26, 0xEB, 0xC4, 0xB5, 0x9A,
0xAE, 0x81, 0xF0, 0xDF, 0x12, 0x3D, 0x4C, 0x63, 0xF9, 0xD6, 0xA7, 0x88, 0x45, 0x6A, 0x1B, 0x34,
0x73, 0x5C, 0x2D, 0x02, 0xCF, 0xE0, 0x91, 0xBE, 0x24, 0x0B, 0x7A, 0x55, 0x98, 0xB7, 0xC6, 0xE9,
0xDD, 0xF2, 0x83, 0xAC, 0x61, 0x4E, 0x3F, 0x10, 0x8A, 0xA5, 0xD4, 0xFB, 0x36, 0x19, 0x68, 0x47,
0xE6, 0xC9, 0xB8, 0x97, 0x5A, 0x75, 0x04, 0x2B, 0xB1, 0x9E, 0xEF, 0xC0, 0x0D, 0x22, 0x53, 0x7C,
0x48, 0x67, 0x16, 0x39, 0xF4, 0xDB, 0xAA, 0x85, 0x1F, 0x30, 0x41, 0x6E, 0xA3, 0x8C, 0xFD, 0xD2,
0x95, 0xBA, 0xCB, 0xE4, 0x29, 0x06, 0x77, 0x58, 0xC2, 0xED, 0x9C, 0xB3, 0x7E, 0x51, 0x20, 0x0F,
0x3B, 0x14, 0x65, 0x4A, 0x87, 0xA8, 0xD9, 0xF6, 0x6C, 0x43, 0x32, 0x1D, 0xD0, 0xFF, 0x8E, 0xA1,
0xE3, 0xCC, 0xBD, 0x92, 0x5F, 0x70, 0x01, 0x2E, 0xB4, 0x9B, 0xEA, 0xC5, 0x08, 0x27, 0x56, 0x79,
0x4D, 0x62, 0x13, 0x3C, 0xF1, 0xDE, 0xAF, 0x80, 0x1A, 0x35, 0x44, 0x6B, 0xA6, 0x89, 0xF8, 0xD7,
0x90, 0xBF, 0xCE, 0xE1, 0x2C, 0x03, 0x72, 0x5D, 0xC7, 0xE8, 0x99, 0xB6, 0x7B, 0x54, 0x25, 0x0A,
0x3E, 0x11, 0x60, 0x4F, 0x82, 0xAD, 0xDC, 0xF3, 0x69, 0x46, 0x37, 0x18, 0xD5, 0xFA, 0x8B, 0xA4,
0x05, 0x2A, 0x5B, 0x74, 0xB9, 0x96, 0xE7, 0xC8, 0x52, 0x7D, 0x0C, 0x23, 0xEE, 0xC1, 0xB0, 0x9F,
0xAB, 0x84, 0xF5, 0xDA, 0x17, 0x38, 0x49, 0x66, 0xFC, 0xD3, 0xA2, 0x8D, 0x40, 0x6F, 0x1E, 0x31,
0x76, 0x59, 0x28, 0x07, 0xCA, 0xE5, 0x94, 0xBB, 0x21, 0x0E, 0x7F, 0x50, 0x9D, 0xB2, 0xC3, 0xEC,
0xD8, 0xF7, 0x86, 0xA9, 0x64, 0x4B, 0x3A, 0x15, 0x8F, 0xA0, 0xD1, 0xFE, 0x33, 0x1C, 0x6D, 0x42,
};

static const unsigned char KREISELCTCTABLEMSG[] = {
0x3D, 0xBE, 0x44, 0xC5, 0x4B, 0xCC, 0x52, 0xD3, 0x59, 0xDA, 0x60, 0xE1, 0x67, 0xE8, 0x6E, 0xEF,
};

uint8_t kreisel_bms_crc(uint8_t* data, uint8_t alive_counter)
{
	unsigned char crc, datalen;
	datalen = 6;

	crc = 0xFF; // CRC = 255 Startvalue

	while (datalen--)
		crc = KREISELCRCTABLEDEF[*data++ ^ crc];

	crc = KREISELCTCTABLEMSG[alive_counter] ^ crc;
	crc = KREISELCRCTABLEDEF[crc];
	return (crc ^ 0xFF);
}

/*
static void KREISEL_Send_PTRequest(void)
{
	// Set Message Data
	Kreisel_PTRequest.chargepoweravail[0] = ___; // => 16 Bit Motorola Format
	Kreisel_PTRequest.chargepoweravail[1] = ___;
	Kreisel_PTRequest.request.isolation_req = ___;
	Kreisel_PTRequest.request.range_req = ___;
	Kreisel_PTRequest.request.sleep_req = ___;
	Kreisel_PTRequest.request.state_req = ___; // Battery Working Mode
	Kreisel_PTRequest.comm.ctrl_aux1 = ___;
	Kreisel_PTRequest.comm.ctrl_aux2 = ___;
	Kreisel_PTRequest.comm.ctrl_aux3 = ___;
	Kreisel_PTRequest.comm.ctrl_aux4 = ___;
	KREISEL_Crc();
	// Send Data
	CAN_SendData(&Kreisel_PTRequest, 7);
}
*/


void kreisel_bms_dummy_can_send(uint32_t id, bool extended, uint8_t len, uint8_t* buf)
{
	(void) id;
	(void) extended;
	(void) len;
	(void) buf;
}

void kreisel_bms_init(struct kreisel_bms_data_t* data)
{
	data->config.ticks_per_s = 100;
	data->config.can_send = kreisel_bms_dummy_can_send;

	// We are timed out until we get a message
	data->outputs.timeout = true;
	data->internal.can_tick_counter = (data->config.ticks_per_s * 2) +1;
}

// TODO: ignore bad values

void kreisel_bms_parse_can(uint32_t can_id, uint8_t* buf, struct kreisel_bms_data_t* data)
{
	if (can_id == BMS_BMS_CHARGE_FRAME_ID)
	{
		// 0x333
		struct bms_bms_charge_t charge;

		bms_bms_charge_unpack(&charge, buf, BMS_BMS_CHARGE_LENGTH);

		data->outputs.max_charge_current = bms_bms_charge_charge_current_setpoint_bms_decode(charge.charge_current_setpoint_bms);
		data->outputs.max_charge_voltage = bms_bms_charge_charge_voltage_limit_bms_decode(charge.charge_voltage_limit_bms);
	}

	if (can_id == BMS_BMS_CURRENT_VOLTAGE_FRAME_ID)
	{
		// 0x331
		struct bms_bms_current_voltage_t current_voltage;

		bms_bms_current_voltage_unpack(&current_voltage, buf, BMS_BMS_CURRENT_VOLTAGE_LENGTH);

		data->outputs.pack_current = bms_bms_current_voltage_current_bms_decode(current_voltage.current_bms);
		data->outputs.pack_voltage = bms_bms_current_voltage_link_voltage_bms_decode(current_voltage.link_voltage_bms);
	}

	if (can_id == BMS_BMS_DRIVE_LIMITS_FRAME_ID)
	{
		// 0x332
		struct bms_bms_drive_limits_t drive_limits;

		bms_bms_drive_limits_unpack(&drive_limits, buf, BMS_BMS_DRIVE_LIMITS_LENGTH);

		data->outputs.max_discharge_current = bms_bms_drive_limits_discharge_current_limit_bms_decode(drive_limits.discharge_current_limit_bms);
	}

	if (can_id == BMS_BMS_ENERGY_SOH_FRAME_ID)
	{
		// 0x334
		struct bms_bms_energy_soh_t energy_soh;

		bms_bms_energy_soh_unpack(&energy_soh, buf, BMS_BMS_ENERGY_SOH_LENGTH);

		data->outputs.soh = bms_bms_energy_soh_min_soh_bms_decode(energy_soh.min_soh_bms);
		data->outputs.energy_remain = bms_bms_energy_soh_energy_available_bms_decode(energy_soh.energy_available_bms);
	}

	if (can_id == BMS_BMS_FLUID_MEAS_USER_SOC_FRAME_ID)
	{
		// 0x33a
		struct bms_bms_fluid_meas_user_soc_t fluid_meas_user_soc;

		bms_bms_fluid_meas_user_soc_unpack(&fluid_meas_user_soc, buf, BMS_BMS_FLUID_MEAS_USER_SOC_LENGTH);

		data->outputs.temp_inlet = bms_bms_fluid_meas_user_soc_fluid_temperature_inlet_bms_decode(fluid_meas_user_soc.fluid_temperature_inlet_bms);
		data->outputs.temp_outlet = bms_bms_fluid_meas_user_soc_fluid_temperature_outlet_bms_decode(fluid_meas_user_soc.fluid_temperature_outlet_bms); // TODO: no outlet temp
		data->outputs.pressure_inlet = bms_bms_fluid_meas_user_soc_fluid_pressure_inlet_bms_decode(fluid_meas_user_soc.fluid_pressure_inlet_bms); // above ambient
		data->outputs.soc = bms_bms_fluid_meas_user_soc_user_soc_bms_decode(fluid_meas_user_soc.user_soc_bms);
	}

	if (can_id == BMS_BMS_ISOLATION_SOC_FRAME_ID)
	{
		// 0x336
		struct bms_bms_isolation_soc_t isolation_soc;

		bms_bms_isolation_soc_unpack(&isolation_soc, buf, BMS_BMS_ISOLATION_SOC_LENGTH);

		data->outputs.iso_ext = bms_bms_isolation_soc_isolation_resistance_extern_bms_decode(isolation_soc.isolation_resistance_extern_bms);
		data->outputs.iso_int = bms_bms_isolation_soc_isolation_resistance_intern_bms_decode(isolation_soc.isolation_resistance_intern_bms);
	}

	if (can_id == BMS_BMS_MODULE_TEMPERATURES_FRAME_ID)
	{
		// 0x337
		struct bms_bms_module_temperatures_t module_temperatures;

		bms_bms_module_temperatures_unpack(&module_temperatures, buf, BMS_BMS_MODULE_TEMPERATURES_LENGTH);

		data->outputs.min_cell_temp = bms_bms_module_temperatures_min_module_temperature_bms_decode(module_temperatures.min_module_temperature_bms);
		data->outputs.max_cell_temp = bms_bms_module_temperatures_max_module_temperature_bms_decode(module_temperatures.max_module_temperature_bms);
	}

	if (can_id == BMS_BMS_BRICK_VOLTAGES_FRAME_ID)
	{
		// 0x335
		struct bms_bms_brick_voltages_t voltages;

		bms_bms_brick_voltages_unpack(&voltages, buf, BMS_BMS_BRICK_VOLTAGES_LENGTH);

		data->outputs.min_cell_voltage = bms_bms_brick_voltages_min_brick_voltage_bms_decode(voltages.min_brick_voltage_bms);
		data->outputs.max_cell_voltage = bms_bms_brick_voltages_max_brick_voltage_bms_decode(voltages.max_brick_voltage_bms);
	}

	if (can_id == BMS_BMS_POWER_PREDICTION_FRAME_ID)
	{
		// 0x33B
		struct bms_bms_power_prediction_t power;

		bms_bms_power_prediction_unpack(&power, buf, BMS_BMS_POWER_PREDICTION_LENGTH);

		data->outputs.max_discharge_power = bms_bms_power_prediction_discharge_power_pred_bms_decode(power.discharge_power_pred_bms);
	}

	if (can_id == BMS_BMS_STATE_FRAME_ID)
	{
		// 0x11a
		struct bms_bms_state_t state;

		bms_bms_state_unpack(&state, buf, BMS_BMS_STATE_LENGTH);

		data->outputs.bms_state = state.state_bms; // 1:1 with DBC

		data->outputs.isolation_monitor_enabled = (state.isolation_state_bms == BMS_BMS_STATE_ISOLATION_STATE_BMS_ISOLATION_ACTIVE_CHOICE);

		if (state.hvil_state_bms == BMS_BMS_STATE_HVIL_STATE_BMS_HVIL_OK_CHOICE)
			data->outputs.bms_hvil_closed = true;
		else
			data->outputs.bms_hvil_closed = false;

		data->internal.can_tick_counter = 0;
	}
}

void kreisel_bms_tick(struct kreisel_bms_data_t* data)
{
	// Send at 10ms
	if (1)
	{
		struct bms_pt_request_t pt_request;

		pt_request.state_request_pt = data->inputs.state_cmd; // 1:1 YACP and KE20 DBC

		if (data->inputs.sleep_cmd == 0)
			pt_request.sleep_request_pt = BMS_PT_REQUEST_SLEEP_REQUEST_PT_SLEEP_INACTIVE_CHOICE;
		else
			pt_request.sleep_request_pt = BMS_PT_REQUEST_SLEEP_REQUEST_PT_SLEEP_ACTIVE_CHOICE;

		// For now always max, but could be storage etc
		pt_request.range_request_pt = BMS_PT_REQUEST_RANGE_REQUEST_PT_RANGE_MAXIMUM_CHOICE;

		if (data->inputs.isolation_monitor_cmd == 1)
			pt_request.isolation_request_pt = BMS_PT_REQUEST_ISOLATION_REQUEST_PT_ISOLATION_ACTIVE_CHOICE;
		else
			pt_request.isolation_request_pt = BMS_PT_REQUEST_ISOLATION_REQUEST_PT_ISOLATION_INACTIVE_CHOICE;

		 // inhibits drive when there is charge power
		pt_request.charge_power_available_pt = bms_pt_request_charge_power_available_pt_encode(data->inputs.charge_power_available);

		pt_request.cntr_request_aux1_pt = 0;
		pt_request.cntr_request_aux2_pt = 0;
		pt_request.cntr_request_aux3_pt = 0;
		pt_request.cntr_request_aux4_pt = 0;

		pt_request.alive_pt = data->internal.alive_counter;

		uint8_t bytes[7];
		bms_pt_request_pack(bytes, &pt_request, BMS_PT_REQUEST_LENGTH);

		pt_request.crc_pt = kreisel_bms_crc(bytes, pt_request.alive_pt);

		bms_pt_request_pack(bytes, &pt_request, BMS_PT_REQUEST_LENGTH);

		if (data->inputs.sleep_cmd == 0) // TODO: WTF??????
		{
			data->config.can_send(BMS_PT_REQUEST_FRAME_ID, false, BMS_PT_REQUEST_LENGTH, bytes);
		}

		if (++data->internal.alive_counter > 15)
			data->internal.alive_counter = 0;
	}

	counter_inc(&(data->internal.can_tick_counter));
	data->internal.startup_counter++;

	// Reset the data if the OBC has timed out
	if (data->internal.can_tick_counter > (data->config.ticks_per_s * 2))
	{
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
		data->outputs.iso_int = 0;
		data->outputs.iso_ext = 0;
		data->outputs.min_cell_temp = 0;
		data->outputs.max_cell_temp = 0;
		data->outputs.bms_state = BMS_BMS_STATE_STATE_BMS_STATE_INVALID_CHOICE;
		data->outputs.bms_hvil_closed = false;

		data->outputs.timeout = true;
	}
	else
	{
		data->outputs.timeout = false;
	}
}

