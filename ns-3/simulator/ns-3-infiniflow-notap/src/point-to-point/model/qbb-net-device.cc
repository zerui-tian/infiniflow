#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <stdio.h>
#include "ns3/qbb-net-device.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/object-vector.h"
#include "ns3/pause-header.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/assert.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/qbb-channel.h"
#include "ns3/random-variable.h"
#include "ns3/qbb-header.h"
#include "ns3/error-model.h"
#include "ns3/cn-header.h"
#include "ns3/ppp-header.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"
#include "ns3/pointer.h"
#include "ns3/custom-header.h"
#include "ns3/rdma-tag.h"
#include "ns3/interface-tag.h"
#include "ns3/unsched-tag.h"
#include "ns3/fcp-header.h"
#include "switch-node.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE("QbbNetDevice");

#define OUTPUT_1 0

namespace ns3 {

// RdmaEgressQueue
TypeId RdmaEgressQueue::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::RdmaEgressQueue")
	                    .SetParent<Object> ()
	                    .AddTraceSource ("RdmaEnqueue", "Enqueue a packet in the RdmaEgressQueue.",
	                                     MakeTraceSourceAccessor (&RdmaEgressQueue::m_traceRdmaEnqueue), "ns3::Packet::TracedCallback")
	                    .AddTraceSource ("RdmaDequeue", "Dequeue a packet in the RdmaEgressQueue.",
	                                     MakeTraceSourceAccessor (&RdmaEgressQueue::m_traceRdmaDequeue), "ns3::Packet::TracedCallback")
	                    ;
	return tid;
}

RdmaEgressQueue::RdmaEgressQueue() {
	m_rrlast = 0;
	m_qlast = 0;
	m_ackQ = CreateObject<DropTailQueue<Packet>>();
	m_ackQ->SetAttribute("MaxSize", QueueSizeValue (QueueSize (BYTES, 0xffffffff))); // queue limit is on a higher level, not here
	m_fcpQ = CreateObject<DropTailQueue<Packet>>();
	m_fcpQ->SetAttribute("MaxSize", QueueSizeValue (QueueSize (BYTES, 0xffffffff))); // queue limit is on a higher level, not here
}

Ptr<Packet> RdmaEgressQueue::DequeueQindex(int qIndex) {
	Ptr<Packet> p;
	if (qIndex == -1) { // ack
		p = m_ackQ->Dequeue();
		m_qlast = -1;
		m_traceRdmaDequeue(p, 1);
	}
	else if (qIndex == -2){ // fcp
		p = m_fcpQ->Dequeue();
		m_qlast = -2;
		m_traceRdmaDequeue(p, 0);
	}
	else if (qIndex >= 0) { // qp
		p = m_rdmaGetNxtPkt(m_qpGrp->Get(qIndex));
		m_rrlast = qIndex;
		m_qlast = qIndex;
		m_traceRdmaDequeue(p, m_qpGrp->Get(qIndex)->m_pg);
	}
	else {
		p = 0;
	}
	return p;
}
int RdmaEgressQueue::GetNextQindex(uint32_t credit, uint32_t inflight[], uint32_t thresholds[]) {
	bool found = false;
	uint32_t qIndex;
	
	if (m_fcpQ->GetNPackets() > 0) {
		return -2;
	}
	else if (m_ackQ->GetNPackets() > 0 && 
			credit >= m_ackQ->Peek()->GetSize() &&
			(thresholds[1] - inflight[1]) >= m_ackQ->Peek()->GetSize()
		){
		return -1;
	}

	// no pkt in highest priority queue, do rr for each qp
	int res = -1024;

	uint32_t fcount = m_qpGrp->GetN();
	uint32_t min_finish_id = 0xffffffff;
	for (qIndex = 1; qIndex <= fcount; qIndex++) {
		uint32_t idx = (qIndex + m_rrlast) % fcount;
		Ptr<RdmaQueuePair> qp = m_qpGrp->Get(idx);
		if (qp->GetBytesLeft() > 0) {
		// if (qp->GetBytesLeft() > 0 && !qp->IsWinBound()) {
			if (m_qpGrp->Get(idx)->m_nextAvail.GetTimeStep() > Simulator::Now().GetTimeStep()) //not available now
				continue;
			uint32_t pkt_size = (qp->GetBytesLeft() > m_mtu ? m_mtu : qp->GetBytesLeft()) + CustomHeader::GetStaticWholeHeaderSize();
			if( credit < pkt_size || (thresholds[qp->m_pg] <= pkt_size + inflight[qp->m_pg]) ) // no enough credits
			// if( credit < pkt_size ) // no enough credits
				continue;
						res = idx;
			break;
					}
		else if (qp->IsFinished()) {
			min_finish_id = idx < min_finish_id ? idx : min_finish_id;
		}
	}

	// clear the finished qp
	if (min_finish_id < 0xffffffff) {
		int nxt = min_finish_id;
		auto &qps = m_qpGrp->m_qps;
		for (int i = min_finish_id + 1; i < fcount; i++) if (!qps[i]->IsFinished()) {
				if (i == res) // update res to the idx after removing finished qp
					res = nxt;
				qps[nxt] = qps[i];
				nxt++;
			}
		qps.resize(nxt);
	}
	return res;
}

