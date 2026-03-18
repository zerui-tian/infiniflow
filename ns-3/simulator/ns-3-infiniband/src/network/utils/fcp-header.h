
#ifndef FCP_HEADER_H
#define FCP_HEADER_H

#include "ns3/header.h"

#include <string>

namespace ns3
{

class FcpHeader : public Header
{
public:
    static TypeId GetTypeId (void);
    FcpHeader ();
    virtual ~FcpHeader();

    void SetVl(uint32_t vl);
    
    void SetFccl(uint32_t fccl);

    uint32_t GetVl();

    uint32_t GetFccl();

    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    static uint32_t GetHeaderSize(void);
    //private: // Some errors. Too frustrated. Just making it public for now.
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);

    
    uint32_t m_vl;
    uint32_t m_fccl;

};

} // namespace ns3

#endif /* FCP_HEADER_H */
