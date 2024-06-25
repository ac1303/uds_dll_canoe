/*-----------------------------------------------------------------------------
Module: VIA (observed)
Interfaces: This is a public interface header.
-------------------------------------------------------------------------------
VIA interfaces for ADAS.
-------------------------------------------------------------------------------
Copyright (c) Vector Informatik GmbH. All rights reserved.
-----------------------------------------------------------------------------*/

#ifndef VIA_ADAS_H
#define VIA_ADAS_H


// ============================================================================
// Version of the interfaces, which are declared in this header
// See  REVISION HISTORY for a description of the versions
// 10.12.2019   1.0   Fbo    Created CANoe 13.0
// ============================================================================

#define VIAADASMajorVersion 1
#define VIAADASMinorVersion 0

#ifndef VIA_H
#include "VIA.h"
#endif

// Forward declarations
namespace NADAS {
    class IADASDataAccess;

    class IADASDataManager;

    class IScenarioManager;
}

// ============================================================================
// class VIAADASService
// ============================================================================

class VIAADASService {
public:

    // Get ADAS data access interface
    VIASTDDECL GetADASDataAccess(NADAS::IADASDataAccess **adasDataAccessr) = 0;

    VIASTDDECL GetADASDataManager(NADAS::IADASDataManager **adasDataManager) = 0;

    VIASTDDECL GetScenarioManager(NADAS::IScenarioManager **scenarioManager) = 0;

}; // class VIAADASService


#endif // VIA_ADAS_H