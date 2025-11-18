#ifndef DCDC_H
#define DCDC_H

#include <QObject>
#include <QDebug>
#include <iostream>
#include <list>
#include <memory>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <linux/can.h>
#include "dcdc_set.h"

class Subject;

class Observer
{
public:
    Observer(const std::string &name = "unkown"):m_name(name){}
    virtual ~Observer() = default;

    virtual void update(Subject *sbj, struct can_frame frame) = 0;
    virtual const std::string& name(){return m_name;}
private:
    std::string m_name;
};


class Subject
{
public:
    Subject(){}
    virtual ~Subject() = default;
    virtual void attach(Observer* p_obs) = 0;
    virtual void detach(Observer* p_obs) = 0;
    virtual void notify(struct can_frame frame) = 0;

protected:
    std::list<Observer *> m_observer_list;
};



class Widget;

#define MAX_LEN 8

typedef struct
{
    int id;
    std::string timestamp;
    unsigned int can_id;
    unsigned int can_len;
    unsigned int can_data[MAX_LEN];
} linestr_2_dbinfo_t;

#define DCDC_NUM (12U)
#define NORMAL_PROTNO 0x060
#define DCDC_POWER_GAIN (1000.f)

#define MY_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef float  float32_t;
typedef double float64_t;

#pragma pack(push, 1)
typedef union
{
    uint32_t id;
    struct
    {
        uint32_t group : 3;
        uint32_t src_addr : 8;
        uint32_t dst_addr : 8;
        uint32_t ptp : 1;
        uint32_t protno : 9;
        uint32_t reserved : 3;
    } can_id_info;

} dcdc_can_id_u;
#pragma pack(pop)

typedef enum
{
    DC_DC_MODEL_WINLINE = 0x01,      /* 永联 */
    DC_DC_MODEL_UUGREENPOWER = 0x02, /* 优优绿能 */
} dc_dc_model_e;

typedef enum
{
    DCDC_STATE_FAULT = 0,
    DCDC_STATE_ALARM,
    DCDC_STATE_NORMAL,
} dcdc_state_e;

#pragma pack(push, 1)
typedef struct
{
    uint8_t mode_fault : 1;              /* 模块故障 */
    uint8_t mode_protect : 1;            /* 模块保护 */
    uint8_t mode_reserve1 : 1;           /* 预留 */
    uint8_t sci_comm_alarm : 1;          /* sci故障 */
    uint8_t input_model_fault : 1;       /* 输入模式错误 */
    uint8_t mode_mismatch : 1;           /* 监控下发输入模式与实际工作模式不匹配 */
    uint8_t mode_reserve2 : 1;           /* 预留 */
    uint8_t dcdc_outpu_over_vol : 1;     /* dcdc输出过压 */
    uint8_t pfc_voltage_anomaly : 1;     /* pfc电压错误 */
    uint8_t dcdc_input_over_vol : 1;     /* 直流输入过压(原ac输入过压) */
    uint8_t mode_reserve3 : 1;           /* 预留 */
    uint8_t mode_reserve4 : 1;           /* 预留 */
    uint8_t mode_reserve5 : 1;           /* 预留 */
    uint8_t mode_reserve6 : 1;           /* 预留 */
    uint8_t dcdc_input_under_vol : 1;    /* 直流输入欠压(原ac输入欠压) */
    uint8_t mode_reserve7 : 1;           /* 预留 */
    uint8_t can_comm_lost : 1;           /* can通信故障 */
    uint8_t model_unnalance_current : 1; /* 模块不均流 */
    uint8_t mode_reserve8 : 1;           /* 预留 */
    uint8_t mode_reserve9 : 1;           /* 预留 */
    uint8_t mode_reserve10 : 1;          /* 预留 */
    uint8_t mode_reserve11 : 1;          /* 预留 */
    uint8_t switch_state : 1;            /* dcdc开关机状态 */
    uint8_t mode_power_limit : 1;        /* 模块限功率 */
    uint8_t model_temp_power_limit : 1;  /* 温度限功率 */
    uint8_t dc_limit_pow : 1;            /* 直流限功率 */
    uint8_t mode_reserve12 : 1;          /* 预留 */
    uint8_t fan_fault : 1;               /* 风扇故障 */
    uint8_t short_current : 1;           /* dcdc短路 */
    uint8_t mode_reserve13 : 1;          /* 预留 */
    uint8_t dcdc_over_temp : 1;          /* dcdc过温 */
    uint8_t output_overvoltage : 1;      /* dcdc输出过压 */
} mode_alarm_bit_state_t;
#pragma pack(pop)



