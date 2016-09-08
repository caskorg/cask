#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <time.h>

class Timer
{
    public:
        Timer();
        ~Timer();

        void start();
        void stop();
        // Returns amount of seconds between start() and end() calls
        double elapsed();

    private:
        Timer(const Timer& rhs);
        Timer& operator=(const Timer& rhs);

        timespec m_start;
        timespec m_end;
};

#endif //TIMER_H
