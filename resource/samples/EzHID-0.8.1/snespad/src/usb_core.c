/*
 *
 * $Id: usb_core.c,v 1.6 2004/03/06 12:24:05 arniml Exp $
 *
 * Copyright (C) 2004, Arnim Laeuger
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. See also the file COPYING which
 *  came with this application.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "usb_core.h"
#include "all_ext.h"


void core_apply_reset_values(void) using 1;


/*
 * remember IDLE mode
 */
bit pad1_idle_mode;            /* apply_reset_values */
bit pad2_idle_mode;            /* apply_reset_values */
/*
 * idle duration expressed as multiples of 4 ms
 * default is 125 * 4 ms = 500 ms
 */
data Byte pad1_idle_duration;  /* apply_reset_values */
data Byte pad1_idle_counter;
data Byte pad2_idle_duration;  /* apply_reset_values */
data Byte pad2_idle_counter;



/*****************************************************************************
 * apply_reset_values()
 *
 * Applies the reset values to all global variables owned by usb_core.
 *****************************************************************************/
void apply_reset_values(void) using 1
{
  core_apply_reset_values();

  in1_busy  = FALSE;
  in2_busy  = FALSE;

  pad1_idle_mode     = FALSE;
  pad2_idle_mode     = FALSE;

  pad1_idle_duration = 125;
  pad2_idle_duration = 125;

  sof_delay = SOF_RELOAD;
}


/*****************************************************************************
 * sof_isr()
 *
 * The Start Of Frame interrupt service routine.
 * It checks the idle counter and the padX_new_state flags every 4 ms in
 * order to determine whether a report has to be transmitted.
 *****************************************************************************/
#pragma save
#pragma nooverlay
void sof_isr(void) using 1
{
  bit pad1_send_report = FALSE;
  bit pad2_send_report = FALSE;

  /* when sof_delay has reached 0, it's time to care for the idle counters */
  if (--sof_delay == 0) {
    /* reload */
    sof_delay = SOF_RELOAD;

    /*
     * Pad 1 service
     */
    if (!pad1_idle_mode) {
      if (--pad1_idle_counter == 0) {
        pad1_send_report = TRUE;
        /* reload counter */
        pad1_idle_counter = pad1_idle_duration;
      }
    }

    /* send report if the state has changed */
    if (pad1_new_state) {
      pad1_send_report  = TRUE;
      pad1_new_state    = FALSE;
      /* reload counter */
      pad1_idle_counter = pad1_idle_duration;
    }

    /* finally send the report if necessary */
    if (pad1_send_report) {
      if (!in1_busy) {
        in1buf(0) = pad1_buffer[0];
        in1buf(1) = pad1_buffer[1];

        /* transfer the key buffer to endpoint IN1 */
        in1_busy  = TRUE;
        IN1BC     = 2;
      } else {
        /* re-queue report for next time */
        pad1_new_state = TRUE;
      }
    }


    /*
     * Pad 2 service
     */
    if (!pad2_idle_mode) {
      if (--pad2_idle_counter == 0) {
        pad2_send_report = TRUE;
        /* reload counter */
        pad2_idle_counter = pad2_idle_duration;
      }
    }

    /* send report if the state has changed */
    if (pad2_new_state) {
      pad2_send_report  = TRUE;
      pad2_new_state    = FALSE;
      /* reload counter */
      pad2_idle_counter = pad2_idle_duration;
    }

    /* finally send the report if necessary */
    if (pad2_send_report) {
      if (!in2_busy) {
        in2buf(0) = pad2_buffer[0];
        in2buf(1) = pad2_buffer[1];

        /* transfer the key buffer to endpoint IN1 */
        in2_busy  = TRUE;
        IN2BC     = 2;
      } else {
        /* re-queue report for next time */
        pad2_new_state = TRUE;
      }
    }


  }
}
#pragma restore


/*****************************************************************************
 * get_descriptor()
 *
 * Sends the requested descriptor back to the host.
 * Currently supported descriptor types are:
 *   + device
 *   + configuration
 *   + string
 *   + HID
 *   + report
 *****************************************************************************/