typedef enum
{
    GET_MODE_OUTPUT_VOL = 0x0001,                    /* 取模块输出电压 */
    GET_MODE_OUTPUT_CUR = 0x0002,                    /* 取模块输出电流 */
    GET_MODE_LIMIT_POIINT = 0x0003,                  /* 取模块限流点 */
    GET_MODE_DC_BORAD_TEMP = 0x0004,                 /* 取模块DC板温度 */
    GET_MODE_DC_INPUT_VOL = 0x0005,                  /* 取模块输入电压 */
    GET_MODE_PFC0_VOL = 0x0008,                      /* 取模块PFC0电压 正半母线 */
    GET_MODE_PFC1_VOL = 0x000A,                      /* 取模块PFC1电压 负半母线*/
    GET_MODE_ENV_TEMP = 0x000B,                      /* 取环境温度 */
    GET_MODE_PFC_temp = 0x0010,                      /* 取模块PFC板温度 */
    GET_MODE_RATED_OUTPUT_PWR = 0x0011,              /* 取模块额定输出功率 */
    GET_MODE_RATED_OUTPUT_CUR = 0x0012,              /* 取模块额定输出电流 */
    SET_MODE_WORK_ALTITUDE = 0x0017,                 /* 设置模块工作海拔 */
    SET_MODE_OUTPUT_CUR = 0x001B,                    /* 设置模块输出电流 */
    SET_MODE_GROUP_NUM = 0x001E,                     /* 设置模块组号 */
    SET_MODE_ADDR_ALLOC_METH = 0x001F,               /* 设置模块地址分配方式 */
    SET_MODE_OUTPUT_PWR = 0x0020,                    /* 设置模块输出功率 */
    SET_MODE_OUTPUT_VOL = 0x0021,                    /* 设置模块输出电压 */
    SET_MODE_LIMIT_POINT = 0x0022,                   /* 设置模块限流点 */
    SET_MODE_OUTPUT_VOL_MAX = 0x0023,                /* 设置模块最大输出电压 */
    SET_MODE_SWITCH = 0x0030,                        /* 设置模块开关机 */
    SET_MODE_OVER_VOL_RESET = 0x0031,                /* 设置模块过压复位 */
    SET_MODE_OUT_OVER_VOL_PROTECTION_RELATED = 0x3E, /* 设置模块过压保护关联 */
    GET_MODE_ALARM_BIT = 0x0040,                     /* 取模块告警状态 */
    GET_MODE_DIP_ADDR = 0x0043,                      /* 取模块拨码地址 */
    SET_MODE_SC_RESET = 0x0044,                      /* 设置模块短路复位 */
    SET_MODE_INPUT_MODEL = 0x0046,                   /* 设置模块输入模式 */
    GET_MODE_INPUT_PWR = 0x0048,                     /* 获取模块输入功率 */
    GET_MODE_WORK_ALTITUDE = 0x004A,                 /* 获取模块工作海拔 */
    GET_MODE_INPUT_MODEL = 0x004B,                   /* 获取模块输入模式 */
    GET_MODE_DCDC_SOFTWARE_VER = 0x0056,             /* 取模块DCDC版本号 */
    GET_MODE_PFC_SOFTWARE_VER = 0x0057,              /* 取模块PFC版本号 */
    SET_MODE_FAN = 0x0033                            /* 设置dcdc风扇 */
} dcdc_func_no_type;

typedef enum
{
    DCDC_AUTO_ADDR_ALLOC = 0x00000000, /* 自动分配 */
    DCDC_DIP_ADDR_ALLOC = 0x00010000   /* 拨码分配 */
} addr_alloc_type_e;

typedef enum
{
    DCDC_TURN_ON = 0x00000000, /* 开机 */
    DCDC_TURN_OFF = 0x00010000 /* 关机 */
} switch_state_e;

typedef enum
{
    DCDC_DISABLE_OVER_VOL_RESET = 0x00000000, /* 失能过压复位 */
    DCDC_ENABLE_OVER_VOL_RESET = 0x00010000   /* 使能过压复位 */
} over_vol_reset_e;

typedef enum
{
    DCDC_DISABLE_OVER_VOL_PROTECT = 0x00010000, /* 失能过压保护关联 */
    DCDC_ENABLE_OVER_VOL_PROTECT = 0x00000000   /* 使能过压保护关联 */
} over_vol_protect_e;

typedef enum
{
    DCDC_DISABLE_SC_RESET = 0x00000000, /* 失能短路复位 */
    DCDC_ENABLE_SC_RESET = 0x00010000   /* 使能短路复位 */
} sc_reset_e;

typedef enum
{
    DCDC_SINGLE_PHASE_AC = 0x00000001, // 单相交流
    DCDC_DC = 0x00000002,              // 直流
    DCDC_THREE_PHASE_AC = 0x00000003,  // 三相交流
    DCDC_MISMATCH = 0x00000005         // 模式不匹配（相序错误）
} input_mode_e;


