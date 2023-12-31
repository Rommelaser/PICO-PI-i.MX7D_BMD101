/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

////////////////// Agregado por RAJCF/////////////////////
#include "gpio_pins.h"
/////////////////////// Hasta aqu� ///////////////////////

#include "board.h"
#include "pin_mux.h"

void hardware_init(void)
{
    /* Board specific RDC settings */
    BOARD_RdcInit();

    /* Board specific clock settings */
    BOARD_ClockInit();

    /* initialize debug uart */
    dbg_uart_init();

    /* RDC MU*/
    RDC_SetPdapAccess(RDC, BOARD_MU_RDC_PDAP, 3 << (BOARD_DOMAIN_ID * 2), false, false);

    /* Enable clock gate for MU*/
    CCM_ControlGate(CCM, BOARD_MU_CCM_CCGR, ccmClockNeededRun);

///////////////////////// Agregado por RAJCF ///////////////////////
    /* In this demo, we need to access RDC SEMAPHORE1 on this board */
        RDC_SetPdapAccess(RDC, rdcPdapSemaphore1, 0xFF, false, false);

        /* In this demo, we need to share board GPIO, we can set sreq argument to true
         * when the peer core could also access GPIO with RDC_SEMAPHORE, or the peer
         * core doesn't access the GPIO at all */
        RDC_SetPdapAccess(RDC, BOARD_GPIO_LED_RDC_PDAP, 0xFF, false/*true*/, false);
        /* BOARD_GPIO_LED_RDC_PDAP is same as BOARD_GPIO_KEY_RDC_PDAP, so bypass
         * BOARD_GPIO_KEY_RDC_PDAP setting */

        /* Enable PLL PFD0 for GPTA */
        CCM_ControlGate(CCM, ccmPllGateSys, ccmClockNeededRunWait);
        CCM_ControlGate(CCM, ccmPllGatePfd0, ccmClockNeededRunWait);

        /* Select GPTA clock derived from PLL PFD0 */
        CCM_UpdateRoot(CCM, BOARD_GPTA_CCM_ROOT, ccmRootmuxGptSysPllPfd0, 0, 0);
        /* Enable clock used by GPTA */
        CCM_EnableRoot(CCM, BOARD_GPTA_CCM_ROOT);
        CCM_ControlGate(CCM, BOARD_GPTA_CCM_CCGR, ccmClockNeededRunWait);

        /* Enable RDC SEMAPHORE GATE needed in this demo */
        CCM_ControlGate(CCM, ccmCcgrGateSema1, ccmClockNeededRunWait);

        /* Enable gpio clock gate, led and key share same CCGR on this board */
 //		CCM_ControlGate(CCM, BOARD_GPIO_KEY_CCM_CCGR, ccmClockNeededRunWait);

        /* Configure gpio pin IOMUX */
        configure_gpio_pin(BOARD_GPIO_LED_CONFIG);

////////////////////////   Hasta Aqui       ////////////////////////
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
