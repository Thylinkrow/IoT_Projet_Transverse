#include "fire_management.h"
#include "rf.h"

void fireTreatData(uint16_t messageLen){
    uint16_t i;
    for(i=0;i<messageLen;i++){
        uprintf(UART0,"%c",readBuff[i]);
        if(readBuff[i] == '\n') uprintf(UART0,"\r");
    }
    uprintf(UART0,"\n\r");
}