#include "dc_dc_recv.h"
#include "dcdc.h"

#include <string.h>
#include <arpa/inet.h>

#define DCDC_INPUT_POWER_GAIN (1000.0f)        /* dc输入功率增益 */
#define DCDC_RATED_OUTPUT_POWER_GAIN (1000.0f) /* dc额定输出功率增益 */
#define DCDC_SET_SUCCESS (0xF0)
#define DCDC_SET_FAILED (0xF2)

#define PRINT_DEBUG_GET(index, func_namestr, flag_successd, value)\
    do\
    {\
        std::cout << "dcdc[" << index << "] " << func_namestr << (flag_successd?" successed":" failed") << ": " << value << std::endl;\
    } while (0)

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

/* 收到模块PFC版本号返回值 整型 */
int32_t recv_get_mode_pfc_soft_ver(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag  = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        msg[recv_data.index].set_pfc_version((frame.data[6] << 8) | (frame.data[7]));
        flag = true;
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_pfc_version());

    // log_d("g_dcdc_info %d %s %d", recv_data.index + 1, __FUNCTION__, msg[recv_data.index].dc_unit_focus.pfc_version);
    return 1;
}

/* 收到模块软件版本号返回值 整型 */
int32_t recv_get_mode_dcdc_soft_ver(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag  = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        msg[recv_data.index].set_dcdc_version((frame.data[6] << 8) | (frame.data[7]));
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_dcdc_version());
    // log_d("g_dcdc_info %d %s %d", recv_data.index + 1, __FUNCTION__, msg[recv_data.index].dc_unit_focus.dcdc_version);
    return 1;
}

/* 收到获取模块输入工作模式返回值 整型*/
int32_t recv_get_mode_input_work_mode(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        msg[recv_data.index].set_input_model((input_mode_e)recv_data.data);
    }
    return 1;
}

/* 收到获取模块当前工作海拔返回值 整型 */
int32_t recv_get_mode_work_altitude(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag  = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        msg[recv_data.index].set_work_altitude(recv_data.data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_work_altitude());
    return 1;
}

/* 收到获取模块输入功率返回值 整型 */
int32_t recv_get_mode_input_pow(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag  = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_input_pwr(recv_data.float_data / DCDC_INPUT_POWER_GAIN);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_input_pwr());
    // if (DCDC_TURN_ON == msg[recv_data.index].switch_state)
    // {
    //     log_d("dcdc[%d] %s %f", recv_data.index+1, __FUNCTION__, msg[recv_data.index].input_pwr);
    // }
    // log_d("g_dcdc_info %d recv_get_mode_input_pow %f", recv_data.index+1, msg[recv_data.index].input_pwr);
    return 1;
}

/* 收到获取模块拨码地址返回值 整型 byte4 ~ 5 组号, byte6 ~ 7地址*/
int32_t recv_get_mode_dip_addr(struct can_frame frame)
{
    dcdc_can_id_u can_id_info = {0};
    can_id_info.id = frame.can_id;
    uint32_t index = can_id_info.can_id_info.src_addr - 1;
    if (index >= DCDC_NUM)
    {
        return -1;
    }
    dcdc *msg = get_g_dcdc_info();
    bool flag  = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[index].set_dip_addr((uint32_t)(((uint16_t)frame.data[6] << 8) | frame.data[7]));
        msg[index].set_group_num((uint32_t)(((uint16_t)frame.data[4] << 8) | frame.data[5]));
    }
    PRINT_DEBUG_GET(index + 1, __func__, flag, msg->get_dip_addr());
    PRINT_DEBUG_GET(index + 1, __func__, flag, msg->get_group_num());
    // log_d("can_id %u, g_dcdc_info[%u] recv_get_mode_dip_addr: dip_addr=%u, group_num=%u",
    //             can_id, index + 1, msg[index].dip_addr, msg[index].group_num);
    return 1;
}

static void print_binary_32(uint32_t val, char *buf, uint32_t len)
{
    if (len < 37U)
    {
        return; // 需要 32 位 + 7 个 '_' + '\0'
    }

    for (uint8_t i = 0U; i < 32U; i++)
    {
        int bit = (val >> (31U - i)) & 1U;
        buf[i + i / 4U] = bit ? '1' : '0'; // 每 4 位插入 '_'
        if ((i % 4U == 3U) && (i < 31U))
        {
            buf[i + i / 4U + 1U] = '_';
        }
    }
    buf[36] = '\0';
}

