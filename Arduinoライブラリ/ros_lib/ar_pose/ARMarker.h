#ifndef _ROS_ar_pose_ARMarker_h
#define _ROS_ar_pose_ARMarker_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ros/msg.h"
#include "std_msgs/Header.h"
#include "geometry_msgs/PoseWithCovariance.h"

namespace ar_pose
{

  class ARMarker : public ros::Msg
  {
    public:
      typedef std_msgs::Header _header_type;
      _header_type header;
      typedef uint32_t _id_type;
      _id_type id;
      typedef geometry_msgs::PoseWithCovariance _pose_type;
      _pose_type pose;
      typedef uint32_t _confidence_type;
      _confidence_type confidence;

    ARMarker():
      header(),
      id(0),
      pose(),
      confidence(0)
    {
    }

    virtual int serialize(unsigned char *outbuffer) const
    {
      int offset = 0;
      offset += this->header.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->id >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->id >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (this->id >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (this->id >> (8 * 3)) & 0xFF;
      offset += sizeof(this->id);
      offset += this->pose.serialize(outbuffer + offset);
      *(outbuffer + offset + 0) = (this->confidence >> (8 * 0)) & 0xFF;
      *(outbuffer + offset + 1) = (this->confidence >> (8 * 1)) & 0xFF;
      *(outbuffer + offset + 2) = (this->confidence >> (8 * 2)) & 0xFF;
      *(outbuffer + offset + 3) = (this->confidence >> (8 * 3)) & 0xFF;
      offset += sizeof(this->confidence);
      return offset;
    }

    virtual int deserialize(unsigned char *inbuffer)
    {
      int offset = 0;
      offset += this->header.deserialize(inbuffer + offset);
      this->id =  ((uint32_t) (*(inbuffer + offset)));
      this->id |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      this->id |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      this->id |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      offset += sizeof(this->id);
      offset += this->pose.deserialize(inbuffer + offset);
      this->confidence =  ((uint32_t) (*(inbuffer + offset)));
      this->confidence |= ((uint32_t) (*(inbuffer + offset + 1))) << (8 * 1);
      this->confidence |= ((uint32_t) (*(inbuffer + offset + 2))) << (8 * 2);
      this->confidence |= ((uint32_t) (*(inbuffer + offset + 3))) << (8 * 3);
      offset += sizeof(this->confidence);
     return offset;
    }

    const char * getType(){ return "ar_pose/ARMarker"; };
    const char * getMD5(){ return "93c4ce9061a70bc30293e52ac4675f76"; };

  };

}
#endif