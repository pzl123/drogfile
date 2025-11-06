#include "dcdc_set.h"
#include "dc_dc_recv.h"
#include "widget.h"


#include <string.h>
#include <arpa/inet.h>

#include <cstdio>

void print_can_frame(const struct can_frame& frame)
{
    printf("ID: 0x%X, Data:", frame.can_id);
    for (int i = 0; i < frame.can_dlc; i++) {
        printf(" %02X", frame.data[i]);
    }
    printf("\n");
}

static uint32_t frame_from_dcdc_index(struct can_frame frame)
{
    dcdc_can_id_u can_id;
    can_id.id = frame.can_id;
    return can_id.can_id_info.dst_addr - 1;
}


template <typename SetterFunc>
void frame_to_set(Widget *widget_m, const can_frame &frame, SetterFunc setter, bool is_float = true)
{
    dcdc_can_id_u canId{};
    canId.id = frame.can_id;

    dcdc *dc = widget_m->getdcdcArray();
    recv_data_t data = {0};
    if (is_float)
    {
        recv_float_data_pre(frame, &data);
    }
    else
    {
        recv_data_pre(frame, &data);
    }
    float32_t value_f = data.float_data;
    float32_t value_u = data.data;

    if (0 == canId.can_id_info.ptp) /* 广播 */
    {
        if (0xFE == canId.can_id_info.dst_addr) /* 组内广播 */
        {
            for (uint8_t i = 0U; i < DCDC_NUM; i++)
            {
                if (canId.can_id_info.group == dc[i].get_group_num())
                {
                    if (is_float)
                    {
                        (dc[i].*setter)(value_f);
                    }
                    else
                    {
                        (dc[i].*setter)(value_u);
                    }
                }
            }
        }
        else if (0xFF == canId.can_id_info.dst_addr) /* 全广播 */
        {
            for (uint8_t i = 0U; i < DCDC_NUM; i++)
            {
                if (is_float)
                {
                    (dc[i].*setter)(value_f);
                }
                else
                {
                    (dc[i].*setter)(value_u);
                }
            }
        }
        else
        {
            std::cout << "error dst " << canId.can_id_info.dst_addr << std::endl;
        }
    }
    else
    {
        if (is_float)
        {
            (dc[canId.can_id_info.dst_addr - 1].*setter)(value_f);
        }
        else
        {
            (dc[canId.can_id_info.dst_addr - 1].*setter)(value_u);
        }
    }
}

template <typename SetterFunc, typename EnumType>
void frame_to_set(Widget *widget_m, const can_frame& frame, SetterFunc setter, EnumType*)
{
    dcdc_can_id_u canId{};
    canId.id = frame.can_id;
    dcdc* dc = widget_m->getdcdcArray();
    recv_data_t data = {0};
    recv_data_pre(frame, &data);
    uint32_t raw_value = data.data;

    EnumType value = static_cast<EnumType>(raw_value);

    uint8_t dst = canId.can_id_info.dst_addr;

    if (canId.can_id_info.ptp == 0) // 广播
    {
        if (dst == 0xFE) // 组内广播
        {
            for (uint8_t i = 0; i < DCDC_NUM; ++i)
            {
                if (canId.can_id_info.group == dc[i].get_group_num())
                {
                    (dc[i].*setter)(value);
                }
            }
        }
        else if (dst == 0xFF) // 全广播
        {
            for (uint8_t i = 0; i < DCDC_NUM; ++i)
            {
                (dc[i].*setter)(value);
            }
        }
        else
        {
            std::cout << "error dst " << (int)dst << std::endl;
        }
    }
    else
    {
        (dc[dst - 1].*setter)(value);
    }
}


void frame2set_output_vol(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_output_vol);
}

void frame2set_output_vol_max(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_output_vol_max);
}
void frame2set_output_cur(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_output_cur);
}
void frame2set_output_pwr(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_output_pwr);
}
void frame2set_switch_state(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_switch_state, (switch_state_e*)nullptr);
}
void frame2set_group_num(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_group_num, false);
}
void frame2set_work_altitude(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_work_altitude, false);
}
void frame2set_over_vol_reset(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_over_vol_reset, (over_vol_reset_e*)nullptr);
}
void frame2set_over_vol_protect(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_over_vol_protect, (over_vol_protect_e*)nullptr);
}
void frame2set_sc_reset(Widget *widget_m, struct can_frame frame)
{
    frame_to_set(widget_m, frame, &dcdc::set_set_sc_reset, (sc_reset_e*)nullptr);
}
