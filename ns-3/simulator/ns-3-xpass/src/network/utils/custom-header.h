/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef CUSTOM_HEADER_H
#define CUSTOM_HEADER_H

#include "ns3/header.h"
#include "ns3/int-header.h"

namespace ns3
{
/**
 * \ingroup ipv4
 *
 * \brief Custom packet header
 */
class CustomHeader : public Header
{
  public:
    /**
     * \brief Construct a null custom header
     */
    CustomHeader();
    CustomHeader(uint32_t _headerType);

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual void Print(std::ostream& os) const;
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);

    uint32_t brief, headerType;

    enum HeaderType
    {
        L2_Header = 1,
        L3_Header = 2,
        L4_Header = 4
    };

    enum UdpFlags
    {
        FLAG_SYN = 0,
        FLAG_FIN
    };

    // ppp header
    uint16_t pppProto;

    // IPv4 header
    enum FlagsE
    {
        DONT_FRAGMENT = (1 << 0),
        MORE_FRAGMENTS = (1 << 1)
    };

    uint16_t m_payloadSize;    //!< payload size
    uint16_t ipid;             //!< identification
    uint32_t m_tos : 8;        //!< TOS
    uint32_t m_ttl : 8;        //!< TTL
    uint32_t l3Prot : 8;       //!< Protocol,0
    uint32_t ipv4Flags : 3;    //!< flags
    uint16_t m_fragmentOffset; //!< Fragment offset
    uint32_t sip;              //!< source address
    uint32_t dip;              //!< destination address
    uint16_t m_checksum;       //!< checksum
    uint16_t m_headerSize;     //!< IP header size

    union{
    
        struct {
            //tcp
            uint16_t sport, dport;
            uint16_t flags;
            uint16_t pg;
            uint32_t m_seq; // the qbb sequence number.
            uint32_t m_rtt;

            uint32_t recv_next;
            uint8_t mtype; //表示所发消息类型0x01表示request,0x02表示credit,0x03表示data

            //cmnh
            uint8_t ptype; //表示状态close,sending等

            //xph
            uint32_t credit_seq;
            uint32_t credit_sent_time;
            uint32_t sendbuffer;
        } cdt;
        
    };

    static uint32_t GetXpassHeaderSize(void); 
    static uint32_t GetStaticWholeHeaderSize(void);
};

} // namespace ns3

#endif /* CUSTOM_HEADER_H */