int RdmaEgressQueue::GetLastQueue() {
	return m_qlast;
}

uint32_t RdmaEgressQueue::GetNBytes(uint32_t qIndex) {
	NS_ASSERT_MSG(qIndex < m_qpGrp->GetN(), "RdmaEgressQueue::GetNBytes: qIndex >= m_qpGrp->GetN()");
	return m_qpGrp->Get(qIndex)->GetBytesLeft();
}

uint32_t RdmaEgressQueue::GetFlowCount(void) {
	return m_qpGrp->GetN();
}

Ptr<RdmaQueuePair> RdmaEgressQueue::GetQp(uint32_t i) {
	return m_qpGrp->Get(i);
}

void RdmaEgressQueue::RecoverQueue(uint32_t i) {
	NS_ASSERT_MSG(i < m_qpGrp->GetN(), "RdmaEgressQueue::RecoverQueue: qIndex >= m_qpGrp->GetN()");
	m_qpGrp->Get(i)->snd_nxt = m_qpGrp->Get(i)->snd_una;
}

void RdmaEgressQueue::EnqueueAckQ(Ptr<Packet> p) {
	m_traceRdmaEnqueue(p, 0);
	m_ackQ->Enqueue(p);
}

void RdmaEgressQueue::EnqueueFcpQ(Ptr<Packet> p) {
	m_traceRdmaEnqueue(p, 0);
	m_fcpQ->Enqueue(p);
}

/******************
 * QbbNetDevice
 *****************/
NS_OBJECT_ENSURE_REGISTERED(QbbNetDevice);