#pragma save
#pragma nooverlay
static void get_descriptor(void) using 1
{
  switch (sdat->wValueH) {
    /* type: device */
  case USB_DT_DEVICE:
    EP0CS   = 0x02;             /* clear HSNACK */
    SUDPTRH = (Byte)((unsigned int)dev_desc >> 8);
    SUDPTRL = (unsigned int)dev_desc & 0xff;
    break;

    /* type: configuration */
  case USB_DT_CONFIG:
    EP0CS   = 0x02;             /* clear HSNACK */
    SUDPTRH = (Byte)((unsigned int)conf_desc >> 8);
    SUDPTRL = (unsigned int)conf_desc & 0xff;
    break;

    /* type: string */
  case USB_DT_STRING:
    if (sdat->wValueL < NUM_STRING) {
      EP0CS     = 0x02;         /* clear HSNACK */
      SUDPTRH = (Byte)(string_index[sdat->wValueL] >> 8);
      SUDPTRL = string_index[sdat->wValueL] & 0xff;
    } else {
      EP0CS     = 0x03;         /* stall */
    }
    break;

    /* type: HID */
  case USB_DT_HID:
    EP0CS   = 0x02;   /* clear HSNACK */
      /* we transmit the same HID descriptor for both Pads */
    SUDPTRH = (Byte)(((unsigned int)conf_desc) + 18 >> 8);
    SUDPTRL = ((unsigned int)conf_desc) + 18 & 0xff;
    break;

    /* type: Report */
  case USB_DT_REPORT:
    switch (sdat->wIndexL) {
      case 0x00:
        /* fallthrough */
      case 0x01:
        EP0CS   = 0x02;   /* clear HSNACK */
        /* we transmit the same report descriptor for both Pads */
        SUDPTRH = (Byte)((unsigned int)(report_desc_pad) >> 8);
        SUDPTRL = (unsigned int)(report_desc_pad) & 0xff;
        break;

      default:
        EP0CS   = 0x03;   /* stall */
        break;
    }
    break;

  default:
    EP0CS     = 0x03;           /* stall */
    break;
  }
}
#pragma restore


/*****************************************************************************
 * usb_isr()
 *
 * USB interrupt service routine
 * Performs five major tasks:
 *   + parse the SETUP packets upon SUDAV interrupt and answer them
 *   + ack IN0 interrupts
 *   + ack OUT0 interrupts
 *   + ack additional INx and OUTx interrupts
 *   + put device into suspend upon SUSB interrupt
 *
 * According to the EZ-USB Technical Reference Manual it is essential to
 * clear HSNACK when stalling EP0 (see section 12.12).
 *****************************************************************************/