/* 收到获取模块当前告警状态返回值 整型 */
int32_t recv_get_mode_alarm_status(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    switch_state_e last_switch_state = msg[recv_data.index].get_switch_state();
    bool flag  = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        mode_alarm_bit_state_t alarm_bit = {0};
        (void)memcpy(&alarm_bit, &recv_data.data, sizeof(uint32_t));
        msg[recv_data.index].set_alarm_bit(alarm_bit);
        if (0U == msg[recv_data.index].get_alarm_bit().switch_state)
        {
            msg[recv_data.index].set_switch_state(DCDC_TURN_ON);
            PRINT_DEBUG_GET(recv_data.index + 1, "recv switch_state", flag, msg->get_switch_state());
        }
        else
        {
            msg[recv_data.index].set_switch_state(DCDC_TURN_OFF);
            PRINT_DEBUG_GET(recv_data.index + 1, "recv switch_state", flag, msg->get_switch_state());
        }
    }


    // if (last_switch_state != msg[recv_data.index].get_switch_state())
    // {
    //     log_d("dcdc[%d] last_switch_state:%s now_switch_state:%s", recv_data.index + 1,
    //         (DCDC_TURN_ON == last_switch_state) ? "DCDC_TURN_ON" : (DCDC_TURN_OFF == last_switch_state) ? "DCDC_TURN_OFF" : "SWITCH_ERROR",
    //         (DCDC_TURN_ON == msg[recv_data.index].switch_state) ? "DCDC_TURN_ON" : (DCDC_TURN_OFF == msg[recv_data.index].switch_state) ? "DCDC_TURN_OFF" : "SWITCH_ERROR");
    // }

    if (1U == msg[recv_data.index].get_alarm_bit().mode_fault)
    {
        msg[recv_data.index].set_dcdc_state( DCDC_STATE_FAULT);
        PRINT_DEBUG_GET(recv_data.index + 1, "recv dcdc_state", flag, msg->get_dcdc_state());
        // char bin_str[37];
        // print_binary_32(recv_data.data, bin_str, sizeof(bin_str));
        // log_e("dcdc[%d] state: DCDC_STATE_FAULT, alarm bit: %s", recv_data.index + 1, bin_str);
    }
    else if ((0U == msg[recv_data.index].get_alarm_bit().mode_fault) && (1U == msg[recv_data.index].get_alarm_bit().mode_protect))
    {
        msg[recv_data.index].set_dcdc_state(DCDC_STATE_ALARM);
        PRINT_DEBUG_GET(recv_data.index + 1, "recv dcdc_state", flag, msg->get_dcdc_state());
        // char bin_str[37];
        // print_binary_32(recv_data.data, bin_str, sizeof(bin_str));
        // log_e("dcdc[%d] state: DCDC_STATE_ALARM, alarm bit: %s", recv_data.index + 1, bin_str);
    }
    else
    {
        msg[recv_data.index].set_dcdc_state(DCDC_STATE_NORMAL);
        PRINT_DEBUG_GET(recv_data.index + 1, "recv dcdc_state", flag, msg->get_dcdc_state());
    }

    return 1;
}


/* 收到获取模块额定输出电流返回值 浮点型 */
int32_t recv_get_mode_rated_out_cur(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_rated_output_cur(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_rated_output_cur());
    // log_d("g_dcdc_info %d recv_get_mode_rated_out_cur %f", index+1, g_dcdc_info[index].rated_output_cur);
    return 1;
}

/* 收到获取模块额定输出功率返回值 浮点型 */
int32_t recv_get_mode_rated_out_pow(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_rated_output_pwr(recv_data.float_data / DCDC_RATED_OUTPUT_POWER_GAIN);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_rated_output_pwr());
    // log_d("g_dcdc_info %d recv_get_mode_rated_out_pow %f", recv_data.index+1, g_dcdc_info[recv_data.index].rated_output_pwr);
    return 1;
}

/* 收到获取模块pfc温度返回值 浮点型 */
int32_t recv_get_mode_pfc_temp(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_pfc_temp(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_pfc_temp());
    // log_d("g_dcdc_info %d recv_get_mode_pfc_temp %f", index+1, g_dcdc_info[index].pfc_temp);
    return 1;
}

/* 收到获取模块环境温度返回值 浮点型 */
int32_t recv_get_mode_env_temp(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_env_temp(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_pfc_temp());
    // log_d("g_dcdc_info %d recv_get_mode_env_temp %f", index+1, g_dcdc_info[index].env_temp);
    return 1;
}

/* 收到获取模块pfc1电压返回值 浮点型 */
int32_t recv_get_mode_pfc1_vol(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_pfc1_vol(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_pfc1_vol());
    // log_d("g_dcdc_info %d recv_get_mode_pfc1_vol %f", index+1, g_dcdc_info[index].pfc1_vol);
    return 1;
}

