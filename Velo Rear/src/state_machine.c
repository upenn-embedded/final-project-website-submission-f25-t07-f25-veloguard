#include "state_machine.h"
#include "config.h"

static rear_state_t st;

void state_machine_init(void)
{
    st.turn = TURN_NONE;
    st.brake = BRAKE_IDLE;
}

void state_machine_update(float dist, rear_turn_mode_t turn,
                          bool link_ok, bool decel)
{
    st.turn = turn;     // 转向灯单独处理

    // 最高优先级：蓝牙断连
    if (!link_ok) {
        st.brake = BRAKE_LINK_LOSS;
        return;
    }

    // 距离很近：蜂鸣 + 刹车灯
    if (dist <= D_ALERT) {
        st.brake = BRAKE_BURST;      // 状态 3
    }
    // 距离稍远，但已经减速：只亮刹车灯
    else if (decel) {
        st.brake = BRAKE_STEADY;     // 状态 1
    }
    // 距离在 [D_ALERT, D_WARN] 之间，可以当成一个 warning 状态
    else if (dist <= D_WARN) {
        st.brake = BRAKE_WARNING;    // 选做
    }
    // 距离安全 && 没减速：灯灭
    else {
        st.brake = BRAKE_IDLE;       // 状态 2
    }
}

rear_state_t state_machine_get(void)
{
    return st;
}