TypeId
QbbNetDevice::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::QbbNetDevice")
	                    .SetParent<PointToPointNetDevice>()
	                    .AddConstructor<QbbNetDevice>()
	                    .AddAttribute("QbbEnabled",
	                                  "Enable the generation of PAUSE packet.",
	                                  BooleanValue(true),
	                                  MakeBooleanAccessor(&QbbNetDevice::m_qbbEnabled),
	                                  MakeBooleanChecker())
	                    .AddAttribute("QcnEnabled",
	                                  "Enable the generation of PAUSE packet.",
	                                  BooleanValue(false),
	                                  MakeBooleanAccessor(&QbbNetDevice::m_qcnEnabled),
	                                  MakeBooleanChecker())
	                    .AddAttribute ("TxBeQueue",
	                                   "A queue to use as the transmit queue in the device.",
	                                   PointerValue (),
	                                   MakePointerAccessor (&QbbNetDevice::m_queue),
	                                   MakePointerChecker<Queue<Packet>>())
	                    .AddAttribute ("RdmaEgressQueue",
	                                   "A queue to use as the transmit queue in the device.",
	                                   PointerValue (),
	                                   MakePointerAccessor (&QbbNetDevice::m_rdmaEQ),
	                                   MakePointerChecker<Object> ())
	                    .AddTraceSource ("QbbEnqueue", "Enqueue a packet in the QbbNetDevice.",
	                                     MakeTraceSourceAccessor (&QbbNetDevice::m_traceEnqueue), "ns3::Packet::TracedCallback")
	                    .AddTraceSource ("QbbDequeue", "Dequeue a packet in the QbbNetDevice.",
	                                     MakeTraceSourceAccessor (&QbbNetDevice::m_traceDequeue), "ns3::Packet::TracedCallback")
	                    .AddTraceSource ("QbbDrop", "Drop a packet in the QbbNetDevice.",
	                                     MakeTraceSourceAccessor (&QbbNetDevice::m_traceDrop), "ns3::Packet::TracedCallback")
	                    .AddTraceSource ("RdmaQpDequeue", "A qp dequeue a packet.",
	                                     MakeTraceSourceAccessor (&QbbNetDevice::m_traceQpDequeue), "ns3::Packet::TracedCallback")
	                    .AddTraceSource ("QbbPfc", "get a PFC packet. 0: resume, 1: pause",
	                                     MakeTraceSourceAccessor (&QbbNetDevice::m_tracePfc), "ns3::Packet::TracedCallback")
	                    ;

	return tid;
}

QbbNetDevice::QbbNetDevice()
{
	NS_LOG_FUNCTION(this);
	m_ecn_source = new std::vector<ECNAccount>;
	m_rdmaEQ = CreateObject<RdmaEgressQueue>();
	m_rdmaEQ->qb_dev = this;
	perpt_fctbs = 0;
	for(int i = 0; i < qCnt; i++){
		perpq_accumu_drained_bytes[i] = 0;
		perpq_accumu_recycled_credit[i] = 0;
		perpq_accumu_used_credit[i] = 0;
		perpg_inflight_threshold[i] = 0;
	}
	memset(need_set_tap, 0, sizeof(need_set_tap));
	memset(need_ret_tap, 0, sizeof(need_ret_tap));
	memset(rate_adjusting, 0, sizeof(rate_adjusting));
	}

QbbNetDevice::~QbbNetDevice()
{
	NS_LOG_FUNCTION(this);
}

void
QbbNetDevice::DoDispose()
{
	NS_LOG_FUNCTION(this);

	PointToPointNetDevice::DoDispose();
}

DataRate QbbNetDevice::GetDataRate() {
	return m_bps;
}
Ptr<NetDevice>
QbbNetDevice::GetRemoteDevice(void) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_channel->GetNDevices() == 2);
    for (std::size_t i = 0; i < m_channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> tmp = m_channel->GetDevice(i);
        if (tmp != this)
        {
            return tmp;
        }
    }
    NS_ASSERT(false);
    // quiet compiler.
    return NULL;
}
bool
QbbNetDevice::TransmitStart(Ptr<Packet> p)
{
	NS_LOG_FUNCTION(this << p);
	NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
	//
	// This function is called to start the process of transmitting a packet.
	// We need to tell the channel that we've started wiggling the wire and
	// schedule an event that will be executed when the transmission is complete.
	//
	NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
	m_txMachineState = BUSY;
	m_currentPkt = p;
	m_phyTxBeginTrace(m_currentPkt);

	Time txTime = m_bps.CalculateBytesTxTime(p->GetSize());
	Time txCompleteTime = txTime + m_tInterframeGap;

	NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
	Simulator::Schedule(txCompleteTime, &QbbNetDevice::TransmitComplete, this);

	bool result = m_channel->TransmitStart(p, this, txTime);
	if (result == false)
	{
		m_phyTxDropTrace(p);
	}
	return result;
}

