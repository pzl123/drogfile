#include "dcdc_set.h"
#include "dc_dc_recv.h"

#include <linux/can.h>
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

class float_extractor
{
private:
    float32_t value;
public:
    void operator()(struct can_frame frame, recv_data_t *data)
    {
        recv_float_data_pre(frame, data);
        value = data->float_data;
    }
    float32_t get_value() { return value;}
};

class uint32_extractor
{
private:
    uint32_t value;
public:
    void operator()(struct can_frame frame, recv_data_t *data)
    {
        recv_data_pre(frame, data);
        value = data->data;
    }
    uint32_t get_value() { return value;}
};

template<typename EnumType>
class enum_extractor
{
private:
    EnumType value;
public:
    void operator()(struct can_frame frame, recv_data_t *data)
    {
        recv_data_pre(frame, data);
        value = static_cast<EnumType>(data->data);
    }
    EnumType get_value() { return value;}
};

template<typename SetterFunc, typename Extractor>
void generic_set_value(struct can_frame frame, SetterFunc setter, Extractor extractor)
{
    uint32_t index = frame_from_dcdc_index(frame);
    dcdc *dcdc = get_g_dcdc_info();
    recv_data_t recv_data = {0};
    extractor(frame, &recv_data);
    auto value = extractor.get_value();
    (dcdc[index].*setter)(value);
}

#define PRINT_SET_INFO(set_func) \
    void print_debug(struct can_frame frame)\
    {\
        print_can_frame(FRAME); \
        dcdc_can_id_u can_id;\
        can_id.id = frame.can_id;\
        dcdc *dc = get_g_dcdc_info();\
        std::cout << "dcdc[" << can_id.can_id_info.dst_addr << "]" << __func__ << ":" <<  dc[can_id.can_id_info.dst_addr - 1].get_output_vol() << std::endl;\
    }

void set_output_vol(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_output_vol, float_extractor{});
    print_can_frame(frame);
    dcdc_can_id_u can_id;
    can_id.id = frame.can_id;
    dcdc *dc = get_g_dcdc_info();
    std::cout << "dcdc[" << can_id.can_id_info.dst_addr << "]" << __func__ << ":" <<  dc[can_id.can_id_info.dst_addr - 1].get_output_vol() << std::endl;
}

void set_output_vol_max(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_output_vol_max, float_extractor{});
}
void set_output_cur(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_output_cur, float_extractor{});
}
void set_output_pwr(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_output_pwr, float_extractor{});
}
void set_switch_state(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_switch_state, enum_extractor<switch_state_e>{});
}
void set_group_num(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_group_num, uint32_extractor{});
}
void set_work_altitude(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_work_altitude, uint32_extractor{});
}
void set_over_vol_reset(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_over_vol_reset, enum_extractor<over_vol_reset_e>{});
}
void set_over_vol_protect(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_over_vol_protect, enum_extractor<over_vol_protect_e>{});
}
void set_sc_reset(struct can_frame frame)
{
    generic_set_value(frame, &dcdc::set_sc_reset, enum_extractor<sc_reset_e>{});
}
