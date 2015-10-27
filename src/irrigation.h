#ifndef IRRIGATION_H_INCLUDED
#define IRRIGATION_H_INCLUDED
#include "fby.h"
#include "fbynet.h"

bool StartFTDI();
void SetFTDI(int bit);
void StopFTDI();

extern std::string startTime1;
extern std::string startTime2;
FBYCLASSPTR(Valve);
FBYCLASS(Valve) : public ::Fby::BaseObj {
public:
    std::string name;
    int valve_num;
    int run_time;
    int remaining_run_time;
    int days;
    Fby::IntervalObjectPtr timer;

    Valve();
    Valve(int valve_num);
    void TurnOff(Fby::NetPtr);
    void TurnOn(Fby::NetPtr);
};


#endif /* #ifndef IRRIGATION_H_INCLUDED */