void
QbbNetDevice::TransmitComplete(void)
{
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
	m_txMachineState = READY;
	NS_ASSERT_MSG(m_currentPkt != 0, "QbbNetDevice::TransmitComplete(): m_currentPkt zero");
	m_phyTxEndTrace(m_currentPkt);
	m_currentPkt = 0;
	DequeueAndTransmit();
}

void
QbbNetDevice::DequeueAndTransmit(void)
{
	int qIndex;

	NS_LOG_FUNCTION(this);
	if (!m_linkUp) return; // if link is down, return
	if (m_txMachineState == BUSY) return;	// Quit if channel busy
	Ptr<Packet> p;
	if (m_node->GetNodeType() == 0) {
		uint32_t credit = perpt_fccl - perpt_fctbs;
		uint32_t inflight_counters[qCnt];
		uint32_t thresholds[qCnt];
		for(int qu = 1; qu < qCnt; qu++){
			inflight_counters[qu] = perpq_accumu_used_credit[qu] - perpq_accumu_recycled_credit[qu];
			thresholds[qu] = perpg_inflight_threshold[qu];
		}

		
		qIndex = m_rdmaEQ->GetNextQindex(credit, inflight_counters, thresholds);

		if (qIndex != -1024) {
			p = m_rdmaEQ->DequeueQindex(qIndex);
			if(qIndex == -2) { // fcp
				m_traceDequeue(p, 0);
				TransmitStart(p);
				totalBytesSent += p->GetSize();
			}
			else if (qIndex == -1) { // ack
				m_traceDequeue(p, 1);
				totalBytesSent += p->GetSize();
				perpt_fctbs += p->GetSize(); // update fctbs
				perpq_accumu_used_credit[1] += p->GetSize();
				if(need_set_tap[1]){
					PppHeader ppp;
					Ipv4Header h;
					p->RemoveHeader(ppp);
					p->RemoveHeader(h);
					h.SetTap();
					// std::cout << "set tap for ack packet at node " << m_node->GetId() << " at port " << GetIfIndex() << "qIndex 1" << std::endl;
					p->AddHeader(h);
					p->AddHeader(ppp);
					need_set_tap[1] = false;
				}
				else {
					PppHeader ppp;
					Ipv4Header h;
					p->RemoveHeader(ppp);
					p->RemoveHeader(h);
					h.ResetTap();
					// std::cout << "reset tap for ack packet at node " << m_node->GetId() << " at port " << GetIfIndex() << "qIndex 1" << std::endl;
					p->AddHeader(h);
					p->AddHeader(ppp);
				}
				TransmitStart(p);
				#if OUTPUT_1
				std::cout << "Time " << Simulator::Now().GetNanoSeconds() << " fccl " << perpt_fccl 
				<< " fctbs " << perpt_fctbs << 
                 " inflight " << perpq_accumu_used_credit[1] - perpq_accumu_recycled_credit[1]
				 << " node " << m_node->GetId() 
				 << " port " << GetIfIndex() << " qIndex 1" << std::endl;
				#endif
			}
			else { // qps
				Ptr<RdmaQueuePair> lastQp = m_rdmaEQ->GetQp(qIndex);
				m_traceQpDequeue(p, lastQp);
				totalBytesSent += p->GetSize();
				perpt_fctbs += p->GetSize();
				perpq_accumu_used_credit[lastQp->m_pg] += p->GetSize();
				if(need_set_tap[lastQp->m_pg]){
					PppHeader ppp;
					Ipv4Header h;
					p->RemoveHeader(ppp);
					p->RemoveHeader(h);
					h.SetTap();
					// std::cout << "set tap for ack packet at node " << m_node->GetId() << " at port " << GetIfIndex() << "qIndex " << lastQp->m_pg << std::endl;
					p->AddHeader(h);
					p->AddHeader(ppp);
					need_set_tap[lastQp->m_pg] = false;
				}
				else {
					PppHeader ppp;
					Ipv4Header h;
					p->RemoveHeader(ppp);
					p->RemoveHeader(h);
					h.ResetTap();
					// std::cout << "reset tap for ack packet at node " << m_node->GetId() << " at port " << GetIfIndex() << "qIndex " << lastQp->m_pg << std::endl;
					p->AddHeader(h);
					p->AddHeader(ppp);
				}
				// update for the next avail time
				m_rdmaPktSent(lastQp, p, m_tInterframeGap);
				TransmitStart(p);
				if (m_txBytesPerFlow.find(lastQp->dport) != m_txBytesPerFlow.end()) {
    				m_txBytesPerFlow[lastQp->dport] += p->GetSize();
				}
				#if OUTPUT_1
				std::cout << "Time " << Simulator::Now().GetNanoSeconds() << " fccl " << perpt_fccl <<
				 " fctbs " << perpt_fctbs <<
				 " perpq_accumu_used_credit " << perpq_accumu_used_credit[lastQp->m_pg] << 
				 " perpq_accumu_recycled_credit " << perpq_accumu_recycled_credit[lastQp->m_pg] <<
                 " inflight " << perpq_accumu_used_credit[lastQp->m_pg] - perpq_accumu_recycled_credit[lastQp->m_pg]
				 << " node " << m_node->GetId() 
				 << " port " << GetIfIndex() << " qIndex " << lastQp->m_pg <<  std::endl;
			
				#endif
							}
		}
	}
	else {  //switch, doesn't care about qcn, just send
		Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(m_node);
		uint32_t credit;
		if(sw->m_mmu->fccl[m_ifIndex] >= sw->m_mmu->fctbs[m_ifIndex]){
			credit = sw->m_mmu->fccl[m_ifIndex] - sw->m_mmu->fctbs[m_ifIndex];
		}
		else{
			std::cout << "error! negative credit!";
			credit = 0;
		}

		uint32_t inflight_counter[qCnt];
		uint32_t threshold[qCnt];
		for(int i = 0; i < qCnt; i++){
			if(sw->m_mmu->accumu_used_credit[m_ifIndex][i] >= sw->m_mmu->accumu_recycled_credit[m_ifIndex][i]){
				inflight_counter[i] = sw->m_mmu->accumu_used_credit[m_ifIndex][i] - sw->m_mmu->accumu_recycled_credit[m_ifIndex][i];
			}
			else{
				inflight_counter[i] = 0;
				std::cout << "error! negative inflight_counter!";
			}
			
			threshold[i] = sw->m_mmu->inflight_threshold[m_ifIndex][i];
		}
		
		p = m_queue->DequeueRR(credit, inflight_counter, threshold);		//this is round-robin
		if (p != 0) {
			m_snifferTrace(p);
			m_promiscSnifferTrace(p);

			CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
			p->PeekHeader(ch);
			
			InterfaceTag t;
			uint32_t qIndex = m_queue->GetLastQueue();
			if(qIndex != 0) {// is not fcp
				m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);
				p->RemovePacketTag(t);
			}
			m_traceDequeue(p, qIndex);
			
			TransmitStart(p);
			totalBytesSent += p->GetSize();
			if (m_txBytesPerFlow.find(ch.udp.dport) != m_txBytesPerFlow.end()) {
    			m_txBytesPerFlow[ch.udp.dport] += p->GetSize();
			}
		}
	}
	return;
}

