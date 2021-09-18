/*
 * File:   IntervalTimer2.cpp
 * Author: xxxajk
 *
 * Created on September 12, 2021, 4:11 AM
 */

#include "IntervalTimer2.h"

#if defined(KINETISK) || defined(KINETISKE) || defined(KINETISL)

#include <stdio.h>

void IntervalTimer2::cISR() {
        runfunct();
}

#endif