void usb_isr(void) interrupt 8 using 1
{
  /* clear INT2 interrupt */
  EXIF &= 0xef;

  /* was this the SUDAV interrupt? */
  if (USBIRQ & 0x01) {

    /* clear SUDAV interrupt */
    USBIRQ = 0x01;

    switch (sdat->bRequest) {

      /* bRequest == 0x00 */
    case 0x00:
      switch (sdat->bmRequestType) {
        /* GetStatus(Device) TODO */
      case 0x80:
        /* GetStatus(Interface) */
      case 0x81:
        /* GetStatus(Endpoint) TODO*/
      case 0x82:
        in0buf(0) = 0x00;
        in0buf(1) = 0x00;
        IN0BC     = 0x02;
        EP0CS     = 0x02;       /* clear HSNACK */
        break;

      default:
        EP0CS     = 0x03;       /* stall */
        break;
      }
      break;

      /* bRequest == 0x01 */
    case 0x01:
      switch (sdat->bmRequestType) {
        /* ClearFeature(Device) TODO */
      case 0x00:
        EP0CS     = 0x02;       /* clear HSNACK */
        break;
        /* ClearFeature(Interface) TODO */
      case 0x01:
        EP0CS     = 0x02;       /* clear HSNACK */
        break;
        /* ClearFeature(Endpoint) */
      case 0x02:
        switch (sdat->wIndexL) {
          /* ClearFeature(EP) should reset TOGGLE bit
             (ref. USB spec 1.1, section 5.8.5) */
        case 0x00:              /* OUT0 */
          TOGCTL = 0x00;
          TOGCTL = 0x20;
          EP0CS  = 0x02;        /* clear HSNACK */
          break;
        case 0x80:              /* IN0  */
          TOGCTL = 0x10;
          TOGCTL = 0x30;
          EP0CS  = 0x02;        /* clear HSNACK */
          break;
        case 0x81:              /* IN1  */
          IN1CS &= 0xfe;        /* unstall endpoint */
          TOGCTL = 0x11;
          TOGCTL = 0x31;
          EP0CS  = 0x02;        /* clear HSNACK */;
          break;
        case 0x82:              /* IN2  */
          IN2CS &= 0xfe;        /* unstall endpoint */
          TOGCTL = 0x12;
          TOGCTL = 0x32;
          EP0CS  = 0x02;        /* clear HSNACK */;
          break;
        default:
          EP0CS = 0x03;         /* stall */
          break;
        }
        break;

	/* Get_Report TODO*/
      case 0xa1:
	/* return a report over IN0BUF */
	switch (sdat->wIndexL) {
          /* Interface 0 -> Pad 1 */
          case 0x00:
            in0buf(0) = pad1_buffer[0];
            in0buf(1) = pad1_buffer[1];

            /* remove flag */
            pad1_new_state = FALSE;
            /* initiate transfer */
            IN0BC    = 0x02;
            EP0CS    = 0x02;   /* clear HSNACK */
            break;

            /* Interface 1 -> Pad 2 */
          case 0x01:
            in0buf(0) = pad2_buffer[0];
            in0buf(1) = pad2_buffer[1];

            /* remove flag */
            pad2_new_state = FALSE;
            /* initiate transfer */
            IN0BC    = 0x02;
            EP0CS    = 0x02;   /* clear HSNACK */
            break;

          default:
            EP0CS    = 0x03;   /* stall */
            break;
        }
	break;

      default:
        EP0CS      = 0x03;   /* stall */
        break;
      }
      break;

      /* bRequest == 0x02 */
    case 0x02:
      switch (sdat->bmRequestType) {
	/* Get_Idle */
      case 0xa1:
        switch (sdat->wIndexL) {
            /* Interface 0 -> Pad 1 */
          case 0x00 :
            in0buf(0) = pad1_idle_duration;
            IN0BC     = 0x01;
            EP0CS     = 0x02;     /* clear HSNACK */
            break;

            /* Interface 1 -> Pad 2 */
          case 0x01 :
            in0buf(0) = pad2_idle_duration;
            IN0BC     = 0x01;
            EP0CS     = 0x02;     /* clear HSNACK */
            break;

          default:
            EP0CS     = 0x03;     /* stall */
            break;
        }
	break;

      default:
	EP0CS       = 0x03;     /* stall */
	break;
      }
      break;
      
      
      /* SetFeature */
    case 0x03:
      switch (sdat->bmRequestType) {
        /* SetFeature(Device) TODO */
      case 0x00:
        EP0CS     = 0x02;       /* clear HSNACK */
        break;
        /* SetFeature(Interface) TODO */
      case 0x01:
        EP0CS     = 0x02;       /* clear HSNACK */
        break;
        /* SetFeature(Endpoint) */
      case 0x02:
        switch (sdat->wIndexL) {
        case 0x00:              /* OUT0 */
          EP0CS  |= 0x01;       /* stall endpoint */
          TOGCTL  = 0x00;
          TOGCTL  = 0x20;
          OUT0BC  = 0x00;
          EP0CS  |= 0x02;       /* clear HSNACK */
          break;
        case 0x80:              /* IN0  */
          EP0CS  |= 0x01;       /* stall endpoint */
          TOGCTL  = 0x10;
          TOGCTL  = 0x30;
          EP0CS  |= 0x02;       /* clear HSNACK */
          break;
        case 0x81:              /* IN1  */
          IN1CS   = 0x01;       /* stall endpoint */
          TOGCTL  = 0x11;
          TOGCTL  = 0x31;
          IN1CS  |= 0x02;
          EP0CS   = 0x02;       /* clear HSNACK */;
          break;
        case 0x82:              /* IN2  */
          IN2CS   = 0x01;       /* stall endpoint */
          TOGCTL  = 0x12;
          TOGCTL  = 0x32;
          IN2CS  |= 0x02;
          EP0CS   = 0x02;       /* clear HSNACK */;
          break;
        default:
          EP0CS  = 0x03;        /* stall */
          break;
        }
        break;

	/* Get_Protocol TODO */
      case 0xa1:
	switch (sdat->wIndexL) {
            /* Interface 0+1 -> Pad 1+2 */
          case 0x00:
            /* fallthrough */
          case 0x01:
            in0buf(0) = 1;      /* always send Report protocol */
            IN0BC     = 0x01;
            EP0CS     = 0x02;   /* clear HSNACK */
            break;

          default:
            EP0CS     = 0x03;   /* stall */
            break;
        }
	break;

      default:
        EP0CS       = 0x03;   /* stall */
        break;
      }
      break;

      /* GetDescriptor */
    case 0x06:
      get_descriptor(/*sdat*/);
      break;

      /* GetConfiguration */
    case 0x08:
      if (sdat->bmRequestType == 0x80) {
        in0buf(0) = 0x01;
        IN0BC     = 0x01;
        EP0CS     = 0x02;       /* clear HSNACK */
      } else {
        EP0CS     = 0x03;       /* stall */
      }
      break;

      /* SetConfiguration */
    case 0x09:
      switch (sdat->bmRequestType) {
        /* SetConfiguration */
      case 0x00:
        EP0CS     = 0x02;       /* clear HSNACK */
        break;

	/* Set_Report */
      case 0x21:
        EP0CS     = 0x03;       /* stall */
	break;

      default:
        EP0CS     = 0x03;       /* stall */
        break;
      }
      break;

      /* bRequest == 0x0a */
    case 0x0a:
      switch (sdat->bmRequestType) {
        /* GetInterface */
      case 0x81:
        in0buf(0) = 0x00;
        IN0BC     = 0x01;
        EP0CS     = 0x02;       /* clear HSNACK */
        break;

	/* Set_Idle */
      case 0x21:
        switch (sdat->wIndexL) {
            /* Interface 0 -> Pad 1 */
          case 0x00:
            if (sdat->wValueH > 0) {
              pad1_idle_mode     = FALSE;
              pad1_idle_duration = sdat->wValueH;
            } else {
              pad1_idle_mode     = TRUE;
            }

            EP0CS   = 0x02;   /* clear HSNACK */
            break;

            /* Interface 1 -> Pad 2 */
          case 0x01:
            if (sdat->wValueH > 0) {
              pad2_idle_mode     = FALSE;
              pad2_idle_duration = sdat->wValueH;
            } else {
              pad2_idle_mode     = TRUE;
            }

            EP0CS   = 0x02;   /* clear HSNACK */
            break;

          default:
            EP0CS   = 0x03;   /* stall */
            break;
        }
	break;

      default:
        EP0CS     = 0x03;   /* stall */
        break;
      }
      break;

      /* bRequest == 0x0b */
    case 0x0b:
      switch (sdat->bmRequestType) {
        /* SetInterface */
      case 0x01:
        if (sdat->wValueL == 0) { /* only AS 0 supported */
          switch (sdat->wIndexL) {
              /* Interface 0, EP1 -> Pad 1 */
            case 0x00:
              TOGCTL = 0x11;
              TOGCTL = 0x31;      /* reset toggle of IN1 */
              IN1CS |= 0x02;      /* remove busy */

              EP0CS  = 0x02;      /* clear HSNACK */
              break;

              /* Interface 1, EP2 -> Pad 2 */
            case 0x01:
              TOGCTL = 0x12;
              TOGCTL = 0x32;      /* reset toggle of IN1 */
              IN2CS |= 0x02;      /* remove busy */

              EP0CS  = 0x02;      /* clear HSNACK */
              break;

            default:
              EP0CS  = 0x03;      /* stall */
              break;
          }
        } else
          EP0CS  = 0x03;        /* stall */
        break;

        /* Set_Protocol TODO */
      case 0x21:
        EP0CS     = 0x02;       /* clear HSNACK */
        break;

      default:
        EP0CS     = 0x03;       /* stall */
        break;
      }
      break;

    default:
      EP0CS     = 0x03;         /* stall */
      break;
    }


  }

  /* was this an IN0 interrupt? */
  if (IN07IRQ & 0x01) {
    /* clear IN0 interrupt */
    IN07IRQ = 0x01;
  }

  /* was this a OUT0 interrupt? */
  if (OUT07IRQ & 0x01) {
    /* clear OUT0 interrupt */
    OUT07IRQ = 0x01;

    /* arm endpoint again */
    OUT0BC = 0x00;
  }


  /* was this the URES interrupt? */
  if (USBIRQ & 0x10) {
    /* clear request */
    USBIRQ = 0x10;

    /* global variables must be re-initialized here,
       in addition everything else must be put back into reset state */
    apply_reset_values();
  }

  /* was this the SUSP interrupt? */
  if (USBIRQ & 0x08) {
    /* clear request */
    USBIRQ = 0x08;
    /* enter idle mode */
    PCON |= 0x01;
  }

  /* was this an IN1 interrupt? */
  if (IN07IRQ & 0x02) {
    /* clear IN1 interrupt */
    IN07IRQ = 0x02;

    /* clear busy flag */
    in1_busy = FALSE;
  }

  /* was this an IN2 interrupt? */
  if (IN07IRQ & 0x04) {
    /* clear IN2 interrupt */
    IN07IRQ = 0x04;

    /* clear busy flag */
    in2_busy = FALSE;
  }

  /* was this the SOF interrupt? */
  if (USBIRQ & 0x02) {
    /* clear request */
    USBIRQ = 0x02;

    sof_isr();
  }
}


