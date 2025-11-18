#include "dcdc.h"
#include "dc_dc_recv.h"
#include "widget.h"

#include <fstream>
#include <string>
#include <regex>
#include <string>

#include <linux/can.h>

dcdc::dcdc()
    : dcdc_id(0)
    , model(DC_DC_MODEL_WINLINE)           // 默认永联
    , dip_addr(0)
    , comm_state(false)                    // 默认掉线
    , dcdc_state(DCDC_STATE_FAULT)         // 安全起见，初始为故障
    , input_vol(0.0f)
    , output_vol(0.0f)
    , output_vol_max(0.0f)
    , output_cur(0.0f)
    , rated_output_cur(0.0f)
    , limit_point(0.0f)
    , input_pwr(0.0f)
    , output_pwr(0.0f)
    , rated_output_pwr(0.0f)
    , pfc0_vol(0.0f)
    , pfc1_vol(0.0f)
    , env_temp(0.0f)
    , dc_board_temp(0.0f)
    , pfc_temp(0.0f)
    , work_altitude(0)
    , alarm_bit{}                         // 零初始化位域结构体
    , group_num(0)
    , input_model(DCDC_MISMATCH)          // 初始为“模式不匹配”
    , switch_state(DCDC_TURN_OFF)         // 默认关机
    , addr_alloc_meth(DCDC_DIP_ADDR_ALLOC)// 默认拨码分配
    , sc_reset(DCDC_DISABLE_SC_RESET)
    , over_vol_reset(DCDC_DISABLE_OVER_VOL_RESET)
    , over_vol_protect(DCDC_ENABLE_OVER_VOL_PROTECT)
    , dc_set_msg{}                        // 零初始化结构体
{
    // 如果 alarm_bit 和 dc_set_msg 有复杂逻辑，可在此补充
    // 但当前已通过 {} 初始化为 0，安全
};

dcdc::~dcdc()
{
}

typedef struct
{
    uint32_t func_no;
    int32_t (*on_recv_func)(Widget *widget_m, struct can_frame frame);
} dc_recv_func_map_t;
static dc_recv_func_map_t g_dc_recv_func_map[] = {
    {.func_no = GET_MODE_OUTPUT_VOL,                      .on_recv_func = &recv_get_mode_out_vol},
    {.func_no = GET_MODE_OUTPUT_CUR,                      .on_recv_func = &recv_get_mode_out_cur},
    {.func_no = GET_MODE_LIMIT_POIINT,                    .on_recv_func = &recv_get_mode_flow_limit_point},
    {.func_no = GET_MODE_DC_BORAD_TEMP,                   .on_recv_func = &recv_get_mode_dc_card_temp},
    {.func_no = GET_MODE_DC_INPUT_VOL,                    .on_recv_func = &recv_get_mode_dc_input_vol},
    {.func_no = GET_MODE_PFC0_VOL,                        .on_recv_func = &recv_get_mode_pfc0_vol},
    {.func_no = GET_MODE_PFC1_VOL,                        .on_recv_func = &recv_get_mode_pfc1_vol},
    {.func_no = GET_MODE_ENV_TEMP,                        .on_recv_func = &recv_get_mode_env_temp},
    {.func_no = GET_MODE_PFC_temp,                        .on_recv_func = &recv_get_mode_pfc_temp},
    {.func_no = GET_MODE_RATED_OUTPUT_PWR,                .on_recv_func = &recv_get_mode_rated_out_pow},
    {.func_no = GET_MODE_RATED_OUTPUT_CUR,                .on_recv_func = &recv_get_mode_rated_out_cur},
    {.func_no = SET_MODE_WORK_ALTITUDE,                   .on_recv_func = &recv_set_mode_work_altitude},
    {.func_no = SET_MODE_OUTPUT_CUR,                      .on_recv_func = &recv_set_mode_out_cur},
    {.func_no = SET_MODE_GROUP_NUM,                       .on_recv_func = &recv_set_mode_group_num},
    {.func_no = SET_MODE_ADDR_ALLOC_METH,                 .on_recv_func = &recv_set_mode_addr_alloc},
    {.func_no = SET_MODE_OUTPUT_PWR,                      .on_recv_func = &recv_set_mode_out_pwr},
    {.func_no = SET_MODE_OUTPUT_VOL,                      .on_recv_func = &recv_set_mode_out_vol},
    {.func_no = SET_MODE_LIMIT_POINT,                     .on_recv_func = &recv_set_mode_limit_point},
    {.func_no = SET_MODE_OUTPUT_VOL_MAX,                  .on_recv_func = &recv_set_mode_out_vol_max},
    {.func_no = SET_MODE_SWITCH,                          .on_recv_func = &recv_set_mode_on_or_off},
    {.func_no = SET_MODE_OVER_VOL_RESET,                  .on_recv_func = &recv_set_mode_over_vol_reset},
    {.func_no = SET_MODE_OUT_OVER_VOL_PROTECTION_RELATED, .on_recv_func = &recv_set_mode_out_over_vol_protection},
    {.func_no = GET_MODE_ALARM_BIT,                       .on_recv_func = &recv_get_mode_alarm_status},
    {.func_no = GET_MODE_DIP_ADDR,                        .on_recv_func = &recv_get_mode_dip_addr},
    {.func_no = SET_MODE_SC_RESET,                        .on_recv_func = &recv_set_mode_sc_reset},
    {.func_no = SET_MODE_INPUT_MODEL,                     .on_recv_func = &recv_set_mode_input_model},
    {.func_no = GET_MODE_INPUT_PWR,                       .on_recv_func = &recv_get_mode_input_pow},
    {.func_no = GET_MODE_WORK_ALTITUDE,                   .on_recv_func = &recv_get_mode_work_altitude},
    {.func_no = GET_MODE_INPUT_MODEL,                     .on_recv_func = &recv_get_mode_input_work_mode},
    {.func_no = GET_MODE_DCDC_SOFTWARE_VER,               .on_recv_func = &recv_get_mode_dcdc_soft_ver},
    {.func_no = GET_MODE_PFC_SOFTWARE_VER,                .on_recv_func = &recv_get_mode_pfc_soft_ver}
};


