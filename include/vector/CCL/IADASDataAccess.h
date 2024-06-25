/*-------------------------------------------------------------------------
Module: ADAS
Interfaces: -
---------------------------------------------------------------------------
Copyright (c) Vector Informatik GmbH. All rights reserved.
-------------------------------------------------------------------------*/

#pragma once
#ifndef IADASDATAACCESS_H
#define IADASDATAACCESS_H

namespace NADAS {
    class IADASDataAccess {
    public:
        virtual bool SetGpbOSI3Data(const uint8_t *data, int32 dataSize) = 0;

        virtual bool SetGpbOSI3Data2(const uint8_t *data, int32 dataSize, uint8 dataType) = 0;
    };
}
#endif //IADASDATAACCESS_H