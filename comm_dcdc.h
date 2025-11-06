#ifndef COMM_DCDC_H
#define COMM_DCDC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

typedef union
{
    uint32_t id;
#ifdef WINLIN
    struct
    {
        uint32_t group : 3;
        uint32_t src_addr : 8;
        uint32_t dst_addr : 8;
        uint32_t ptp : 1;
        uint32_t protno : 9;
        uint32_t reserved : 3;
    } winlin_id_struct;
#elif UUGREENPOWER
    struct
    {
        uint32_t serial_num_low_part : 9; /* 可选 默认填0 */
        uint32_t production_day : 5; /* 可选 01- 31 */
        uint32_t module_addr : 7; /* 0x01 ~ 0x0F 0x00表示广播 */
        uint32_t monitor_addr : 4;  /* 0x01 ~ 0x0F 默认0x01 0x00表示广播 */
        uint32_t protocol : 4;
        uint32_t reserved : 3;
    } uugreenpower;
#else

#endif
} dcdc_can_id_u;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* COMM_DCDC_H */