typedef struct dc_set_msg
{
    float32_t output_vol;                /* 输出电压 */
    float32_t output_vol_max;            /* 输出电压上限 */
    float32_t output_cur;                /* 输出电流 */
    float32_t output_pwr;                /* 输出功率 */
    switch_state_e switch_state;         /* 模块开关机 0x00010000:关机 ； 0x00000000:开机 */
    uint32_t group_num;                  /* 模块组号 */
    uint32_t work_altitude;              /* 工作海拔 单位1m*/
    over_vol_reset_e over_vol_reset;     /* 模块过压复位 0x00000000:禁止；0x00010000:复位*/
    over_vol_protect_e over_vol_protect; /* 模块过压保护 0x00000000:允许；0x00010000:禁止*/
    sc_reset_e sc_reset;                 /* 短路复位 0x00000000：禁止；0x00010000：复位 */
} dc_set_msg_t;


// ===================================================================
// X-Macro 定义：所有需要生成 get/set 的成员变量
// 格式：_(Type, Name, Comment)
// ===================================================================

#define DCDC_MEMBER_LIST(_) \
    /* 基本信息 */ \
    _(uint32_t, dcdc_id, "DCDC ID") \
    _(dc_dc_model_e, model, "DCDC 型号") \
    _(uint32_t, dip_addr, "拨码地址") \
    _(bool, comm_state, "通信状态 true=在线 false=掉线") \
    _(dcdc_state_e, dcdc_state, "DCDC 状态 0=告警 1=保护 2=正常") \
    /* 电压 */ \
    _(float32_t, input_vol, "输入电压") \
    _(float32_t, output_vol, "输出电压") \
    _(float32_t, output_vol_max, "输出电压上限") \
    _(float32_t, pfc0_vol, "正半母线电压 PFC0") \
    _(float32_t, pfc1_vol, "负半母线电压 PFC1") \
    /* 电流与功率 */ \
    _(float32_t, output_cur, "输出电流") \
    _(float32_t, rated_output_cur, "额定输出电流") \
    _(float32_t, limit_point, "限流点") \
    _(float32_t, input_pwr, "输入功率") \
    _(float32_t, output_pwr, "输出功率") \
    _(float32_t, rated_output_pwr, "额定输出功率") \
     /* 温度 */ \
     _(float32_t, env_temp, "环境温度") \
     _(float32_t, dc_board_temp, "DC板温度") \
     _(float32_t, pfc_temp, "PFC板温度") \
     /* 控制参数 */ \
    _(uint32_t, work_altitude, "工作海拔 单位1m") \
    _(uint32_t, group_num, "模块组号") \
    _(input_mode_e, input_model, "输入模式") \
    _(switch_state_e, switch_state, "开关机状态") \
    _(addr_alloc_type_e, addr_alloc_meth, "地址分配方式") \
    _(sc_reset_e, sc_reset, "短路复位") \
    _(over_vol_reset_e, over_vol_reset, "过压复位") \
    _(over_vol_protect_e, over_vol_protect, "过压保护关联") \
    /* 版本号 */ \
    _(uint32_t, dcdc_version, "DCDC 软件版本号") \
    _(uint32_t, pfc_version, "PFC 软件版本号") \
    \
    /* 告警位图（可读） */ \
    _(mode_alarm_bit_state_t, alarm_bit, "模块告警/保护位图")

#define DC_SET_MEMBER_LIST(_)\
    _(float32_t, output_vol, "设置的输出电压")\
    _(float32_t, output_vol_max, "设置的输出电压上限")\
    _(float32_t, output_cur, "设置的输出电压")\
    _(float32_t, output_pwr, "设置的功率")\
    _(switch_state_e, switch_state, "设置的模块开关机")\
    _(uint32_t, group_num, "设置的组号")\
    _(uint32_t, work_altitude, "设置的海拔")\
    _(over_vol_reset_e, over_vol_reset, "设置的过压复位")\
    _(over_vol_protect_e, over_vol_protect, "设置的过压保护")\
    _(sc_reset_e, sc_reset, "设置的短路复位")

class dcdc : public Subject
{
public:
    dcdc();
    ~dcdc();

    void attach(Observer* p_obs) override
    {
        m_observer_list.push_back(p_obs);
    }

    void detach(Observer* p_obs) override
    {
        m_observer_list.remove(p_obs);
    }


    void notify(struct can_frame frame) override
    {
        // qDebug() << "dcdc::notify() called, observer count:" << m_observer_list.size();
        for (Observer* obs : m_observer_list)
        {
            obs->update(this, frame);
        }
    }



