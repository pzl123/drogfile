#ifndef DCDC_SET_H
#define DCDC_SET_H

#include "dcdc.h"

#include <linux/can.h>

class Widget;

void frame2set_output_vol(Widget *widget_m, struct can_frame frame);
void frame2set_output_vol_max(Widget *widget_m, struct can_frame frame);
void frame2set_output_cur(Widget *widget_m, struct can_frame frame);
void frame2set_output_pwr(Widget *widget_m, struct can_frame frame);
void frame2set_switch_state(Widget *widget_m, struct can_frame frame);
void frame2set_group_num(Widget *widget_m, struct can_frame frame);
void frame2set_work_altitude(Widget *widget_m, struct can_frame frame);
void frame2set_over_vol_reset(Widget *widget_m, struct can_frame frame);
void frame2set_over_vol_protect(Widget *widget_m, struct can_frame frame);
void frame2set_sc_reset(Widget *widget_m, struct can_frame frame);

void print_can_frame(const struct can_frame& frame);
#endif