void resume_isr(void) interrupt 6 using 2
{
  /* clear resume interrupt */
  EICON &= ~0x10;
}


/*****************************************************************************
 * setup_usb_int()
 *
 * Setup the USB interrupts.
 *****************************************************************************/
void setup_usb_int()
{
  /* disable global interrupt */
  EA       = 0;

  /* clear autovector enable */
  USBBAV   = 0x00;
#if !defined (ROM_FIRMWARE)
  /* clear SUDAV interrupt and all other interrupts */
  USBIRQ   = 0xff;
#endif
  /* enable SUDAV, SOF, SUSP and URES interrupts */
  USBIEN   = 0x1b;

#if !defined (ROM_FIRMWARE)
  /* clear IN0, IN1 and IN2 interrupts */
  IN07IRQ  = 0x07;
#endif
  /* enable IN0, IN1 and IN2 interrupts */
  IN07IEN  = 0x07;
#if !defined (ROM_FIRMWARE)
  /* clear OUT0 interrupt */
  OUT07IRQ = 0x01;
#endif
  /* enable OUT0 interrupt */
  OUT07IEN = 0x01;

#if !defined (ROM_FIRMWARE)
  /* unstall endpoints */
  IN1CS    = 0x00;
  IN2CS    = 0x00;
#endif

  /* validate endpoints */
  IN07VAL  = 7;
  OUT07VAL = 1;

  /* enable external 2 interrupt */
  EUSB     = 1;

  /* clear pending resume interrupt */
  EICON   &= ~0x10;
  /* enable resume interrupt */
  EICON   |= 0x20;

  /* enable global interrupt */
  EA       = 1;
}




/*****************************************************************************
 *
 * Change History:
 * ===============
 *
 * $Log: usb_core.c,v $
 * Revision 1.6  2004/03/06 12:24:05  arniml
 * adjust pragmas to SDCC 2.4.0
 *
 * Revision 1.5  2004/02/15 01:15:51  arniml
 * declare "using 1" for all functions called in interrupts
 *
 * Revision 1.4  2004/01/29 19:16:19  arniml
 * apply reset value for padX_idle_mode
 *
 * Revision 1.3  2004/01/18 20:53:28  arniml
 * first version with all basic features implemented
 *
 * Revision 1.2  2004/01/18 14:29:45  arniml
 * - implement sof_isr()
 * - handle Get_Descriptor for USB_DT_REPORT
 *
 * Revision 1.1  2004/01/18 13:17:59  arniml
 * initial check-in
 *
 */