void
QbbNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
	m_rxCallback = cb;
}

void
QbbNetDevice::Receive(Ptr<Packet> packet)
{
// std::cout << "receive" << std::endl;
	NS_LOG_FUNCTION(this << packet);
	if (!m_linkUp) {
		m_traceDrop(packet, 0);
		return;
	}

	if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
	{
		//
		// If we have an error model and it indicates that it is time to lose a
		// corrupted packet, don't forward this packet up, let it go.
		//
		m_phyRxDropTrace(packet);
		return;
	}

	m_macRxTrace(packet);
	PppHeader ph;
	packet->PeekHeader(ph);
	uint16_t pppProto = ph.GetProtocol();
	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	ch.getInt = 1; // parse INT header
	
	if(pppProto == 0x0086) {
		if (m_node->GetNodeType() > 0) {
			
			DynamicCast<SwitchNode>(m_node)->SwitchReceiveFcp(packet, m_ifIndex);
		}
		else {
						PppHeader ph;
			packet->RemoveHeader(ph);
			FcpHeader fh;
			packet->RemoveHeader(fh);
			uint32_t qIndex = fh.GetVl(); 
			uint32_t fccl = fh.GetFccl();
			uint32_t fccr = fh.GetFccr();
			uint32_t qlen = fh.GetQlen();
			bool tap = fh.GetTap();
			perpt_fccl = fccl;
			perpq_accumu_recycled_credit[qIndex] = fccr;
			if(tap){
				rate_adjusting[qIndex] = false;
			}
			if(true){
				int32_t new_threshold;
				if(qlen > qlen_max){
					uint32_t inflight_bytes = perpq_accumu_used_credit[qIndex] - perpq_accumu_recycled_credit[qIndex];
					new_threshold = inflight_bytes + qlen_min - qlen;
					perpg_inflight_threshold[qIndex] = new_threshold > 0 ? new_threshold : 0;
					rate_adjusting[qIndex] = true;
					need_set_tap[qIndex] = true;
				}
				else if(qlen <= qlen_min && perpg_inflight_threshold[qIndex] != maxThreshold){
					new_threshold = perpg_inflight_threshold[qIndex] + qlen_min - qlen;
					perpg_inflight_threshold[qIndex] = new_threshold > maxThreshold ? maxThreshold : new_threshold;
					rate_adjusting[qIndex] = true;
            		need_set_tap[qIndex] = true;
				}
			}
			#if OUTPUT_1
			std::cout << "Time " << Simulator::Now().GetNanoSeconds() 
					 << " threshold " << perpg_inflight_threshold[qIndex] 
					 << " node " << m_node->GetId()  
					 << " port " << GetIfIndex() 
					 << " qIndex " << qIndex
					 <<  std::endl;
				std::cout << "Time " << Simulator::Now().GetNanoSeconds() 
					 << " qlen " << qlen 
					 << " inflight " << perpq_accumu_used_credit[qIndex] - perpq_accumu_recycled_credit[qIndex]
					 << " node " << m_node->GetId()  
					 << " port " << GetIfIndex() 
					 << " qIndex " << qIndex
					 <<  std::endl;
			#endif
			DequeueAndTransmit(); 
		}
	}
	else {
		packet->PeekHeader(ch);
		if (m_node->GetNodeType() > 0) { // switch
			packet->AddPacketTag(InterfaceTag(m_ifIndex));
			m_node->SwitchReceiveFromDevice(this, packet, ch);	
		} else { // NIC
			uint32_t vl;
			FcpHeader fcph = FcpHeader();
			if (ch.l3Prot == 0xFC) { //ack packet
				vl = 1;
			} 
			else {
				vl = ch.udp.pg; 
			}
			
            bool tap = ((ch.m_tos & 0x03) == 0x03);
			perpq_accumu_drained_bytes[vl] += packet->GetSize();
			fcph.SetVl(vl);
			fcph.SetFccl(0x0fffffff);
			fcph.SetQlen(0);
			fcph.SetFccr(perpq_accumu_drained_bytes[vl]);
			Ptr<Packet> fcp = Create<Packet> (0);
			
			if(tap){
				fcph.SetTap();
			}
			else{
				fcph.ResetTap();
			}

			fcp->AddHeader(fcph);
			AddHeader(fcp, 0x0086);
			RdmaEnqueueFcpQ(fcp);
			DequeueAndTransmit();
			#if OUTPUT_1
			std::cout << "Time " << Simulator::Now().GetNanoSeconds() 
			<< " ingress_bytes 0"
			<< " node " << m_node->GetId()
			<< " port " << GetIfIndex() 
			<< " qIndex " << vl
			<< std::endl;
			#endif
			m_rdmaReceiveCb(packet, ch);
		}
	}
	return;
}

