#include "dc_dc_recv.h"

#include "widget.h"

#include <arpa/inet.h>

bool is_little_endian(void)
{
    union
    {
        uint16_t i;
        uint8_t c;
    } u;
    u.i = 0x01;
    /*
    数据高位 --> 低位
    0x 00 01

    低地址 --> 高地址
    小端模式
    0x 01 00
    大端模式
    0x 00 01
     */
    return u.c == 0x01;
}

void convert_byte_order(void *src, size_t size, bool big_endian)
{
    if ((NULL == src) || (0 == size) )
    {
        return;
    }

    /* 主机字节序与转换字节序相同不需要进行转换 */
    if (!is_little_endian() && big_endian)
    {
        return;
    }

    uint8_t *tmp = (uint8_t*)src;
    for (size_t i = 0U; i < size / 2; i++)
    {
        uint8_t c = tmp[i];
        tmp[i] = tmp[size - 1 - i];
        tmp[size - 1 - i] = c;
    }
}


float32_t frame_data_to_float(struct can_frame frame)
{
    uint32_t data_tmp = 0;
    (void)memcpy(&data_tmp, &frame.data[4], 4);
    convert_byte_order(&data_tmp, sizeof(uint32_t), false);
    union
    {
        uint32_t num;
        float32_t f;
    } u;
    u.num = data_tmp;
    return u.f;
}

void recv_data_pre(struct can_frame frame, recv_data_t *recv_data)
{
    dcdc_can_id_u can_id_info = {0};
    can_id_info.id = frame.can_id;
    recv_data->index = can_id_info.can_id_info.src_addr - 1;
    if (recv_data->index >= DCDC_NUM)
    {
        return;
    }
    (void)memcpy(&recv_data->data, &frame.data[4], sizeof(uint32_t));
    recv_data->data = ntohl(recv_data->data);
}

void recv_float_data_pre(struct can_frame frame, recv_data_t *recv_data)
{
    dcdc_can_id_u can_id_info = {0};
    can_id_info.id = frame.can_id;
    recv_data->index = can_id_info.can_id_info.src_addr - 1;
    if (recv_data->index >= DCDC_NUM)
    {
        return;
    }
    recv_data->float_data = frame_data_to_float(frame);
}

int32_t recv_get_mode_out_vol(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_out_cur(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_flow_limit_point(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_dc_card_temp(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_dc_input_vol(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_pfc0_vol(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_pfc1_vol(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_env_temp(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_pfc_temp(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_rated_out_pow(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_rated_out_cur(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_work_altitude(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_out_cur(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_group_num(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_addr_alloc(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_out_pwr(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_out_vol(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_limit_point(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_out_vol_max(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_on_or_off(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_over_vol_reset(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_out_over_vol_protection(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_alarm_status(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_dip_addr(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_input_pow(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_work_altitude(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_input_work_mode(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_dcdc_soft_ver(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_get_mode_pfc_soft_ver(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_input_model(Widget *widget_m, can_frame frame)
{
    return 0;
}

int32_t recv_set_mode_sc_reset(Widget *widget_m, can_frame frame)
{
    return 0;
}
