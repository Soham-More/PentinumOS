#pragma once

#include <includes.h>

#define USB_TDRST       10
#define USB_TRSTRCY     10 

#define CTRL_ENDPOINT   0

#define PACKET_SETUP    0x2D
#define PACKET_IN       0x69
#define PACKET_OUT      0xe1

#define DATA0 0
#define DATA1 (1 << 19)

#define USB_HOST_TO_DEVICE 0
#define USB_DEVICE_TO_HOST (1 << 7)
#define USB_REQ_STRD 0
#define USB_REQ_CLASS (1 << 5)
#define USB_REQ_VENDOR (2 << 5)
#define USB_REP_DEVICE 0
#define USB_REP_INTERFACE 1
#define USB_REP_ENDPOINT 2
#define USB_REP_OTHER 3

#define REQ_GET_DESC 6
#define REQ_SET_ADDR 5

#define USB_GET_DESC    6
#define USB_DESC_DEVICE 1
#define USB_DESC_CONFIG 2
#define USB_DESC_STRING 3

#define STR_MAX_SIZE 256

namespace USB
{
    struct request_packet
    {
        uint8_t  requestType;
        uint8_t  request;
        uint16_t value;
        uint16_t index;
        uint16_t size;
    }_packed;

    struct usb_device
    {
        bool     isLowSpeedDevice;
        uint16_t portAddress;
        uint8_t  address;
        uint8_t  subClass;
        uint8_t  protocolCode;
        uint8_t  maxPacketSize;
        uint16_t vendorID;
        uint16_t productID;
        uint16_t bcdDevice;
        uint8_t  configCount;
        char* manufactureName;
        char* productName;
        char* serialNumber;
    };
}
