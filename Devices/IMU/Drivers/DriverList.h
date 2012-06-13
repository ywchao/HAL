// Single inclusion point for drivers. In the future this file will likely be
// automatically generated by CMake.

#ifndef _IMU_DRIVER_LIST_H_
#define _IMU_DRIVER_LIST_H_

///////////////////////////////////////////////////////////////////////////////
// Global map of driver name to driver creation function pointers:
std::map<std::string,IMUDriver*(*)()> g_mIMUDriverTable;

#include "RPG/Devices/IMU/Drivers/MicroStrain/MicroStrainDriver.h"
IMUDriverRegisteryEntry<MicroStrainDriver> _MicroStrainReg( "MicroStrain" );

#endif