    template <typename T>
    void update_member(T &old_data, const T &new_data)
    {
        old_data = new_data;
    }

// 为普通成员生成 getter/setter
#define MAKE_GETTER(type, name, doc) \
    type get_##name() const { return name; }
    DCDC_MEMBER_LIST(MAKE_GETTER)
#undef MAKE_GETTER

// #define MAKE_SETTER(type, name, doc) \
//     void set_##name(struct can_frame frame, const type &v) { update_member(name, v); notify(frame);}
//     DCDC_MEMBER_LIST(MAKE_SETTER)
// #undef MAKE_SETTER

    void set_dcdc_id(struct can_frame frame, const uint32_t &v)
    {
        dcdc_id = v;
    }

    void set_output_vol(struct can_frame frame, const float32_t &v)
    {
        output_vol = v;
    }

    void set_output_cur(struct can_frame frame, const float32_t &v)
    {
        output_cur = v;
    }
    void set_output_switchstate(struct can_frame frame, const switch_state_e &v)
    {
        switch_state = v;
    }

// 为 dc_set_msg 成员生成 getter/setter
#define MAKE_GETSETTER(type, name, doc) \
    type get_set_##name() const {  return dc_set_msg.name; }
    DC_SET_MEMBER_LIST(MAKE_GETSETTER)
#undef MAKE_GETSETTER

// #define MAKE_SETSETTER(type, name, doc) \
//     void set_set_##name(struct can_frame frame, const type &v) { update_member(dc_set_msg.name, v); std::cout << get_set_##name() << std::endl; notify(frame); }
//     DC_SET_MEMBER_LIST(MAKE_SETSETTER)
// #undef MAKE_SETSETTER
    void set_set_output_vol(struct can_frame frame, const float32_t &v)
    {
        dc_set_msg.output_vol = v;
    }

    void set_set_output_cur(struct can_frame frame, const float32_t &v)
    {
        dc_set_msg.output_cur = v;
    }

    void set_set_switch_state(struct can_frame frame, const switch_state_e &v)
    {
        dc_set_msg.switch_state = v;
    }



private:
#define DECLARE_MEMBER(type, name, doc) type name{};
    DCDC_MEMBER_LIST(DECLARE_MEMBER)
#undef DECLARE_MEMBER

    dc_set_msg_t dc_set_msg{}; // 值初始化为 0
};


class dcdc_manager : public QObject
{
    Q_OBJECT

public:
    dcdc_manager(QObject *parent = nullptr)
        : QObject(parent)
    {}

    Observer* get_observer() { return &m_observer; }

signals:
    void outputVolChanged(int index, double vol);
    void setoutputVolChanged(int index, double vol);
    void outputCurChanged(int index, double vol);
    void setoutputCurChanged(int index, double vol);
    void switchStateChanged(int index, double vol);
    void setswitchStateChanged(int index, double vol);

private:
    struct ManagerObserver : public Observer
    {
        explicit ManagerObserver(dcdc_manager* mgr)
            : Observer("dcdc_manager")
            , manager(mgr)
        {}

        void update(Subject* sbj, struct can_frame frame) override
        {
            if (auto* dc = dynamic_cast<dcdc*>(sbj))
            {
                uint32_t id = dc->get_dcdc_id();
                std::cout << name() << " received update from dcunit[" << id << "]" << std::endl;
                dcdc_func_no_type func_no = (dcdc_func_no_type)((frame.data[2] << 8) | frame.data[3]);
                qDebug() << Qt::hex << Qt::showbase << func_no;
                switch (func_no)
                {
                case GET_MODE_OUTPUT_VOL:
                {
                    double value = dc->get_output_vol();
                    qDebug() << "emit vol change" << value;
                    emit manager->outputVolChanged(id, value);
                    break;
                }
                case SET_MODE_OUTPUT_VOL:
                {
                    double value = dc->get_set_output_vol();
                    qDebug() << "emit setvol change" << value;
                    emit manager->setoutputVolChanged(id, value);
                    break;
                }
                case GET_MODE_OUTPUT_CUR:
                {
                    double value = dc->get_output_cur();
                    qDebug() << "emit cur change" << value;
                    emit manager->outputCurChanged(id, value);
                    break;
                }
                case SET_MODE_OUTPUT_CUR:
                {
                    double value = dc->get_set_output_cur();
                    qDebug() << "emit setcur change" << value;
                    emit manager->setoutputCurChanged(id, value);
                    break;
                }
                case SET_MODE_SWITCH:
                {
                    int value = dc->get_set_switch_state();
                    qDebug() << "emit set_switchstate change" << value;
                    emit manager->setswitchStateChanged(id, value);
                    break;
                }
                default:
                    break;
                }
            }
        }

        dcdc_manager* manager;
    };

    ManagerObserver m_observer{this};
};

// dcdc_manager *get_g_dcdc_manager();
void deal_with_frame(Widget *widget_m, struct can_frame frame);

#endif
