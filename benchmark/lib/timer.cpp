#include "timer.hpp"

Timer::Timer() : m_start(), m_end() {}
Timer::~Timer() {}

void Timer::start()
{
    clock_gettime(CLOCK_REALTIME, &m_start);
}

void Timer::stop()
{
    clock_gettime(CLOCK_REALTIME, &m_end);
}

double Timer::elapsed()
{
    double dend   = m_end.tv_sec   + ( m_end.tv_nsec   * 1.0e-9);
    double dstart = m_start.tv_sec + ( m_start.tv_nsec * 1.0e-9);

    return (dend - dstart);
}