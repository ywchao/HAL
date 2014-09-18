#include "ConvertDriver.h"
#include "HAL/Devices/DeviceException.h"

#include <iostream>

#include <opencv2/opencv.hpp>

namespace hal
{

ConvertDriver::ConvertDriver(
    std::shared_ptr<CameraDriverInterface> Input,
    const std::string& sFormat,
    double dRange
    )
  : m_Input(Input),
    m_sFormat(sFormat),
    m_nOutCvType(-1),
    m_nNumChannels(Input->NumChannels()),
    m_dRange(dRange)
{
  for(size_t i = 0; i < Input->NumChannels(); ++i) {
    m_nImgWidth.push_back(Input->Width(i));
    m_nImgHeight.push_back(Input->Height(i));
  }

  // Guess output color coding
  if( m_sFormat == "MONO8" ) {
    m_nOutCvType = CV_8UC1;
    m_nOutPbType = pb::Format::PB_LUMINANCE;
  } else if( m_sFormat == "RGB8" ) {
    m_nOutCvType = CV_8UC3;
    m_nOutPbType = pb::Format::PB_RGB;
  } else if( m_sFormat == "BGR8" ) {
    m_nOutCvType = CV_8UC3;
    m_nOutPbType = pb::Format::PB_BGR;
  }

  if( m_nOutCvType == -1 )
    throw DeviceException("HAL: Error! Unknown target format: " + m_sFormat);
}

bool ConvertDriver::Capture( pb::CameraMsg& vImages )
{
  m_Message.Clear();
  m_Input->Capture( m_Message );

  // Guess source color coding.
  if( m_nCvType.empty() ) {
    for(int i = 0; i < m_Message.image_size(); ++i) {
      int cvtype = -1;
      pb::Format pbtype = m_Message.image(i).format();
      int channels = 0;

      if( m_Message.image(i).format() == pb::PB_LUMINANCE )
        channels = 1;
      else if( m_Message.image(i).format() == pb::PB_RGB ||
               m_Message.image(i).format() == pb::PB_BGR )
        channels = 3;

      if( channels != 0 ) {
        if( m_Message.image(i).type() == pb::PB_BYTE ||
            m_Message.image(i).type() == pb::PB_UNSIGNED_BYTE )
          cvtype = (channels == 1 ? CV_8UC1 : CV_8UC3);
        else if( m_Message.image(i).type() == pb::PB_UNSIGNED_SHORT ||
                 m_Message.image(i).type() == pb::PB_SHORT )
          cvtype = (channels == 1 ? CV_16UC1 : CV_16UC3);
        else if( m_Message.image(i).type() == pb::PB_FLOAT )
          cvtype = (channels == 1 ? CV_32FC1 : CV_32FC3);
      }

      m_nCvType.push_back(cvtype);
      m_nPbType.push_back(pbtype);

      if( cvtype == -1 ) {
        std::cerr << "HAL: Error! Could not guess source color coding of "
                     "channel " << i << ". Is it RAW?" << std::endl;
      }
    }
  }

  // Prepare return images.
  vImages.set_device_time( m_Message.device_time() );

  for(size_t ii = 0; ii < m_nNumChannels; ++ii) {
    pb::ImageMsg* pbImg = vImages.add_image();

    if( m_nCvType[ii] == -1 ) { // this image cannot be converted
      *pbImg = m_Message.image(ii);
      continue;
    }

    pbImg->set_width( m_nImgWidth[ii] );
    pbImg->set_height( m_nImgHeight[ii] );
    pbImg->set_type( pb::PB_UNSIGNED_BYTE );
    pbImg->set_format( m_nOutPbType );
    pbImg->mutable_data()->resize(m_nImgWidth[ii] * m_nImgHeight[ii] *
                                  (m_nOutCvType == CV_8UC1 ? 1 : 3) );
    pbImg->set_timestamp( m_Message.image(ii).timestamp() );
    pbImg->set_serial_number( m_Message.image(ii).serial_number() );

    cv::Mat sImg(m_nImgHeight[ii], m_nImgWidth[ii], m_nCvType[ii],
                   (void*)m_Message.mutable_image(ii)->data().data());

    cv::Mat dImg(m_nImgHeight[ii], m_nImgWidth[ii], m_nOutCvType,
                   (void*)pbImg->mutable_data()->data());

    // note: cv::cvtColor cannot convert between depth types and
    // cv::Mat::convertTo cannot change the number of channels
    cv::Mat aux;
    switch( m_nCvType[ii] ) {
      case CV_8UC1:
        if( m_nOutCvType == CV_8UC1 )
          std::copy(sImg.begin<unsigned char>(), sImg.end<unsigned char>(),
                    dImg.begin<unsigned char>());
        else
          cv::cvtColor(sImg, dImg,
            (m_nOutPbType == pb::Format::PB_RGB ? CV_GRAY2RGB : CV_GRAY2BGR));
        break;

      case CV_8UC3:
        if( m_nOutCvType == CV_8UC1 )
          cv::cvtColor(sImg, dImg,
            (m_nPbType[ii] == pb::Format::PB_RGB ? CV_RGB2GRAY : CV_BGR2GRAY));
        else {
          if( m_nPbType[ii] == m_nOutPbType )
            std::copy(sImg.begin<unsigned char>(), sImg.end<unsigned char>(),
                      dImg.begin<unsigned char>());
          else
            cv::cvtColor(sImg, dImg,
              (m_nPbType[ii] == pb::Format::PB_RGB ? CV_RGB2BGR : CV_BGR2RGB));
        }
        break;

      case CV_16UC1:
        sImg.convertTo(aux, CV_64FC1);
        if( m_nOutCvType == CV_8UC1 )
          aux.convertTo(dImg, CV_8UC1, 255. / m_dRange);
        else {
          aux.convertTo(aux, CV_8UC1, 255. / m_dRange);
          cv::cvtColor(aux, dImg,
            (m_nOutPbType == pb::Format::PB_RGB ? CV_GRAY2RGB : CV_GRAY2BGR));
        }
        break;

      case CV_16UC3:
        sImg.convertTo(aux, CV_64FC3);
        if( m_nOutCvType == CV_8UC1 ) {
          aux.convertTo(aux, CV_8UC3, 255. / m_dRange);
          cv::cvtColor(aux, dImg,
            (m_nPbType[ii] == pb::Format::PB_RGB ? CV_RGB2GRAY : CV_BGR2GRAY));
        } else {
          if( m_nPbType[ii] == m_nOutPbType )
            aux.convertTo(dImg, CV_8UC3, 255. / m_dRange);
          else {
            aux.convertTo(aux, CV_8UC3, 255. / m_dRange);
            cv::cvtColor(aux, dImg,
              (m_nPbType[ii] == pb::Format::PB_RGB ? CV_RGB2BGR : CV_BGR2RGB));
          }
        }
        break;

      case CV_32FC1:
        if( m_nOutCvType == CV_8UC1 ) {
          sImg.convertTo(dImg, CV_8UC1, 255. / m_dRange);
        } else {
          sImg.convertTo(aux, CV_8UC1, 255. / m_dRange);
          cv::cvtColor(aux, dImg,
            (m_nOutPbType == pb::Format::PB_RGB ? CV_GRAY2RGB : CV_GRAY2BGR));
        }
        break;

      case CV_32FC3:
        if( m_nOutCvType == CV_8UC1 ) {
          sImg.convertTo(aux, CV_8UC3, 255. / m_dRange);
          cv::cvtColor(aux, dImg,
            (m_nPbType[ii] == pb::Format::PB_RGB ? CV_RGB2GRAY : CV_BGR2GRAY));
        } else {
          if( m_nPbType[ii] == m_nOutPbType )
            sImg.convertTo(dImg, CV_8UC3, 255. / m_dRange);
          else {
            sImg.convertTo(aux, CV_8UC3, 255. / m_dRange);
            cv::cvtColor(aux, dImg,
              (m_nPbType[ii] == pb::Format::PB_RGB ? CV_RGB2BGR : CV_BGR2RGB));
          }
        }
        break;
    }
  }

  return true;
}

std::string ConvertDriver::GetDeviceProperty(const std::string& sProperty)
{
  return m_Input->GetDeviceProperty(sProperty);
}

size_t ConvertDriver::NumChannels() const
{
  return m_nNumChannels;
}

size_t ConvertDriver::Width( size_t idx ) const
{
  return m_nImgWidth[idx];
}

size_t ConvertDriver::Height( size_t idx ) const
{
  return m_nImgHeight[idx];
}

} // namespace
