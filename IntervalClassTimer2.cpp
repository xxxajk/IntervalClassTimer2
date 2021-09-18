
#include <Arduino.h>
#if defined(KINETISK) || defined(KINETISKE) || defined(KINETISL)



/* Copyright (c) 2013 Daniel Gilbert, loglow@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 17 Sept 2021 xxxajk:
        Added Kinetis KE processor with LPIT support.
        Modified to microsecond resolution, and ability to pause.
 */


#if !defined(INTERVALCLASSTIMER_BUILTIN) & !defined(INTERVALCLASSTIMER2_BUILTIN)

#include <IntervalTimer2.h>
#include "IntervalClassTimer2.h"

// ------------------------------------------------------------
// static class variables need to be reiterated here before use
// ------------------------------------------------------------
bool IntervalClassTimer2::PIT_enabled;
bool IntervalClassTimer2::PIT_used[];
IntervalClassTimer2::ISR IntervalClassTimer2::PIT_ISR[];



// ------------------------------------------------------------
// these are the ISRs (Interrupt Service Routines) that get
// called by each PIT timer when it fires. they're defined here
// so that they can auto-clear themselves and so the user can
// specify a custom ISR and reassign it as needed
// ------------------------------------------------------------
#if defined(KINETISK) || defined(KINETISKE)

void pit0_isr() {
        PIT_TFLG0 = 1;
        IntervalClassTimer2::PIT_ISR[0]->CISR();
}

void pit1_isr() {
        PIT_TFLG1 = 1;
        IntervalClassTimer2::PIT_ISR[1]->CISR();
}

void pit2_isr() {
        PIT_TFLG2 = 1;
        IntervalClassTimer2::PIT_ISR[2]->CISR();
}

void pit3_isr() {
        PIT_TFLG3 = 1;
        IntervalClassTimer2::PIT_ISR[3]->CISR();
}

#elif defined(KINETISL)

void pit_isr() {
        if(PIT_TFLG0) {
                PIT_TFLG0 = 1;
                IntervalClassTimer2::PIT_ISR[0]->CISR();
        }
        if(!IntervalClassTimer2::PIT_enabled) return;
        if(PIT_TFLG1) {
                PIT_TFLG1 = 1;
                IntervalClassTimer2::PIT_ISR[1]->CISR();
        }
}
#endif

void IntervalClassTimer2::pause(void) {
        noInterrupts();
        if(!paused && status != TIMER_OFF) {
                *PIT_TCTRL &= ~1;
                paused = true;
        }
        interrupts();
}

void IntervalClassTimer2::resume(void) {
        noInterrupts();
        if(paused && status != TIMER_OFF) {
                paused = false;
                *PIT_TCTRL |= 1;
        }
        interrupts();
}


// ------------------------------------------------------------
// this function inits and starts the timer, using the specified
// function as a callback and the period provided. must be passed
// the name of a function taking no arguments and returning void.
// make sure this function can complete within the time allowed.
// attempts to allocate a timer using available resources,
// returning true on success or false in case of failure.
// period is specified as number of bus cycles
// ------------------------------------------------------------

bool IntervalClassTimer2::beginCycles(ISR newISR, uint32_t newValue) {

        // if this interval timer is already running, stop it
        if(status == TIMER_PIT) {
                stop_PIT();
                status = TIMER_OFF;
        }
        // store callback pointer
        myISR = newISR;
        paused = false;

        // attempt to allocate this timer
        if(allocate_PIT(newValue)) status = TIMER_PIT;
        else status = TIMER_OFF;

        // check for success and return
        if(status != TIMER_OFF) return true;
        return false;

}


// ------------------------------------------------------------
// stop the timer if it's currently running, using its status
// to determine what hardware resources the timer may be using
// ------------------------------------------------------------

void IntervalClassTimer2::end() {
        if(status == TIMER_PIT) stop_PIT();
        status = TIMER_OFF;
        paused = false;
}



// ------------------------------------------------------------
// enables the PIT clock bit, the master PIT reg, and sets flag
// ------------------------------------------------------------

void IntervalClassTimer2::enable_PIT() {
#if defined(KINETISKE)
        PCC_LPIT0 |= PCC_CLKEN;
        LPIT0_MCR |= LPIT0_MCR_RST;
        LPIT0_MCR &= ~LPIT0_MCR_RST;
        LPIT0_MCR |= /* LPIT0_MCR_DBG | */ LPIT0_MCR_DOZE | LPIT0_MCR_CEN;
#else
        SIM_SCGC6 |= SIM_SCGC6_PIT;
        PIT_MCR = 0;
#endif
        PIT_enabled = true;
}

