/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 *
 */

/**
 *	@file
 *	@brief Wii Fit Balance Board device.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#ifdef WIN32
	#include <Winsock2.h>
#endif

#include "definitions.h"
#include "wiiuse_internal.h"
#include "dynamics.h"
#include "events.h"
#include "wiiboard.h"

static uint16_t big_to_lil(uint16_t num)
{
	uint16_t ret = num;
	uint8_t *bret = (uint8_t*)&ret;
	uint8_t tmp = bret[1];
	bret[1] = bret[0];
	bret[0] = tmp;
	return ret;
}

/**
 *	@brief Handle the handshake data from the wiiboard.
 *
 *	@param wb		A pointer to a wii_board_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	@return	Returns 1 if handshake was successful, 0 if not.
 */

int wii_board_handshake(struct wiimote_t* wm, struct wii_board_t* wb, byte* data, uint16_t len) {
	int i;
	uint16_t *handshake_short;

	/* decrypt data */
	printf("DECRYPTED DATA WIIBOARD\n");
	for (i = 0; i < len; ++i)
	{
		if(i%16==0)
		{
			if(i!=0)
				printf("\n");

			printf("%X: ",0x4a40000+32+i);
		}
		printf("%02X ", data[i]);
	}
	printf("\n");

	handshake_short = (uint16_t*)data;

	wb->ctr[0] = big_to_lil(handshake_short[2]);
	wb->cbr[0] = big_to_lil(handshake_short[3]);
	wb->ctl[0] = big_to_lil(handshake_short[4]);
	wb->cbl[0] = big_to_lil(handshake_short[5]);

	wb->ctr[1] = big_to_lil(handshake_short[6]);
	wb->cbr[1] = big_to_lil(handshake_short[7]);
	wb->ctl[1] = big_to_lil(handshake_short[8]);
	wb->cbl[1] = big_to_lil(handshake_short[9]);

	wb->ctr[2] = big_to_lil(handshake_short[10]);
	wb->cbr[2] = big_to_lil(handshake_short[11]);
	wb->ctl[2] = big_to_lil(handshake_short[12]);
	wb->cbl[2] = big_to_lil(handshake_short[13]);

	/* handshake done */
	wm->event = WIIUSE_WII_BOARD_CTRL_INSERTED;
	wm->exp.type = EXP_WII_BOARD;

	#ifdef WIN32
	wm->timeout = WIIMOTE_DEFAULT_TIMEOUT;
	#endif

	return 1;
}


/**
 *	@brief The wii board disconnected.
 *
 *	@param cc		A pointer to a wii_board_t structure.
 */
void wii_board_disconnected(struct wii_board_t* wb) {
	memset(wb, 0, sizeof(struct wii_board_t));
}

static float do_interpolate(uint16_t raw, uint16_t cal[3]) {
	if (raw < cal[1]) {
		return ((raw-cal[0]) * 14.0f)/(float)(cal[1] - cal[0]);
	} else if (raw > cal[1]) {
		return ((raw-cal[1]) * 14.0f)/(float)(cal[2] - cal[1]) + 14.0f;
	}
}

/**
 *	@brief Handle wii board event.
 *
 *	@param wb		A pointer to a wii_board_t structure.
 *	@param msg		The message specified in the event packet.
 */
void wii_board_event(struct wii_board_t* wb, byte* msg) {
	uint16_t *shmsg = (uint16_t*)(msg);
	wb->rtr = (msg[0] << 8) + msg[1];// big_to_lil(shmsg[0]);
	wb->rbr = (msg[2] << 8) + msg[3];// big_to_lil(shmsg[1]);
	wb->rtl = (msg[4] << 8) + msg[5];// big_to_lil(shmsg[2]);
	wb->rbl = (msg[6] << 8) + msg[7];// big_to_lil(shmsg[3]);

	/*
		Interpolate values
		Calculations borrowed from wiili.org - No names to mention sadly :( http://www.wiili.org/index.php/Wii_Balance_Board_PC_Drivers page however!
	*/
	wb->tr = do_interpolate(wb->rtr, wb->ctr);
	wb->tl = do_interpolate(wb->rtl, wb->ctl);
	wb->br = do_interpolate(wb->rbr, wb->cbr);
	wb->bl = do_interpolate(wb->rbl, wb->cbl);
}

void wiiuse_set_wii_board_calib(struct wiimote_t *wm)
{
}
