
#include "xpass-header.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"

#include <iostream>

NS_LOG_COMPONENT_DEFINE("XpassHeader");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(XpassHeader);

	XpassHeader::XpassHeader(uint16_t _pg)
		: pg(_pg), sport(0), dport(0), flags(0), m_seq(0)
	{
	}

	XpassHeader::XpassHeader()
		: pg(0), sport(0), dport(0), flags(0), m_seq(0)
	{}

	XpassHeader::~XpassHeader()
	{}

	void XpassHeader::SetPG(uint16_t _pg)
	{
		pg = _pg;
	}

	void XpassHeader::SetSeq(uint32_t seq)
	{
		m_seq = seq;
	}

	void XpassHeader::SetSport(uint32_t _sport){
		sport = _sport;
	}
	void XpassHeader::SetDport(uint32_t _dport){
		dport = _dport;
	}

	void XpassHeader::SetRtt(uint32_t rtt){
		m_rtt = rtt;
	}


	uint16_t XpassHeader::GetPG() const
	{
		return pg;
	}

	uint32_t XpassHeader::GetSeq() const
	{
		return m_seq;
	}

	uint16_t XpassHeader::GetSport() const{
		return sport;
	}
	uint16_t XpassHeader::GetDport() const{
		return dport;
	}


	uint32_t XpassHeader::GetRtt() const{
		return m_rtt;
	}


	TypeId XpassHeader::GetTypeId (void){
		static TypeId tid = TypeId ("ns3::XpassHeader")
			.SetParent<Header> ()
			.AddConstructor<XpassHeader> ()
		;
		return tid;
	}
	TypeId XpassHeader::GetInstanceTypeId (void) const{
		return GetTypeId ();
	}
	void XpassHeader::Print (std::ostream &os) const
	{
	//os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << ")";
	//os << m_seq << " " << TimeStep (m_ts).GetSeconds () << " " << pg;
		os << mtype << " " << m_seq;
	}
	uint32_t XpassHeader::GetSerializedSize(void)  const
	{
		return GetBaseSize();
	}
	uint32_t XpassHeader::GetBaseSize() {
		XpassHeader tmp;
		return sizeof(tmp.sport) + sizeof(tmp.dport) + sizeof(tmp.flags) + sizeof(tmp.pg) + sizeof(tmp.m_seq) + sizeof(tmp.m_rtt) + sizeof(tmp.mtype)
            + sizeof(tmp.ptype) + sizeof(tmp.credit_seq) + sizeof(tmp.credit_seq) + sizeof(tmp.credit_sent_time) + sizeof(tmp.sendbuffer); // 3/26
	}
	void XpassHeader::Serialize(Buffer::Iterator start)  const
	{
		Buffer::Iterator i = start;
		i.WriteU16(sport);
		i.WriteU16(dport);
		i.WriteU16(flags);
		i.WriteU16(pg);
		i.WriteU32(m_seq);
		i.WriteU32(m_rtt);
		i.WriteU32(recv_next);
		i.WriteU8(mtype);
		i.WriteU8(ptype);

		i.WriteU32(credit_seq);
		i.WriteU32(credit_sent_time);
		i.WriteU32(sendbuffer);
	}

	uint32_t XpassHeader::Deserialize(Buffer::Iterator start)
	{
		Buffer::Iterator i = start;
		sport = i.ReadU16();
		dport = i.ReadU16();
		flags = i.ReadU16();
		pg = i.ReadU16();
		m_seq = i.ReadU32();
		m_rtt = i.ReadU32();

		recv_next = i.ReadU32();
		mtype = i.ReadU8(); //表示所发消息类型0x01表示request,0x02表示credit,0x03表示data
	  
		 //cmnh
		 ptype = i.ReadU8(); //表示状态close,sending等
	  
		 //xph
		 credit_seq = i.ReadU32();
		 credit_sent_time = i.ReadU32();
		 sendbuffer = i.ReadU32();
		return GetSerializedSize();
	}
}; // namespace ns3