// ------------------------------------------------------------
// disables the master PIT reg, the PIT clock bit, and unsets flag
// ------------------------------------------------------------

void IntervalClassTimer2::disable_PIT() {
#if defined(KINETISKE)
        PCC_LPIT0 &= ~PCC_CLKEN;
        LPIT0_MCR &= ~(LPIT0_MCR_DBG | LPIT0_MCR_DOZE | LPIT0_MCR_CEN);
#else
        PIT_MCR = 1;
        SIM_SCGC6 &= ~SIM_SCGC6_PIT;
#endif
        PIT_enabled = false;
}



// ------------------------------------------------------------
// enables the PIT clock if not already enabled, then checks to
// see if any PITs are available for use. if one is available,
// it's initialized and started with the specified value, and
// the function returns true, otherwise it returns false
// ------------------------------------------------------------

bool IntervalClassTimer2::allocate_PIT(uint32_t newValue) {

        // enable clock to the PIT module if necessary
        if(!PIT_enabled) enable_PIT();

        // check for an available PIT, and if so, start it
        for(uint8_t id = 0; id < NUM_PIT; id++) {
                if(!PIT_used[id]) {
                        PIT_id = id;
                        start_PIT(newValue);
                        PIT_used[id] = true;
                        return true;
                }
        }

        // no PIT available
        return false;

}



// ------------------------------------------------------------
// configures a PIT's registers, function pointer, and enables
// interrupts, effectively starting the timer upon completion
// ------------------------------------------------------------

void IntervalClassTimer2::start_PIT(uint32_t newValue) {

        // point to the correct registers
#if defined(KINETISKE)
        PIT_LDVAL = &LPIT0_TVAL0 + PIT_id * 4;
        PIT_TCTRL = &LPIT0_TCTRL0 + PIT_id * 4;
        PIT_IE_BB = &LPIT0_IE_BB + PIT_id * 4;
#else
        PIT_LDVAL = &PIT_LDVAL0 + PIT_id * 4;
        PIT_TCTRL = &PIT_TCTRL0 + PIT_id * 4;
#endif
        // point to the correct PIT ISR
        PIT_ISR[PIT_id] = myISR;

        // write value to register and enable interrupt
#if defined(KINETISKE)
        *PIT_IE_BB = 0;
        *PIT_TCTRL &= ~0xcU;
        *PIT_LDVAL = newValue;
#else
        *PIT_TCTRL = 0;
        *PIT_LDVAL = newValue;
        *PIT_TCTRL = 3;
#endif

#if defined(KINETISK) || defined(KINETISKE)
        IRQ_PIT_CH = IRQ_PIT_CH0 + PIT_id;
        NVIC_SET_PRIORITY(IRQ_PIT_CH, nvic_priority);
        NVIC_ENABLE_IRQ(IRQ_PIT_CH);
#elif defined(KINETISL)
        NVIC_SET_PRIORITY(IRQ_PIT, nvic_priority); // TODO: use the higher of both channels, shared irq
        NVIC_ENABLE_IRQ(IRQ_PIT);
#endif
#if defined(KINETISKE)
        *PIT_IE_BB = 1;
        *PIT_TCTRL |= 1;
#endif
}



// ------------------------------------------------------------
// stops an active PIT by disabling its interrupt, writing to
// its control register, and freeing up its state for future use.
// also, if no PITs remain in use, disables the core PIT clock
// ------------------------------------------------------------

void IntervalClassTimer2::stop_PIT() {

        // disable interrupt and PIT
#if defined(KINETISKE)
        *PIT_IE_BB = 0;
#endif
        *PIT_TCTRL = 0;
#if defined(KINETISK) || defined(KINETISKE)
        NVIC_DISABLE_IRQ(IRQ_PIT_CH);
#elif defined(KINETISL)
        NVIC_DISABLE_IRQ(IRQ_PIT);
#endif

        // free PIT for future use
        PIT_used[PIT_id] = false;

        // check if we're still using any PIT
        for(uint8_t id = 0; id < NUM_PIT; id++) {
                if(PIT_used[id]) return;
        }

        // none used, disable PIT clock
        disable_PIT();

}
#endif

#endif
