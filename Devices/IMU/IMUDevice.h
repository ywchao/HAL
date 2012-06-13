/*
   \file IMUDevice.h

   Abstract device that represents a generic IMU.

 */

#ifndef _IMU_DEVICE_H_
#define _IMU_DEVICE_H_

#include <RPG/Utils/PropertyMap.h>
#include <RPG/Devices/IMU/IMUDriverInterface.h>
#include <RPG/Devices/IMU/Drivers/IMUDriverRegistery.h>

// Driver Creation Factory
extern IMUDriver* CreateDriver( const std::string& sDriverName );

///////////////////////////////////////////////////////////////////////////////
// Generic IMU device
class IMUDevice : public PropertyMap
{
    public:
        ///////////////////////////////////////////////////////////////
        IMUDevice()
        {
        }

        ///////////////////////////////////////////////////////////////
        ~IMUDevice()
        {
        }

        ///////////////////////////////////////////////////////////////
        void RegisterDataCallback(IMUDriverDataCallback callback)
        {
            if( m_pDriver ){
                m_pDriver->RegisterDataCallback( callback );
            }else{
                std::cerr << "ERROR: no driver initialized!\n";
            }
            return;
        }

        ///////////////////////////////////////////////////////////////
        bool InitDriver( const std::string& sDriver )
        {
            m_pDriver = CreateDriver( sDriver );
            if( m_pDriver ){
                m_pDriver->SetPropertyMap( this );
                return m_pDriver->Init();
            }
            return false;
        }

    private:
        // A IMU device will create and initialize a particular driver:
        IMUDriver*          m_pDriver;
};

#endif