static bool find_dcdc_func_no(uint32_t func_no, uint32_t *index)
{
    for (uint32_t i = 0; i < MY_ARRAY_SIZE(g_dc_recv_func_map); i++)
    {
        // log_d("g_dc_recv_func_map[%d].func_no = %x, func_no = %x", i, g_dc_recv_func_map[i].func_no, func_no);
        if (g_dc_recv_func_map[i].func_no == func_no)
        {
            *index = i;
            return true;
        }
    }
    return false;
}


void deal_with_frame(Widget *widget_m, struct can_frame frame)
{
    dcdc_can_id_u can_id_info = {0};
    can_id_info.id = frame.can_id;
    if (can_id_info.can_id_info.protno != NORMAL_PROTNO) /* 只通过0x060开头的报文 */
    {
        return;
    }

    if (can_id_info.can_id_info.src_addr == 0xF0) /* 发送报文 */
    {
        // print_can_frame(frame);
        dcdc_func_no_type func_no = (dcdc_func_no_type)((frame.data[2] << 8) | frame.data[3]);
        qDebug() << Qt::hex <<  Qt::showbase << func_no;
        switch (func_no)
        {
        case SET_MODE_WORK_ALTITUDE:
            frame2set_output_vol(widget_m, frame);
            break;
        case SET_MODE_OUTPUT_CUR:
            frame2set_output_cur(widget_m, frame);
            break;
        case SET_MODE_GROUP_NUM:
            frame2set_group_num(widget_m, frame);
            break;
        case SET_MODE_ADDR_ALLOC_METH:
            break;
        case SET_MODE_OUTPUT_PWR:
            frame2set_output_pwr(widget_m, frame);
            break;
        case SET_MODE_OUTPUT_VOL:
            frame2set_output_vol(widget_m, frame);
            break;
        case SET_MODE_OUTPUT_VOL_MAX:
            frame2set_output_vol_max(widget_m, frame);
            break;
        case SET_MODE_SWITCH:
            frame2set_switch_state(widget_m, frame);
            break;
        case SET_MODE_OVER_VOL_RESET:
            frame2set_over_vol_reset(widget_m, frame);
            break;
        case SET_MODE_OUT_OVER_VOL_PROTECTION_RELATED:
            frame2set_over_vol_protect(widget_m, frame);
            break;
        case SET_MODE_SC_RESET:
            frame2set_sc_reset(widget_m, frame);
            break;
        default:
            break;
        }
    }
    else if (can_id_info.can_id_info.dst_addr == 0xF0) /* 接收报文 */
    {
        if (0xF0!=frame.data[1])
        {
            return;
        }
        else
        {
            uint16_t func_no = (uint16_t)((((uint16_t)frame.data[2])<<8) | ((uint16_t)frame.data[3]));
            uint32_t func_index = 0;
            if (!find_dcdc_func_no(func_no, &func_index))
            {
                return;
            }
            else
            {
                if (func_index >= MY_ARRAY_SIZE(g_dc_recv_func_map))
                {
                    return;
                }
                else
                {
                    (void)g_dc_recv_func_map[func_index].on_recv_func(widget_m, frame);
                    return;
                }
            }
        }
    }
    else
    {
        return;
    }

}

