/*
 *
 * $Id: usb_core.h 44 2006-06-05 14:13:54Z gnrr $
 *
 * Copyright (C) 2003, Arnim Laeuger
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

#ifndef __USB_CORE_H__
#define __USB_CORE_H__

#include <8051.h>
#include "ezusb_reg.h"

#include "common.h"

/*
 * USB descriptor types
 */
#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05

#define USB_DT_HID				0x21
#define USB_DT_REPORT			0x22
#define USB_DT_PHYSICAL			0x23
#define USB_DT_HUB				0x29


/* global variables */

/* the next output packet is related to a control transfer after SETUP */
static bit is_ctrl_out0_kbd;         /* apply_reset_values */

/* busy semaphore for IN1 endpoint */
volatile bit in1_busy;                 /* apply_reset_values */

/* Remember IDLE mode */
volatile bit kbd_idle_mode;            /* apply_reset_values */

/* Idle duration expressed as multiples of 4ms (default: 125 * 4ms = 500ms) */
volatile data uchar kbd_idle_duration;  /* apply_reset_values */
volatile data uchar kbd_idle_counter;

/* mapping for the setup packet */
struct setup_dat {
      uchar bmRequestType;
      uchar bRequest;
      uchar wValueL;
      uchar wValueH;
      uchar wIndexL;
      uchar wIndexH;
      uchar wLengthL;
      uchar wLengthH;
};
typedef struct setup_dat setup_dat;


/* global pointers */
static xdata setup_dat * data sdat = (xdata setup_dat *)&SETUPDAT;

#define NUM_STRING 4
extern code ushort string_index[NUM_STRING];

/* the device descriptor */
static code uchar dev_desc[] = {
    0x12, // bLength             : Length of Descriptor
    0x01, // bDescriptorType     : Descriptor Type = Device
    0x10, // bcdUSB (L)          : USB Specification Version 1.10 (L)
    0x01, // bcdUSB (H)          : USB Specification Version 1.10 (H)
    0x00, // bDeviceClass        : Device Class (0xff is Vendor Specific)
    0x00, // bDeviceSubClass     : Device Sub-Class (0xff is Vendor Specific)
    0x00, // bDeviceProtocol     : Device Protocol (0xff is Vendor Specific)
    0x40, // bMaxPacketSize0     : Maximum Packet Size for EP0
    0xcd, // idVendor (L)        : Vendor ID (L)
    0x06, // idVendor (H)        : Vendor ID (H)
    0x04, // idProduct (L)       : Product ID (L)
    0xc0, // idProduct (H)       : Product ID (H)
    0x00, // bcdDevice (L)       : Device Release Number (BCD,L)
    0x01, // bcdDevice (H)       : Device Release Number (BCD,H)
    0x01, // iManufacturer       : Manufacturer Index String
    0x02, // iProduct            : Product Index String
    0x00, // iSerialNumber       : Serial Number Index String
    0x01  // bNumConfigurations  : Number of Configurations in this Device
};

