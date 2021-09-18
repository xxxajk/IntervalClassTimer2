/*
 * File:   IntervalTimer2.h
 * Author: xxxajk
 *
 * Created on September 12, 2021, 4:11 AM
 */


#if defined(KINETISK) || defined(KINETISKE) || defined(KINETISL)

#ifndef __INTERVALTIMER2_H__
#define __INTERVALTIMER2_H__
#include "IntervalClassTimer2.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
        class IntervalTimer2 : public ictCISR {
        private:
                IntervalClassTimer2 ict;
                void (*runfunct)();
                void cISR();

        public:

                IntervalTimer2() {
                }

                ~IntervalTimer2() {
                        end();
                }

                bool begin(void (*funct)(), unsigned int microseconds) {
                        runfunct = funct;
                        return ict.begin(this, microseconds);

                }

                bool begin(void (*funct)(), int microseconds) {
                        runfunct = funct;
                        return ict.begin(this, microseconds);
                }

                bool begin(void (*funct)(), unsigned long microseconds) {
                        runfunct = funct;
                        return ict.begin(this, microseconds);
                }

                bool begin(void (*funct)(), long microseconds) {
                        runfunct = funct;
                        return ict.begin(this, microseconds);
                }

                bool begin(void (*funct)(), float microseconds) {
                        runfunct = funct;
                        return ict.begin(this, microseconds);
                }

                bool begin(void (*funct)(), double microseconds) {
                        runfunct = funct;
                        return ict.begin(this, microseconds);
                }

                void priority(uint8_t n) {
                        ict.priority(n);
                }

                void end() {
                        ict.end();
                }
        };

#ifdef __cplusplus
}
#endif

#endif // ndef __INTERVALTIMER2_H__

#endif // defined(KINETISK) || defined(KINETISKE) || defined(KINETISL)
