//yibo

#ifndef QBB_HEADER_H
#define QBB_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"

#define VALVE_FLAG_INC 0x03
#define VALVE_FLAG_REG 0x00
#define VALVE_FLAG_DBE 0x01
#define VALVE_FLAG_ABE 0x02


namespace ns3 {

/**
 * \ingroup Pause
 * \brief Header for the Congestion Notification Message
 *
 * This class has two fields: The five-tuple flow id and the quantized
 * congestion level. This can be serialized to or deserialzed from a byte
 * buffer.
 */
 
class qbbHeader : public Header
{
public:
 
  enum {
    FLAG_RC1 = 0,
    FLAG_RC2,
    FLAG_SYN,
    FLAG_FIN
  };
  qbbHeader (uint16_t pg);
  qbbHeader ();
  virtual ~qbbHeader ();

//Setters
  /**
   * \param pg The PG
   */
  void SetPG (uint16_t pg);
  void SetSeq(uint32_t seq);
  void SetSport(uint32_t _sport);
  void SetDport(uint32_t _dport);
  void SetWin(uint32_t winSize); //11/7 by sgh
  void SetRtt(uint32_t rtt);
  void SetSyn();
  void SetFin();
  void SetDataDec();
  void SetAckDec();
  void SetInc();
  void SetReg();
  

//Getters
  /**
   * \return The pg
   */
  uint16_t GetPG () const;
  uint32_t GetSeq() const;
  uint16_t GetPort() const;
  uint16_t GetSport() const;
  uint16_t GetDport() const;
  uint32_t GetWin() const; //11/7 by sgh
  uint32_t GetRtt() const;
  u_int8_t GetValveFlag() const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  static uint32_t GetBaseSize(); // size without INT

private:
  uint16_t sport, dport;
  uint16_t flags;
  uint16_t m_pg;
  uint32_t m_seq; // the qbb sequence number.
  uint32_t m_win_size; // 11/7 by sgh
  uint32_t m_rtt;
};

}; // namespace ns3

#endif /* QBB_HEADER */