void test(void)
{

    // std::ifstream file("bc.log");
    // std::string line;

    // if (!file.is_open())
    // {
    //     std::cout << "open error" << std::endl;
    //     return;
    // }

    struct can_frame frame;
    // while(std::getline(file, line))
    // {
    //     std::regex re(R"zzz(\((\d{4}-\d{2}-\d{2})\s+(\d{2}:\d{2}:\d{2}\.\d+)\)\s+(\w+)\s+([0-9a-fA-F]+)\s+\[\d\]\s+(([0-9a-fA-F]{2}(?:\s+[0-9a-fA-F]{2})*)?))zzz");
    //     std::smatch match;
    //     if (std::regex_search(line, match, re))
    //     {
    //         std::string datastr = match[1];
    //         std::string timestr = match[2];
    //         std::string can_idstr = match[4];
    //         std::string can_data = match[5];
    //         // std::cout << datastr << " "<< timestr << ":0x"<< can_idstr << " "<< can_data << std::endl;
    //         struct can_frame frame = {0};
    //         frame.can_id = std::stoi(can_idstr, nullptr, 16);

    //         std::istringstream iss(can_data);
    //         std::string byteStr;
    //         int index = 0;
    //         bool frame_valid = true;
    //         while (std::getline(iss, byteStr, ' ') && index < 8)
    //         {
    //             if (byteStr.empty())
    //             {
    //                 continue;
    //             }
    //             else
    //             {
    //                 try
    //                 {
    //                     frame.data[index] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
    //                 } catch (const std::invalid_argument & e)
    //                 {
    //                     std::cerr << " invalid argument" << e.what() << std::endl;
    //                     frame_valid = false;
    //                     break;  // 立即停止解析
    //                 } catch (const std::out_of_range & e)
    //                 {
    //                     std::cerr << "out of range " << e.what() << std::endl;
    //                     frame_valid = false;
    //                     break;  // 立即停止解析
    //                 }
    //             }
    //             ++index;
    //         }

    //         if (frame_valid && 7 == index)
    //         {
    //             frame.can_dlc = 8;
    //             deal_with_frame(frame);
    //         }
    //     }
    //     else
    //     {
    //         std::cout << "regex error:" << line <<std::endl;
    //     }
    // }
    // file.close();

    frame.can_id = 0x860F8019;
    frame.can_dlc = 8;
    // 41 F0 00 05 43 F9 88 00
    frame.data[0] = 0x41;
    frame.data[1] = 0xF0;
    frame.data[2] = 0x00;
    frame.data[3] = 0x05;
    frame.data[4] = 0x43;
    frame.data[5] = 0xf9;
    frame.data[6] = 0x88;
    frame.data[7] = 0x00;
    dcdc_can_id_u canID{};
    canID.id = frame.can_id;

    std::cout << "group: " << canID.can_id_info.group << std::endl;      // 1
    std::cout << "src_addr: " << canID.can_id_info.src_addr << std::endl; // 0
    std::cout << "dst_addr: " << canID.can_id_info.dst_addr << std::endl; // 15 (0x0F)
    std::cout << "ptp: " << canID.can_id_info.ptp << std::endl;           // 1
    std::cout << "protno: " << canID.can_id_info.protno << std::endl;     // 24
    std::cout << "reserved: " << canID.can_id_info.reserved << std::endl; // 4

    // deal_with_frame(frame);
    // frame.data[3] = 0x23;
    // deal_with_frame(frame);
    // dcdc_can_id_u canId{};
    // canId.id = frame.can_id;
    // std::cout << "dc[" << canId.can_id_info.dst_addr << "]" << g_dcdc[canId.can_id_info.dst_addr - 1].get_output_vol() <<" " << g_dcdc[canId.can_id_info.dst_addr - 1].get_set_output_vol() << std::endl;
}

/* g++ dcdc.cpp dcdc.h dc_dc_recv.cpp dc_dc_recv.h dcdc_set.cpp dcdc_set.h -g -o dcdc && ./dcdc */
// static void ssss(struct can_frame frame)
// {
//     printf("hello world");
// }
// int main(void)
// {
//     // test();
//     ssss();
//     return 0;
// }

