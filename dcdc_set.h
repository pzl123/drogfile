#ifndef DCDC_SET_H
#define DCDC_SET_H

#include "dcdc.h"

void frame2set_output_vol(struct can_frame frame);
void frame2set_output_vol_max(struct can_frame frame);
void frame2set_output_cur(struct can_frame frame);
void frame2set_output_pwr(struct can_frame frame);
void frame2set_switch_state(struct can_frame frame);
void frame2set_group_num(struct can_frame frame);
void frame2set_work_altitude(struct can_frame frame);
void frame2set_over_vol_reset(struct can_frame frame);
void frame2set_over_vol_protect(struct can_frame frame);
void frame2set_sc_reset(struct can_frame frame);

void print_can_frame(const struct can_frame& frame);
#endif
