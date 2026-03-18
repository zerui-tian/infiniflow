/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
#include "counterIncr-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CounterIncr-Tag");

NS_OBJECT_ENSURE_REGISTERED (CounterIncrTag);

TypeId 
CounterIncrTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CounterIncrTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<CounterIncrTag> ()
  ;
  return tid;
}
TypeId 
CounterIncrTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
CounterIncrTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 2;
}
void 
CounterIncrTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU16 (m_tag);
}
void 
CounterIncrTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  m_tag = buf.ReadU16 ();
}
void 
CounterIncrTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "CounterIncr=" << m_tag;
}
CounterIncrTag::CounterIncrTag ()
  : Tag () 
{
  NS_LOG_FUNCTION (this);
}
void
CounterIncrTag::SetTag (uint16_t tag)
{
  // NS_LOG_FUNCTION (this << tag);
  m_tag = tag;
}
uint16_t
CounterIncrTag::GetTag (void) const
{
  NS_LOG_FUNCTION (this);
  return m_tag;
}

} // namespace ns3