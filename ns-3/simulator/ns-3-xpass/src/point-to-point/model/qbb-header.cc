#include <stdint.h>
#include <iostream>
#include "qbb-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("qbbHeader");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(qbbHeader);

	qbbHeader::qbbHeader(uint16_t pg)
		: m_pg(pg), sport(0), dport(0), flags(0), m_seq(0)
	{
	}

	qbbHeader::qbbHeader()
		: m_pg(0), sport(0), dport(0), flags(0), m_seq(0)
	{}

	qbbHeader::~qbbHeader()
	{}

	void qbbHeader::SetPG(uint16_t pg)
	{
		m_pg = pg;
	}

	void qbbHeader::SetSeq(uint32_t seq)
	{
		m_seq = seq;
	}

	void qbbHeader::SetSport(uint32_t _sport){
		sport = _sport;
	}
	void qbbHeader::SetDport(uint32_t _dport){
		dport = _dport;
	}
	void qbbHeader::SetWin(uint32_t winSize){
		m_win_size = winSize;
	} // 11/7 by sgh

	void qbbHeader::SetRtt(uint32_t rtt){
		m_rtt = rtt;
	}

	void qbbHeader::SetSyn(){
		flags |= (1 << FLAG_SYN);
	}
	void qbbHeader::SetFin(){
		flags |= (1 << FLAG_FIN);
	}
	void qbbHeader::SetAckDec(){
		flags &= (~(0x03));
		flags += VALVE_FLAG_ABE;
	}
	void qbbHeader::SetDataDec(){
		flags &= (~(0x03));
		flags += VALVE_FLAG_DBE;
	}
	void qbbHeader::SetInc(){
		flags &= (~(0x03));
		flags += VALVE_FLAG_INC;
	}
	void qbbHeader::SetReg(){
		flags &= (~(0x03));
		flags += VALVE_FLAG_REG;
	}

	uint16_t qbbHeader::GetPG() const
	{
		return m_pg;
	}

	uint32_t qbbHeader::GetSeq() const
	{
		return m_seq;
	}

	uint16_t qbbHeader::GetSport() const{
		return sport;
	}
	uint16_t qbbHeader::GetDport() const{
		return dport;
	}
	uint32_t qbbHeader::GetWin() const{
		return m_win_size;
	} // 11/7 by sgh

	uint32_t qbbHeader::GetRtt() const{
		return m_rtt;
	}

	u_int8_t qbbHeader::GetValveFlag() const{
		return flags & 0x03;
	}

	TypeId
		qbbHeader::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::qbbHeader")
			.SetParent<Header>()
			.AddConstructor<qbbHeader>()
			;
		return tid;
	}
	TypeId
		qbbHeader::GetInstanceTypeId(void) const
	{
		return GetTypeId();
	}
	void qbbHeader::Print(std::ostream &os) const
	{
		os << "qbb:" << "pg=" << m_pg << ",seq=" << m_seq;
	}
	uint32_t qbbHeader::GetSerializedSize(void)  const
	{
		return GetBaseSize();
	}
	uint32_t qbbHeader::GetBaseSize() {
		qbbHeader tmp;
		return sizeof(tmp.sport) + sizeof(tmp.dport) + sizeof(tmp.flags) + sizeof(tmp.m_pg) + sizeof(tmp.m_seq) + sizeof(tmp.m_win_size) + sizeof(tmp.m_rtt); // 11/7
	}
	void qbbHeader::Serialize(Buffer::Iterator start)  const
	{
		Buffer::Iterator i = start;
		i.WriteU16(sport);
		i.WriteU16(dport);
		i.WriteU16(flags);
		i.WriteU16(m_pg);
		i.WriteU32(m_seq);
		i.WriteU32(m_win_size); //11/7 by sgh
		i.WriteU32(m_rtt);
	}

	uint32_t qbbHeader::Deserialize(Buffer::Iterator start)
	{
		Buffer::Iterator i = start;
		sport = i.ReadU16();
		dport = i.ReadU16();
		flags = i.ReadU16();
		m_pg = i.ReadU16();
		m_seq = i.ReadU32();
		m_win_size = i.ReadU32();  //11/7 by sgh
		m_rtt = i.ReadU32();
		return GetSerializedSize();
	}
}; // namespace ns3