Address
QbbNetDevice::GetRemote (void) const
{
	NS_LOG_FUNCTION (this);
	NS_ASSERT (m_channel->GetNDevices () == 2);
	for (std::size_t i = 0; i < m_channel->GetNDevices (); ++i)
	{
		Ptr<NetDevice> tmp = m_channel->GetDevice (i);
		if (tmp != this)
		{
			return tmp->GetAddress ();
		}
	}
	NS_ASSERT (false);
	// quiet compiler.
	return Address ();
}

bool QbbNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
	NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
	NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
	NS_LOG_LOGIC ("UID is " << packet->GetUid ());

	if (IsLinkUp () == false)
	{
		m_macTxDropTrace (packet);
		return false;
	}
	AddHeader (packet, protocolNumber);
	m_macTxTrace (packet);
	m_queue->Enqueue (packet, 0);
	DequeueAndTransmit();
	return true;
}

bool QbbNetDevice::SwitchSend (uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch) {
	m_macTxTrace(packet);
	m_traceEnqueue(packet, qIndex);
	m_queue->Enqueue(packet, qIndex);
	DequeueAndTransmit();
	return true;
}

void QbbNetDevice::SendFcp(Ptr<Packet> p) {
	AddHeader(p, 0x0086);
	m_queue->Enqueue(p, 0);
	DequeueAndTransmit();
}

