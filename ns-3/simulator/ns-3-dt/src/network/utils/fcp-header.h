
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

    void SetQlen(uint32_t qlen);
    
    void SetFccl(uint32_t fccl);
    
    void SetFccr(uint32_t fccr);

    void SetTap();
    void ResetTap();
    bool GetTap();

    uint32_t GetVl();

    uint32_t GetQlen();

    uint32_t GetFccl();

    uint32_t GetFccr();

    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    static uint32_t GetHeaderSize(void);
    //private: // Some errors. Too frustrated. Just making it public for now.
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);

    
    uint16_t m_vl;
    uint16_t m_flags;
    uint32_t m_qlen;
    uint32_t m_fccl;
    uint32_t m_fccr;

};

} // namespace ns3

#endif /* FCP_HEADER_H */
