#include <asio_cube.hpp>
#include <functional>
#include <iostream>
#include <map>

using namespace std;

typedef struct TimerItem_st
{
    asio::steady_timer timer;
    int ns;
} TimerItem;

static int timerId = 0;
static map<int, TimerItem *> timers;

int addTimer(TimerItem *timer)
{
    int tid = timerId;
    timerId++;
    timers[tid] = timer;
    return tid;
}

TimerItem *getTimer(int tid)
{
    if (timers.find(tid) == timers.end())
        return NULL;
    return timers[tid];
}

static void handle_wait(const asio::error_code &error, int tid, cube_native_var *handler)
{
    if (!error)
    {
        if (handler != NULL)
        {
            cube_native_var *args = NATIVE_LIST();
            CALL_NATIVE_FUNC(handler, args);
        }
    }
}

static void handle_repeat(const asio::error_code &error, int tid, int count, cube_native_var *handler)
{
    if (!error)
    {
        if (handler != NULL)
        {
            cube_native_var *args = NATIVE_LIST();
            ADD_NATIVE_LIST(args, NATIVE_NUMBER(count));
            CALL_NATIVE_FUNC(handler, args);
        }
    }

    TimerItem *timer = getTimer(tid);
    if (timer)
    {
        timer->timer.expires_at(timer->timer.expires_at() + asio::chrono::nanoseconds(timer->ns));
        timer->timer.async_wait(std::bind(handle_repeat, std::placeholders::_1, tid, count + 1, handler));
    }
}

extern "C"
{
    EXPORTED int timer_create_ns(int ns)
    {
        TimerItem *timer = new TimerItem{asio::steady_timer(context, asio::chrono::nanoseconds(ns)), ns};
        return addTimer(timer);
    }

    EXPORTED int timer_create_us(int us)
    {
        TimerItem *timer = new TimerItem{asio::steady_timer(context, asio::chrono::microseconds(us)), us * 1000};
        return addTimer(timer);
    }

    EXPORTED int timer_create_ms(int ms)
    {
        TimerItem *timer = new TimerItem{asio::steady_timer(context, asio::chrono::milliseconds(ms)), ms * 1000 * 1000};
        return addTimer(timer);
    }

    EXPORTED int timer_create_s(int s)
    {
        TimerItem *timer = new TimerItem{asio::steady_timer(context, asio::chrono::seconds(s)), s * 1000 * 1000 * 1000};
        return addTimer(timer);
    }

    EXPORTED int timer_create_m(int m)
    {
        TimerItem *timer =
            new TimerItem{asio::steady_timer(context, asio::chrono::minutes(m)), m * 1000 * 1000 * 1000 * 60};
        return addTimer(timer);
    }

    EXPORTED int timer_create_h(int h)
    {
        TimerItem *timer =
            new TimerItem{asio::steady_timer(context, asio::chrono::minutes(h)), h * 1000 * 1000 * 1000 * 60 * 60};
        return addTimer(timer);
    }

    EXPORTED bool timer_wait(int tid)
    {
        TimerItem *timer = getTimer(tid);
        if (!timer)
            return false;

        timer->timer.wait();
        return true;
    }

    EXPORTED bool timer_wait_async(int tid, cube_native_var *handler)
    {
        TimerItem *timer = getTimer(tid);
        if (!timer)
            return false;

        timer->timer.async_wait(std::bind(handle_wait, std::placeholders::_1, tid, SAVE_FUNC(handler)));
        return true;
    }

    EXPORTED bool timer_repeat_async(int tid, cube_native_var *handler)
    {
        TimerItem *timer = getTimer(tid);
        if (!timer)
            return false;

        timer->timer.async_wait(std::bind(handle_repeat, std::placeholders::_1, tid, 1, SAVE_FUNC(handler)));
        return true;
    }
}