#pragma once

#include <includes.h>
#include "defs.hpp"

namespace drivers::usb
{
    struct endpoint_desc
    {
        u8  endpointAddress;
        u8  attributes;
        u8  maxPacketSize;
        u8  interval;
    };
    struct interface_desc
    {
        u8  interfaceID;
        u8  alternateSetting;
        u8  endpointCount;
        u8  classCode;
        u8  subClass;
        u8  protocolCode;
        std::string interfaceDesc;
        endpoint_desc* endpoints;
    };
    struct config_desc
    {
        u8  interfaceCount;
        u8  value;
        u8  attributes;
        u8  maxPower;
        std::string  configDesc;
        interface_desc* interfaces;
    };

    inline u16 LanguageID = 0x0409; // English(US)

    // allocates string
    std::string get_string(const usb_device& device, usb_controller* controller, u8 stringIndex);
    
    // allocates all needed memory
    config_desc get_config(vfs::node_t* dnode, u8 configID);
    void free_config(config_desc& configDesc);

    // set config
    bool set_config(vfs::node_t* dnode, const config_desc& config);
    u8 get_device_cfg_id(vfs::node_t* dnode);

    // set interface
    bool set_interface(vfs::node_t* dnode, const interface_desc& interface);

    // clear a feature
    bool clear_feature(vfs::node_t* dnode, u8 feature, u8 featureSelection, u8 featureID);

    // send control packet to device
    bool control_packet_out(vfs::node_t* dnode, request_packet rPacket);
    // get control packet to device
    bool control_packet_in(vfs::node_t* dnode, request_packet rPacket, void* buffer);

    // bulk out
    bool bulk_out(vfs::node_t* dnode, endpoint_desc* endpoint, void* buffer, size_t size);
    // bulk in
    bool bulk_in(vfs::node_t* dnode, endpoint_desc* endpoint, void* buffer, size_t size);
}

