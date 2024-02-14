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

    //
}