/* the configuration descriptor */
static code uchar conf_desc[] = {
    0x09, // bLength             : Length of Descriptor
    0x02, // bDescriptorType     : Descriptor Type = Configuration
    0x22, // wTotalLength (L)    : Total Length (L) including Interface and Endpoint
    0x00, // wTotalLength (H)    : Total Length (H)

    0x01, // bNumInterfaces      : One Interface in this Configuration

    0x01, // bConfigurationValue : Configuration Value Used by Set_Configuration Request
          //                       to Select this Configuration
    0x00, // iConfiguration      : Index of String Describing this Configuration
    0x80, // bmAttributes        : Attributes
    0x32, // MaxPower            : Maximum Power


    /*
     * The Keyboard interface descriptor
     */
    // Interface 0, alternate setting 0
    0x09, // bLength             : Length of Descriptor
    0x04, // bDescriptorType     : Descriptor Type = Interface
    0x00, // bInterfaceNumber    : Zero-based index of this Interface
    0x00, // bAlternateSetting   : Alternate Setting
    0x01, // bNumEndpoints       : Number of Endpoints in this Interface
    0x03, // bInterfaceClass     : Interface Class
    0x01, // bInterfaceSubClass  : Interface Sub-Class (boot interface)
    0x01, // bInterfaceProtocol  : Interface Protocol (keyboard)
    0x03, // iInterface          : Index to String Descriptor for this Interface

    // HID Descriptor
    0x09, // bLength             : Length of Descriptor
    0x21, // bDescriptorType     : Descriptor Type = HID
    0x00, // bcdHID (L)          : HID Class Specification release number (L)
    0x01, // bcdHID (H)          : HID Class Specification release number (H)
    0x21, // bCountryCode        : Hardare target country = US
    0x01, // bNumDescriptors     : Number of HID class descriptors to follow
    0x22, // bDescriptorType     : Report descriptor type
    0x3f, // wDescriptorLength(L): Total length of Report descriptor (L)
    0x00, // wDescriptorLength(H): Total length of Report descriptor (H)

    // Endpoint Descriptor
    0x07, // bLength             : Length of Descriptor
    0x05, // bDescriptorType     : Descriptor Type = Endpoint
    0x81, // bEndpointAddress    : Endpoint Address
    0x03, // bmAttributes        : Endpoint Attributes = INT
    0x08, // wMaxPacketSize (L)  : Maximum Packet Size (L)
    0x00, // wMaxPacketSize (H)  : Maximum Packet Size (H)
    0x0a  // bInterval           : Polling intervall in Milliseconds
};


/* the Keyboard Report Descriptor */
static code uchar report_desc_keyboard[] = {
    0x05, 0x01, // Usage Page (Generic, Desktop),
    0x09, 0x06, // Usage (Keyboard),
    0xa1, 0x01, // Collection (Application),
    0x05, 0x07, //   Usage Page (Key Codes);
    0x19, 0xe0, //   Usage Maximum (224),
    0x29, 0xe7, //   Usage Minimum (231),
    0x15, 0x00, //   Logical Minimum (0),
    0x25, 0x01, //   Logical Maximum (1),
    0x75, 0x01, //   Report Size (1),
    0x95, 0x08, //   Report Count (8),
    0x81, 0x02, //   Input (Data, Variable, Absolute), ;Modifier byte
    0x95, 0x01, //   Report Count (1),
    0x75, 0x08, //   Report Size (8),
    0x81, 0x01, //   Input (Constant),                 ;Reserved byte
    0x95, 0x05, //   Report Count (5),
    0x75, 0x01, //   Report Size (1),
    0x05, 0x08, //   Usage Page (Page# for LEDs),
    0x19, 0x01, //   Usage Minimum (1),
    0x29, 0x05, //   Usage Maximum (5),
    0x91, 0x02, //   Output (Data, Variable, Absolute) ;LED report
    0x95, 0x01, //   Report Count (1),
    0x75, 0x03, //   Report Size (3),
    0x91, 0x01, //   Output (Constant),                ;LED report padding
    0x95, 0x06, //   Report Count (6),
    0x75, 0x08, //   Report Size (8),
    0x15, 0x00, //   Logical Minimum (0),
    0x25, 0x65, //   Logical maximum (101),
    0x05, 0x07, //   Usage Page (Key Codes),
    0x19, 0x00, //   Usage Minimum (0),
    0x29, 0x65, //   Usage Maximum (101),
    0x81, 0x00, //   Input (Data, Array),              ;Key array (6 bytes)
    0xc0        // End Collection
};


/* language ID string descriptor */
const uchar string_langid[] = { 0x04, 0x03, 0x00, 0x00 };

/* manufacturer string descriptor */
const uchar string_mfg[] = {
    24, 0x03,
    'a',0, 'u',0, 't',0, 'o',0, 'm',0, 'e',0, 't',0, 'a',0,
    '.',0, 'r',0, 'o', 0
};

/* product string descriptor        */
/* generated by "nerd_timestamp.exe" */


/* keyboard interface string descriptor */
const uchar string_if_keyboard[] = {
    14, 0x03,
    'k',0, 'b',0, 'd',0, '-',0, 'i',0, 'f',0
};

#endif /* __USB_CORE_H__ */
