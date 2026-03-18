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

#include "custom-header.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CustomHeader");

NS_OBJECT_ENSURE_REGISTERED(CustomHeader);

CustomHeader::CustomHeader()
    : brief(1),
      headerType(L3_Header | L4_Header),
      // ppp header
      pppProto(0),
      // IPv4 header
      m_payloadSize(0),
      ipid(0),
      m_tos(0),
      m_ttl(0),
      l3Prot(0),
      ipv4Flags(0),
      m_fragmentOffset(0),
      m_checksum(0),
      m_headerSize(5 * 4)
{
}

CustomHeader::CustomHeader(uint32_t _headerType)
    : brief(1),
      headerType(_headerType),
      // ppp header
      pppProto(0),
      // IPv4 header
      m_payloadSize(0),
      ipid(0),
      m_tos(0),
      m_ttl(0),
      l3Prot(0),
      ipv4Flags(0),
      m_fragmentOffset(0),
      m_checksum(0),
      m_headerSize(5 * 4)
{
}

TypeId
CustomHeader::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::CustomHeader")
                            .SetParent<Header>()
                            .SetGroupName("Network")
                            .AddConstructor<CustomHeader>();
    return tid;
}

TypeId
CustomHeader::GetInstanceTypeId(void) const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

void
CustomHeader::Print(std::ostream& os) const
{
}

uint32_t
CustomHeader::GetSerializedSize(void) const
{
    uint32_t len = 0;
    if (headerType & L2_Header)
        len += 14;
    if (headerType & L3_Header)
        len += 5 * 4;
    if (headerType & L4_Header)
    {
        if (l3Prot == 0x01)
            len += GetXpassHeaderSize();
    }
    return len;
}

/*
 * Function Description: 将CustomHeader结构体转化为packet比特串
 */
void CustomHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    // ppp
    if (headerType & L2_Header)
    {
        i.WriteHtonU16(pppProto);
        // skip 12 Bytes, so total 14 bytes as Ethernet
        i.WriteU64(0); // 8 bytes
        i.WriteU32(0); // 4 byets
    }

    // IPv4
    if (headerType & L3_Header)
    {
        uint8_t verIhl = (4 << 4) | (5);
        i.WriteU8(verIhl);
        i.WriteU8(m_tos);
        i.WriteHtonU16(m_payloadSize + 5 * 4);
        i.WriteHtonU16(ipid);
        uint32_t fragmentOffset = m_fragmentOffset / 8;
        uint8_t flagsFrag = (fragmentOffset >> 8) & 0x1f;
        if (ipv4Flags & DONT_FRAGMENT)
            flagsFrag |= (1 << 6);
        if (ipv4Flags & MORE_FRAGMENTS)
            flagsFrag |= (1 << 5);
        i.WriteU8(flagsFrag);
        uint8_t frag = fragmentOffset & 0xff;
        i.WriteU8(frag);
        i.WriteU8(m_ttl);
        i.WriteU8(l3Prot);
        i.WriteHtonU16(0);
        i.WriteHtonU32(sip);
        i.WriteHtonU32(dip);
    }

    // L4
    if (headerType & L4_Header)
    {
        if (l3Prot == 0x01) {
            i.WriteU16(cdt.sport);
            i.WriteU16(cdt.dport);
            i.WriteU16(cdt.flags);
            i.WriteU16(cdt.pg);
            i.WriteU32(cdt.m_seq);
            i.WriteU32(cdt.m_rtt);
            i.WriteU32(cdt.recv_next);
            i.WriteU8(cdt.mtype);
            i.WriteU8(cdt.ptype);

            i.WriteU32(cdt.credit_seq);
            i.WriteU32(cdt.credit_sent_time);
            i.WriteU32(cdt.sendbuffer);
        }
    }
}

/*
 * Function Description: 将packet比特串转化为CustomHeader结构体
 */
uint32_t CustomHeader::Deserialize(Buffer::Iterator start) {
    Buffer::Iterator i = start;

    // L2
    int l2Size = 0;
    if (headerType & L2_Header)
    {
        pppProto = i.ReadNtohU16();
        i.Next(12);
        l2Size = 14;
    }

    // L3
    int l3Size = 0;
    if (headerType & L3_Header)
    {
        i = start;
        i.Next(l2Size);

        uint8_t verIhl = i.ReadU8();
        uint8_t ihl = verIhl & 0x0f;
        uint16_t headerSize = ihl * 4;
        l3Size = headerSize;

        if ((verIhl >> 4) != 4)
        {
            NS_LOG_WARN("Trying to decode a non-IPv4 header, refusing to do it.");
            return 0;
        }

        if (brief)
        {
            m_tos = i.ReadU8();
            i.Next(2);
            ipid = i.ReadNtohU16();
            i.Next(2);
            m_ttl = i.ReadU8();
            l3Prot = i.ReadU8();
            i.Next(2);
            sip = i.ReadNtohU32();
            dip = i.ReadNtohU32();
        }
        else
        {
            m_tos = i.ReadU8();
            uint16_t size = i.ReadNtohU16();
            m_payloadSize = size - headerSize;
            ipid = i.ReadNtohU16();
            uint8_t flags = i.ReadU8();
            ipv4Flags = 0;
            if (flags & (1 << 6))
                ipv4Flags |= DONT_FRAGMENT;
            if (flags & (1 << 5))
                ipv4Flags |= MORE_FRAGMENTS;
            i.Prev();
            m_fragmentOffset = i.ReadU8() & 0x1f;
            m_fragmentOffset <<= 8;
            m_fragmentOffset |= i.ReadU8();
            m_fragmentOffset <<= 3;
            m_ttl = i.ReadU8();
            l3Prot = i.ReadU8();
            m_checksum = i.ReadU16();
            /* i.Next (2); // checksum */
            sip = i.ReadNtohU32();
            dip = i.ReadNtohU32();
            m_headerSize = headerSize;
        }
    }

    // L4
    int l4Size = 0;
    if (headerType & L4_Header)
    {
        if (l3Prot == 0x01) 
        {// Xpass
            cdt.sport = i.ReadU16();
            cdt.dport = i.ReadU16();
            cdt.flags = i.ReadU16();
            cdt.pg = i.ReadU16();
            cdt.m_seq = i.ReadU32();
            cdt.m_rtt = i.ReadU32();

            cdt.recv_next = i.ReadU32();
            cdt.mtype = i.ReadU8();
            cdt.ptype = i.ReadU8();
            cdt.credit_seq = i.ReadU32();
            cdt.credit_sent_time = i.ReadU32();
            cdt.sendbuffer = i.ReadU32();
            l4Size = GetXpassHeaderSize();
        }
    }

    return l2Size + l3Size + l4Size;
}

uint32_t
CustomHeader::GetXpassHeaderSize(void)
{
    return sizeof(cdt.sport) + sizeof(cdt.dport) + sizeof(cdt.flags) + sizeof(cdt.pg) + sizeof(cdt.m_seq) + sizeof(cdt.m_rtt) + sizeof(cdt.mtype)
            + sizeof(cdt.ptype) + sizeof(cdt.credit_seq) + sizeof(cdt.credit_seq) + sizeof(cdt.credit_sent_time) + sizeof(cdt.sendbuffer); 
}

uint32_t
CustomHeader::GetStaticWholeHeaderSize(void) {
    return 14 + 20 + GetXpassHeaderSize();
}

} // namespace ns3