bool
QbbNetDevice::Attach(Ptr<QbbChannel> ch)
{
	NS_LOG_FUNCTION(this << &ch);
	m_channel = ch;
	m_channel->Attach(this);
	NotifyLinkUp();
	return true;
}

Ptr<Channel>
QbbNetDevice::GetChannel(void) const
{
	return m_channel;
}

bool QbbNetDevice::IsQbb(void) const {
	return true;
}

void QbbNetDevice::NewQp(Ptr<RdmaQueuePair> qp) {
	qp->m_nextAvail = Simulator::Now();
	DequeueAndTransmit();
}
void QbbNetDevice::ReassignedQp(Ptr<RdmaQueuePair> qp) {
	DequeueAndTransmit();
}
void QbbNetDevice::TriggerTransmit(void) {
	DequeueAndTransmit();
}

void QbbNetDevice::SetQueue(Ptr<BEgressQueue> q) {
	NS_LOG_FUNCTION(this << q);
	m_queue = q;
}

Ptr<BEgressQueue> QbbNetDevice::GetQueue() {
	return m_queue;
}

Ptr<RdmaEgressQueue> QbbNetDevice::GetRdmaQueue() {
	return m_rdmaEQ;
}

void QbbNetDevice::RdmaEnqueueAckQ(Ptr<Packet> p) {
	m_traceEnqueue(p, 1);
	m_rdmaEQ->EnqueueAckQ(p);
}

void QbbNetDevice::RdmaEnqueueFcpQ(Ptr<Packet> p) {
	m_traceEnqueue(p, 0);
	m_rdmaEQ->EnqueueFcpQ(p);
}

void QbbNetDevice::SetInflightThreshold(uint32_t qIndex, uint32_t threshold){
	perpg_inflight_threshold[qIndex] = threshold;
}

void QbbNetDevice::SetQlenMax(u_int32_t value){
	qlen_max = value;
}

void QbbNetDevice::SetQlenMin(u_int32_t value){
	qlen_min = value;
}

void QbbNetDevice::SetAiStep(u_int32_t value){
	ai_step = value;
}

void QbbNetDevice::UpdateNextAvail(Time t) {
	if (!m_nextSend.IsExpired() && t < Time(m_nextSend.GetTs())) {
		Simulator::Cancel(m_nextSend);
		Time delta = t < Simulator::Now() ? Time(0) : t - Simulator::Now();
		m_nextSend = Simulator::Schedule(delta, &QbbNetDevice::DequeueAndTransmit, this);
	}
}
} // namespace ns3
