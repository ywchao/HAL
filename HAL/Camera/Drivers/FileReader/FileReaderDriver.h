#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <HAL/Camera/CameraDriverInterface.h>

namespace hal {

class FileReaderDriver : public CameraDriverInterface
{
    public:
        FileReaderDriver(const std::vector<std::string>& ChannelRegex, size_t StartFrame = 0, bool Loop = false, size_t BufferSize = 35, int cvFlags = 0 /*cv::IMREAD_UNCHANGED*/);
        ~FileReaderDriver();

        bool Capture( pb::CameraMsg& vImages );

        std::string GetDeviceProperty(const std::string& sProperty);

        unsigned int Width( unsigned int idx = 0 );

        unsigned int Height( unsigned int idx = 0 );

    private:
        static void _ThreadCaptureFunc( FileReaderDriver* pFR );
        bool _Read();
        double _GetNextTime();

    private:
        volatile bool                                   m_bShouldRun;
        std::thread*									m_CaptureThread;

        // vector of lists of files
        std::mutex                                      m_Mutex;
        std::condition_variable                         m_cBufferEmpty;
        std::condition_variable                         m_cBufferFull;

        // TODO refactor using circular buffer
        std::vector< pb::CameraMsg >                    m_vBuffer;
        unsigned int                                    m_nHead;
        unsigned int                                    m_nTail;

        std::queue< pb::CameraMsg >                     m_qImageBuffer;
        std::vector< std::vector< std::string > >		m_vFileList;
        std::string                                     m_sBaseDir;
        unsigned int                                    m_nNumChannels;
        unsigned int                                    m_nStartFrame;
        unsigned int                                    m_nCurrentImageIndex;
        bool                                            m_bLoop;
        unsigned int                                    m_nNumImages;
        unsigned int                                    m_nBufferSize;
        int                                             m_iCvImageReadFlags;
        std::string                                     m_sTimeKeeper;
};

}