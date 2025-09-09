#pragma once

#include <includes.h>
#include <std/ds.hpp>

#include "../driver_defs.hpp"

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

#define USB_SET_CONFIG  9
#define USB_GET_CONFIG  8
#define USB_CLEAR_FEATURE 1

#define STR_MAX_SIZE 256

namespace drivers::usb
{
    struct request_packet
    {
        u8  requestType;
        u8  request;
        u16 value;
        u16 index;
        u16 size;
    }_packed;

    struct usb_controller;

    struct usb_device
    {
        bool isLowSpeedDevice;
        u16 portAddress;
        u8  address;
        u8  subClass;
        u8  protocolCode;
        u8  maxPacketSize;
        u16 vendorID;
        u16 productID;
        u16 bcdDevice;
        u8  configCount;
        std::string manufactureName;
        std::string productName;
        std::string serialNumber;
    };

    struct usb_controller
    {
        void* instance_data;

        bool (*reset_device)(void* instance_data, usb_device& device);
        //std::vector<usb_device>& (*enumerate_devices)(void* instance_data);

        bool (*controlIn)(void* instance_data, const usb_device& device, request_packet rpacket, void* buffer, u16 size);
        bool (*controlOut)(void* instance_data, const usb_device& device, request_packet rpacket, u16 size);

        bool (*bulkIn)(void* instance_data, const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size);
        bool (*bulkOut)(void* instance_data, const usb_device& device, u8 endpoint, u16 maxPacketSize, void* buffer, u16 size);
    };
}
