#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "fcp-header.h"

#include <iostream>

NS_LOG_COMPONENT_DEFINE ("FcpHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FcpHeader);

FcpHeader::FcpHeader ()
  : m_vl (0)
{
}
FcpHeader::~FcpHeader ()
{
}
TypeId
FcpHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FcpHeader")
    .SetParent<Header> ()
    .AddConstructor<FcpHeader> ()
  ;
  return tid;
}
TypeId
FcpHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
FcpHeader::SetVl (uint32_t vl)
{
  m_vl = vl;
}
uint32_t
FcpHeader::GetVl (void)
{
  return m_vl;
}

void
FcpHeader::SetFccl (uint32_t fccl)
{
  m_fccl = fccl;
}

uint32_t
FcpHeader::GetFccl (void)
{
  return m_fccl;
}

void
FcpHeader::Print (std::ostream &os) const
{
  //os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << ")";
  //os << m_seq << " " << TimeStep (m_ts).GetSeconds () << " " << m_pg;
  os << m_vl << " " << m_fccl;
}
uint32_t
FcpHeader::GetSerializedSize (void) const
{
  return GetHeaderSize();
}
uint32_t FcpHeader::GetHeaderSize(void){
  return 4 + 4; 
}

void
FcpHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_vl);
  i.WriteHtonU32 (m_fccl);
}
uint32_t
FcpHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_vl =  i.ReadNtohU32 ();
  m_fccl = i.ReadNtohU32 ();
  return GetSerializedSize ();
}

} // namespace ns3