/* 收到获取模块pfc0电压返回值 浮点型 */
int32_t recv_get_mode_pfc0_vol(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_pfc0_vol(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_pfc0_vol());
    // log_d("g_dcdc_info %d recv_get_mode_pfc0_vol %f", index+1, g_dcdc_info[index].pfc0_vol);
    return 1;
}

/* 收到获取模块直流输入电压返回值 浮点型 */
int32_t recv_get_mode_dc_input_vol(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_input_vol(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_input_vol());
    // if (DCDC_TURN_ON == msg[recv_data.index].switch_state)
    // {
    //     log_d("dcdc[%d] %s %f", recv_data.index+1, __FUNCTION__, msg[recv_data.index].input_vol);
    // }
    return 1;
}

/* 收到获取模块dc板返回值 浮点型 */
int32_t recv_get_mode_dc_card_temp(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_dc_board_temp(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_dc_board_temp());
    // log_d("g_dcdc_info %d recv_get_mode_dc_card_temp %f", index+1, g_dcdc_info[index].dc_board_temp);
    return 1;
}

/* 收到获取模块限流点返回值 浮点型 */
int32_t recv_get_mode_flow_limit_point(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_limit_point(recv_data.float_data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_limit_point());
    // log_d("g_dcdc_info %d recv_get_mode_flow_limit_point %f", index+1, g_dcdc_info[index].limit_point);
    return 1;
}

/* 收到获取模块电流返回值 浮点型 */
int32_t recv_get_mode_out_cur(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_output_cur(recv_data.float_data);
        msg[recv_data.index].set_output_pwr((msg[recv_data.index].get_output_vol() * msg[recv_data.index].get_output_cur()) / DCDC_POWER_GAIN);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_output_cur());
    PRINT_DEBUG_GET(recv_data.index + 1, "recv output", flag, msg->get_output_pwr());

    // if (DCDC_TURN_ON == msg[recv_data.index].switch_state)
    // {
    //     log_d("dcdc[%d] %s %f", recv_data.index+1, __FUNCTION__, msg[recv_data.index].output_cur);
    // }
    return 1;
}

/* 收到获取模块输出电压返回值 浮点型 */
int32_t recv_get_mode_out_vol(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_output_vol(recv_data.float_data);
        msg[recv_data.index].set_output_pwr((msg[recv_data.index].get_output_vol() * msg[recv_data.index].get_output_cur()) / DCDC_POWER_GAIN);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_output_vol());
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_output_pwr());

    // if (DCDC_TURN_ON == msg[recv_data.index].switch_state)
    // {
    //     log_d("dcdc[%d] %s %f", recv_data.index+1, __FUNCTION__, msg[recv_data.index].output_vol);
    // }
    return 1;
}


/* 收到获取模块短路复位 整型 */
int32_t recv_set_mode_sc_reset(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_sc_reset((sc_reset_e)recv_data.data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_sc_reset());
    // log_d("dcdc[%d] %s %d", index+1, __FUNCTION__, msg[index].sc_reset);
    return 1;
}


/* 收到获取模块输入模式 整型 */
int32_t recv_set_mode_input_model(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_input_model((input_mode_e)recv_data.data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_input_model());
    // log_d("dcdc[%d] %s %s %d", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failue"), msg[recv_data.index].input_model);
    return 1;
}

/* 收到设置模块输出过压保护关联是否允许回复 整型 */
int32_t recv_set_mode_out_over_vol_protection(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_over_vol_protect((over_vol_protect_e)recv_data.data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_over_vol_protect());
    // log_d("dcdc[%d] %s %d", recv_data.index+1, __FUNCTION__, msg[recv_data.index].over_vol_protect);
    return 1;
}

/* 收到设置模块过压复位回复 整型 */
int32_t recv_set_mode_over_vol_reset(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg[recv_data.index].set_over_vol_reset((over_vol_reset_e)recv_data.data);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg->get_over_vol_reset());
    // log_d("dcdc[%d] %s %d", recv_data.index+1, __FUNCTION__, msg[recv_data.index].over_vol_reset);
    return 1;
}

/* 收到设置模块开关机回复 整型 */
int32_t recv_set_mode_on_or_off(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc msg;
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        switch_state_e switch_state;
        (void)memcpy(&switch_state, &recv_data.data, sizeof(uint32_t));
        msg.set_switch_state(switch_state);
    }
    PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg.get_switch_state());
    // log_d("dcdc[%d] %s %s %s recv:%d", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"),
    //                                      (msg.switch_state == DCDC_TURN_ON) ? "ON" : (msg.switch_state == DCDC_TURN_OFF) ? "OFF" : "UNKNOWN", recv_data.data);
    // log_d("dcdc[%d] %s [%d]", recv_data.index+1, __FUNCTION__, msg[recv_data.index].switch_state);
    return 1;
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

