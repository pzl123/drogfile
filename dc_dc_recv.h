#ifndef DC_DC_RECV_H
#define DC_DC_RECV_H

class Widget;
#include "dcdc.h"

#ifdef __cplusplus
extern "C" {
#endif



#include <stdint.h>
#include <linux/can.h>


typedef struct
{
    uint32_t data;
    uint32_t index;
    float32_t float_data;
} recv_data_t;

int32_t recv_get_mode_out_vol(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_out_cur(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_flow_limit_point(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_dc_card_temp(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_dc_input_vol(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_pfc0_vol(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_pfc1_vol(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_env_temp(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_pfc_temp(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_rated_out_pow(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_rated_out_cur(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_work_altitude(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_out_cur(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_group_num(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_addr_alloc(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_out_pwr(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_out_vol(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_limit_point(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_out_vol_max(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_on_or_off(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_over_vol_reset(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_out_over_vol_protection(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_alarm_status(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_dip_addr(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_input_pow(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_work_altitude(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_input_work_mode(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_dcdc_soft_ver(Widget *widget_m, struct can_frame frame);
int32_t recv_get_mode_pfc_soft_ver(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_input_model(Widget *widget_m, struct can_frame frame);
int32_t recv_set_mode_sc_reset(Widget *widget_m, struct can_frame frame);

void convert_byte_order(void *src, size_t size, bool big_endian);
bool is_little_endian(void);
void recv_float_data_pre(struct can_frame frame, recv_data_t *recv_data);
void recv_data_pre(struct can_frame frame, recv_data_t *recv_data);
float32_t frame_data_to_float(struct can_frame frame);

#ifdef __cplusplus
}
#endif

#endif /* DC_DC_RECV_H */
