#ifndef QBB_HEADER_H
#define QBB_HEADER_H

#include "ns3/header.h"

namespace ns3 {

/**
 * \ingroup Pause
 * \brief Header for the Congestion Notification Message
 *
 * This class has two fields: The five-tuple flow id and the quantized
 * congestion level. This can be serialized to or deserialzed from a byte
 * buffer.
 */
 
class XpassHeader : public Header
{
public:
  static TypeId GetTypeId (void);
  XpassHeader (uint16_t pg);
  XpassHeader ();
  virtual ~XpassHeader ();

//Setters
  /**
   * \param pg The PG
   */
  void SetPG (uint16_t pg);
  void SetSeq(uint32_t seq);
  void SetSport(uint32_t _sport);
  void SetDport(uint32_t _dport);
  void SetRtt(uint32_t rtt);
  void SetMtype(uint8_t _mtype) {
    mtype = _mtype;
  };
  void SetAckS(uint32_t recv_next_) {
    recv_next = recv_next_;
  }
  void SetCredit_Sent_Time(uint32_t credit_sent_time_) {
    credit_sent_time = credit_sent_time_;
  };
  void SetSendBuffer(uint32_t sendbuffer_){
    sendbuffer = sendbuffer_;
  }
  void SetCreditSeq(uint32_t credit_seq_){
    credit_seq = credit_seq_;
  }
//Getters
  /**
   * \return The pg
   */
  uint16_t GetPG () const;
  uint32_t GetSeq() const;
  uint16_t GetSport() const;
  uint16_t GetDport() const;
  uint32_t GetRtt() const;
  uint8_t GetMtype() const {
    return mtype;
  };
  uint32_t GetAckSeq() const {
    return recv_next;
  };
  uint32_t GetCredit_Sent_Time() const {
    return credit_sent_time;
  };
  uint32_t GetSendBuffer(){
    return sendbuffer;
  };
  uint32_t GetCreditSeq() {
    return credit_seq;
  }

  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  static uint32_t GetBaseSize(); // size without INT

private:
  uint16_t sport, dport;
  uint16_t flags;
  uint16_t pg;
  uint32_t m_seq; /* sequence number */
  uint32_t m_rtt;
   
  uint32_t recv_next;  /* ACK number for FullTcp */
  uint8_t mtype; //表示所发消息类型0x01表示request,0x02表示credit,0x03表示data,0x04表示ack,0x05表示fin1,0x06表示fin2

   //cmnh
   uint8_t ptype; //表示状态close,sending等

   //xph
   uint32_t credit_seq;
   uint32_t credit_sent_time;
   uint32_t sendbuffer;
};

}; // namespace ns3

#endif /* QBB_HEADER */