/* 收到设置模块输出电压上限值回复 浮点型 */
int32_t recv_set_mode_out_vol_max(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc msg{};
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg.set_output_vol_max(recv_data.float_data);
    }
    // PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg.get_output_vol_max());
    // log_d("dcdc[%d] %s %f", recv_data.index+1, __FUNCTION__, msg[recv_data.index].output_vol_max);
    // log_d("dcdc[%d] %s %s %f", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"), recv_data.float_data);
    return 1;
}

/* 收到设置模块限流点回复 浮点型 */
int32_t recv_set_mode_limit_point(struct can_frame frame)
{
    return 1;
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc msg{};
    bool flag = false;
    if (DCDC_SET_SUCCESS == frame.data[1])
    {
        flag = true;
        msg.set_limit_point(recv_data.float_data);
    }
    // PRINT_DEBUG_GET(recv_data.index + 1, __func__, flag, msg.get_limit_point());
    // log_d("g_dcdc_info %d recv_set_mode_limit_point %f", index+1, g_dcdc_info[index].limit_point);
    return 1;
}

/* 收到设置模块输出电压回复 浮点型 */
int32_t recv_set_mode_out_vol(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    dcdc msg{};
    msg.set_set_output_vol(recv_data.float_data);
    // PRINT_DEBUG_GET(recv_data.index + 1, __func__, true, msg.get_set_output_vol());
    // log_d("dcdc[%d] %s %s %f", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"), recv_data.float_data);

    return 1;
}

/* 收到设置模块输出功率回复 浮点型*/
int32_t recv_set_mode_out_pwr(struct can_frame frame)
{
    return 1;
    recv_data_t recv_data = {0};
    recv_float_data_pre(frame, &recv_data);
    // log_d("dcdc[%d] %s %s %f", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"), recv_data.float_data);
    // log_d("dcdc[%d] %s %f", recv_data.index+1, __FUNCTION__, msg[recv_data.index].output_pwr);
    // log_d("dcdc[%d] %s %s %f", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"), msg[recv_data.index].output_pwr);

    return 1;
}

/* 收到设置模块地址分配方式回复 整型 */
int32_t recv_set_mode_addr_alloc(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    dcdc *msg = get_g_dcdc_info();
    // (void)memcpy(&msg[recv_data.index].addr_alloc_meth, &recv_data.data, sizeof(uint32_t));
    // log_d("dcdc[%d] %s %d", recv_data.index+1, __FUNCTION__, msg[recv_data.index].addr_alloc_meth);
    return 1;
}

/* 收到设置模块组号回复 整型 */
int32_t recv_set_mode_group_num(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    // log_d("dcdc[%d] %s %d", recv_data.index+1, __FUNCTION__, msg[recv_data.index].group_num);
    // dcdc *msg = get_g_dcdc_info();
    // if (DCDC_SET_SUCCESS == frame.data[1])
    // {
    //     (void)memcpy(&msg[recv_data.index].group_num, &recv_data.data, sizeof(uint32_t));
    // }
    // log_d("dcdc[%d] %s %s %d", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"), recv_data.data);

    // for (uint8_t i = 0; i < frame.can_dlc; i++)
    // {
    //     log_d("data:%02x",frame.data[i]);
    // }
    return 1;
}

/* 收到设置模块输出电流回复 整型 */
int32_t recv_set_mode_out_cur(struct can_frame frame)
{
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    // log_d("dcdc[%d] %s %f", recv_data.index+1, __FUNCTION__, msg[recv_data.index].output_cur);
    // log_d("dcdc[%d] %s %s %f", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"), recv_data.data);

    return 1;
}

/* 收到设置模块工作海拔回复 整型 */
int32_t recv_set_mode_work_altitude(struct can_frame frame)
{
    return 1;
    recv_data_t recv_data = {0};
    recv_data_pre(frame, &recv_data);
    // dcdc *msg = get_g_dcdc_info();
    // if (DCDC_SET_SUCCESS == frame.data[1])
    // {
    //     (void)memcpy(&msg[recv_data.index].work_altitude, &recv_data.data, sizeof(uint32_t));
    // }
    // log_d("dcdc[%d] %s %s %f", recv_data.index + 1, __FUNCTION__, (frame.data[1] == DCDC_SET_SUCCESS ? "success" : "failure"), recv_data.data);
    // log_d("dcdc[%d] %s %d", recv_data.index+1, __FUNCTION__, msg[recv_data.index].work_altitude);
    return 1;
}