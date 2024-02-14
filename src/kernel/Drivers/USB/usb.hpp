#pragma once

#include <includes.h>
#include <Bus/USB/USBController.hpp>

namespace USB
{
    struct endpoint_desc
    {
        uint8_t  endpointAddress;
        uint8_t  attributes;
        uint8_t  maxPacketSize;
        uint8_t  interval;
    };
    struct interface_desc
    {
        uint8_t  interfaceID;
        uint8_t  alternateSetting;
        uint8_t  endpointCount;
        uint8_t  classCode;
        uint8_t  subClass;
        uint8_t  protocolCode;
        char*    interfaceDesc;
        endpoint_desc* endpoints;
    };
    struct config_desc
    {
        uint8_t  interfaceCount;
        uint8_t  value;
        uint8_t  attributes;
        uint8_t  maxPower;
        char*    configDesc;
        interface_desc* interfaces;
    };

    inline uint16_t LanguageID = 0x0409; // English(US)

    // getters and setters for controllers
    void setController(USBController& controller);
    USBController* getController();

    // allocates string
    char* getString(const usb_device& device, uint8_t stringIndex);
    void freeString(char** string);
    
    // allocates all needed memory
    config_desc getConfig(const usb_device& device, uint8_t configID);
    void freeConfig(config_desc& configDesc);

    // set config
    bool setConfig(const usb_device& device, const config_desc& config);
    uint8_t getDeviceConfigID(const usb_device& device);

    // set interface
    bool setInterface(const usb_device& device, const interface_desc& interface);

    // clear a feature
    bool clearFeature(const usb_device& device, uint8_t feature, uint8_t featureSelection, uint8_t featureID);

    // send control packet to device
    bool controlPacketOut(const usb_device& device, request_packet rPacket);
    // get control packet to device
    bool controlPacketIn(const usb_device& device, request_packet rPacket, void* buffer);

    // bulk out
    bool bulkOut(const usb_device& device, endpoint_desc* endpoint, void* buffer, size_t size);
    // bulk in
    bool bulkIn(const usb_device& device, endpoint_desc* endpoint, void* buffer, size_t size);
}

