/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
 * Author: Yuliang Li <yuliangli@g.harvard.com>
 * Modified (by Vamsi Addanki) to also serve TCP/IP traffic.
 */

#define __STDC_LIMIT_MACROS 1
#include "ns3/qbb-net-device.h"

#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/custom-header.h"
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/error-model.h"
#include "ns3/interface-tag.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/pause-header.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/pointer.h"
#include "ns3/ppp-header.h"
#include "ns3/qbb-channel.h"
#include "ns3/qbb-header.h"
#include "ns3/random-variable.h"
#include "ns3/rdma-tag.h"
#include "ns3/seq-ts-header.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include <ns3/switch-node.h>
#include <ns3/rdma-hw.h>
#include <ns3/rdma-driver.h>
#include "ns3/unsched-tag.h"

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

NS_LOG_COMPONENT_DEFINE("QbbNetDevice");

namespace ns3
{

// RdmaEgressQueue
TypeId
RdmaEgressQueue::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::RdmaEgressQueue")
            .SetParent<Object>()
            .AddTraceSource("RdmaEnqueue",
                            "Enqueue a packet in the RdmaEgressQueue.",
                            MakeTraceSourceAccessor(&RdmaEgressQueue::m_traceRdmaEnqueue),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RdmaDequeue",
                            "Dequeue a packet in the RdmaEgressQueue.",
                            MakeTraceSourceAccessor(&RdmaEgressQueue::m_traceRdmaDequeue),
                            "ns3::Packet::TracedCallback");
    return tid;
}

RdmaEgressQueue::RdmaEgressQueue()
{
    m_rrlast = 0;
    m_qlast = 0;
    m_ackQ = CreateObject<DropTailQueue<Packet>>();
    m_dataQ = CreateObject<DropTailQueue<Packet>>();
    // queue limit is on a higher level, not here
    m_ackQ->SetAttribute("MaxSize", QueueSizeValue(QueueSize(BYTES, 0xffffffff)));
    m_dataQ->SetAttribute("MaxSize", QueueSizeValue(QueueSize(BYTES, 0xffffffff)));
}

Ptr<Packet>
RdmaEgressQueue::DequeueAckQ()
{
    Ptr<Packet> retp = m_ackQ->Dequeue();
    m_qlast = QINDEX_ACK;
    m_traceRdmaDequeue(retp, QINDEX_ACK);
    return retp;
}

Ptr<Packet>
RdmaEgressQueue::DequeueDataQ()
{
    Ptr<Packet> retp = m_dataQ->Dequeue();
    m_qlast = QINDEX_DATA;
    m_traceRdmaDequeue(retp, QINDEX_DATA);
    return retp;
}

int
RdmaEgressQueue::GetLastQueue()
{
    return m_qlast;
}

uint32_t
RdmaEgressQueue::GetBytesLeftQindex(uint32_t qIndex)
{
    NS_ASSERT_MSG(qIndex < m_qpGrp->GetN(),
                  "RdmaEgressQueue::GetBytesLeftQindex: qIndex >= m_qpGrp->GetN()");
    return m_qpGrp->GetQp(qIndex)->GetBytesLeft();
}

Ptr<RdmaTxWorkQueue>
RdmaEgressQueue::GetQp(uint32_t i)
{
    return m_qpGrp->GetQp(i);
}

Ptr<RdmaRxWorkQueue>
RdmaEgressQueue::GetCreditQp(uint32_t i)
{
    return m_qpGrp->GetCreditQp(i);
}
/*
 * Function Description: 将分组推入ACK队列 for valve
 */
void
RdmaEgressQueue::EnqueueAckQ(Ptr<Packet> p)
{
    m_traceRdmaEnqueue(p, 0);
    m_ackQ->Enqueue(p);
}

/*
 * Function Description: 将分组推入DATA队列 for valve
 */
void
RdmaEgressQueue::EnqueueDataQ(Ptr<Packet> p)
{
    CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
    p->PeekHeader(ch);
    if(ch.ipv4Flags)
        printf("%d\n", ch.ipv4Flags);
    m_traceRdmaEnqueue(p, 0);
    m_dataQ->Enqueue(p);
}

void
RdmaEgressQueue::CleanAckQ(TracedCallback<Ptr<const Packet>, uint32_t> dropCb)
{
    while (m_ackQ->GetNPackets() > 0)
    {
        Ptr<Packet> p = m_ackQ->Dequeue();
        dropCb(p, 0);
    }
}

void
RdmaEgressQueue::CleanDataQ(TracedCallback<Ptr<const Packet>, uint32_t> dropCb)
{
    while (m_dataQ->GetNPackets() > 0)
    {
        Ptr<Packet> p = m_dataQ->Dequeue();
        dropCb(p, 0);
    }
}

/******************
 * QbbNetDevice
 *****************/
NS_OBJECT_ENSURE_REGISTERED(QbbNetDevice);

TypeId
QbbNetDevice::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::QbbNetDevice")
            .SetParent<PointToPointNetDevice>()
            .AddConstructor<QbbNetDevice>()
            .AddAttribute("QbbEnabled",
                          "Enable the generation of PAUSE packet.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&QbbNetDevice::m_qbbEnabled),
                          MakeBooleanChecker())
            .AddAttribute("PauseTime",
                          "Number of microseconds to pause upon congestion",
                          UintegerValue(5),
                          MakeUintegerAccessor(&QbbNetDevice::m_pausetime),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("TxBeQueue",
                          "A queue to use as the transmit queue in the device.",
                          PointerValue(),
                          MakePointerAccessor(&QbbNetDevice::m_queue),
                          MakePointerChecker<Queue<Packet>>())
            .AddAttribute("RdmaEgressQueue",
                          "A queue to use as the transmit queue in the device.",
                          PointerValue(),
                          MakePointerAccessor(&QbbNetDevice::m_rdmaEQ),
                          MakePointerChecker<Object>())
            .AddTraceSource("QbbEnqueue",
                            "Enqueue a packet in the QbbNetDevice.",
                            MakeTraceSourceAccessor(&QbbNetDevice::m_traceEnqueue),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("QbbDequeue",
                            "Dequeue a packet in the QbbNetDevice.",
                            MakeTraceSourceAccessor(&QbbNetDevice::m_traceDequeue),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("QbbDrop",
                            "Drop a packet in the QbbNetDevice.",
                            MakeTraceSourceAccessor(&QbbNetDevice::m_traceDrop),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RdmaQpDequeue",
                            "A qp dequeue a packet.",
                            MakeTraceSourceAccessor(&QbbNetDevice::m_traceQpDequeue),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RdmaCreditQpDequeue",
                            "A qp dequeue a credit packet.",
                            MakeTraceSourceAccessor(&QbbNetDevice::m_traceCreditQpDequeue),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("QbbPfc",
                            "get a PFC packet. 0: resume, 1: pause",
                            MakeTraceSourceAccessor(&QbbNetDevice::m_tracePfc),
                            "ns3::Packet::TracedCallback");

    return tid;
}

QbbNetDevice::QbbNetDevice()
{
    NS_LOG_FUNCTION(this);
    m_rdmaEQ = CreateObject<RdmaEgressQueue>();
    m_rdmaEQ->qb_dev = this;
    m_last_valid_ack_arrival_time = 0;
    m_last_valid_flag = 1;
    tokens_ = 0; //bytes

    m_creditTokenBucketClock = NanoSeconds(0);
    token_bucket_clock_ = NanoSeconds(0);
    credit_bytes = 0;
    
    for (uint32_t i = 0; i < qCnt; i++)
    {
        m_paused[i] = false;
    }
}

QbbNetDevice::~QbbNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
QbbNetDevice::SetQueue(Ptr<BEgressQueue> q)
{
    NS_LOG_FUNCTION(this << q);
    m_queue = q;
}

void
QbbNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);
    PointToPointNetDevice::DoDispose();
}

DataRate
QbbNetDevice::GetDataRate()
{
    return m_bps;
}

void
QbbNetDevice::SetMtu(uint32_t mtu){
	m_mtu = mtu;
	m_trans_time_mtu = double(8 * mtu) / (this->GetDataRate().GetBitRate() / 1e9); // 以当前线速传输一个MTU数据的时间
    return;
}

int 
RdmaEgressQueue::GetNextQindex( ) {
	uint32_t qIndex;

    if(m_ackQ->GetNPackets() > 0) {
        return -1;
    }
	// no pkt in highest priority queue, do rr for each qp
	int res = -1024;

    uint32_t fcount = m_qpGrp->GetCreditN();
    uint32_t min_fin_id = 0xffffffff;
    
    // std::cout << "fcount : " << fcount << std::endl;
    // uint32_t min_finish_id = 0xffffffff;
    for (qIndex = 0; qIndex < fcount; qIndex++) {
        uint32_t idx = (qIndex + m_rrlast) % fcount;
        Ptr<RdmaRxWorkQueue> qp = m_qpGrp->GetCreditQp(idx);
        if(!qp->isFin) {
            if (qp->c_nextAvail.GetTimeStep() > Simulator::Now().GetTimeStep()) //not available now
                continue;
            res = idx;
            break;
        }
        else min_fin_id = idx < min_fin_id ? idx : min_fin_id;
    }
        
    if (min_fin_id < 0xffffffff) {
        int nxt = min_fin_id;
        auto &qps = m_qpGrp->c_qps;
        for(int i = min_fin_id + 1; i < fcount; i++) if (!qps[i]->isFin) {
            if(i == res)
                res = nxt;
            qps[nxt] = qps[i];
            nxt++;
        }
        qps.resize(nxt);
    }

    if(res == -1024 && m_dataQ->GetNPackets() > 0)
        return -2;
    
    // std::cout << "res : " << res << std::endl;
	return res;
}

Ptr<Packet> 
RdmaEgressQueue::DequeueQindex(int q){
    Ptr<Packet> p;
    if(q == -1) {
        p = m_ackQ->Dequeue();
        m_qlast = QINDEX_ACK;
        m_traceRdmaDequeue(p, 0);
		UnSchedTag tag;
		bool found = p->PeekPacketTag(tag);
		uint32_t unsched = tag.GetValue();
    }
    else if(q == -2) {
        p = m_dataQ->Dequeue();
        m_qlast = QINDEX_DATA;
        m_traceRdmaDequeue(p, 0);
        UnSchedTag tag;
		bool found = p->PeekPacketTag(tag);
		uint32_t unsched = tag.GetValue();
    }
    else if(q >= 0) {
        // std::cout << "进入去拿credit包" << std::endl;
        p = m_rdmaGetNxtCreditPkt(m_qpGrp->GetCreditQp(q));
        m_rrlast = q;
		m_qlast = q;
        m_traceRdmaDequeue(p, m_qpGrp->GetCreditQp(q)->c_pg);
        UnSchedTag tag;
		bool found = p->PeekPacketTag(tag);
		uint32_t unsched = tag.GetValue();
    }
    
    return p;
}

bool
QbbNetDevice::TransmitStart(Ptr<Packet> p) // important!
{
    NS_LOG_FUNCTION(this << p);
    NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
    // std::cout << this->m_node->GetId() << "\tTX" << std::endl;
    //
    // This function is called to start the process of transmitting a packet.
    // We need to tell the channel that we've started wiggling the wire and
    // schedule an event that will be executed when the transmission is complete.
    //
    NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
    m_txMachineState = BUSY; // 修改NIC状态为BUSY
    m_currentPkt = p;
    m_phyTxBeginTrace(m_currentPkt);

    //下面1/3 by sgh
    //读取udp头部信息
    // CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	// p->PeekHeader(ch);
    //判断是发送方发送报文
    // if(ch.l3Prot == 0x11 && m_node->GetNodeType() == 0){
    //     //将内容输出到文件中
	//     std::string filename = "/home/pnic/valve-ns3/simulator/ns-3.39/examples/valve/result.txt";
	//     std::ofstream outFile;
	//     outFile.open(filename, std::ios::app); // 使用 std::ios::app 模式追加写入

	//     // 写入内容到文件
	//     outFile << " fid: " << ch.udp.dport << " send " << p->GetSize() << " at " << Simulator::Now().GetNanoSeconds() << " ns " << std::endl;
	//     //关闭文件
	//     outFile.close();
    // }
    
    Time txTime = m_bps.CalculateBytesTxTime(p->GetSize()); // 计算packet发送时间
    Time txCompleteTime =
        txTime +
        m_tInterframeGap; // 计算发送完成时间，其中tInterframeGap的意义是frame间的时间间隔，以掐断不同frame的传输

    NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
    Simulator::Schedule(txCompleteTime, &QbbNetDevice::TransmitComplete, this); // 设置传输完成事件

    bool result = m_channel->TransmitStart(p, this, txTime); // 从ppp链路发送
    if (result == false)
    {
        m_phyTxDropTrace(p);
    }
    //只测量其credit包
    CustomHeader header(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	p->PeekHeader(header);
    totalBytesSent += p->GetSize();
    if(header.cdt.mtype == 0x02) 
        totalCreditBytes += p->GetSize();
    else if(header.cdt.mtype == 0x03) {
        totalDataBytes += p->GetSize();
    }
    return result;
}

void
QbbNetDevice::TransmitComplete(void) // important!
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
    m_txMachineState = READY; // 修改NIC状态为READY
    NS_ASSERT_MSG(m_currentPkt != 0, "QbbNetDevice::TransmitComplete(): m_currentPkt zero");
    m_phyTxEndTrace(m_currentPkt);
    m_currentPkt = 0;
    DequeueAndTransmit(); // 完成后立马尝试从queue中提取下一个packet发送
}

void
QbbNetDevice::DequeueAndTransmit(void) // important!
{
    // CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
    // std::cout << "dequeue " << std::endl;
    NS_LOG_FUNCTION(this);
    if (!m_linkUp)
        return; // if link is down, return
    if (m_txMachineState == BUSY)
        return; // Quit if channel busy

    Ptr<Packet> p;
    int qIndex;

    /* host */
    if (m_node->GetNodeType() == 0) {
        qIndex = m_rdmaEQ->GetNextQindex();
        if(qIndex != -1024) {
            if(qIndex >= 0 ){
                UpdateCreditTokenBucket();
                p = m_rdmaEQ->DequeueQindex(qIndex);
                if (m_creditTokens >= p->GetSize() ) {
                    m_creditTokens -= p->GetSize();
                    TransmitStart(p);
                } 
                else { 
                    // 不足，挂起并等待下次发送
                    // if (m_nextSend.IsExpired()) {
                    //     Time waitTime = NanoSeconds((double)(credit_size- m_creditTokens) * 8 / m_creditRateLimiter);
                    //     std::cout << "waitTime :" << waitTime << std::endl;
                    //     m_nextSend = Simulator::Schedule(waitTime, &QbbNetDevice::DequeueAndTransmit, this);
                    // }
                }
                Ptr<RdmaRxWorkQueue> lastQp = m_rdmaEQ->GetCreditQp(qIndex);
                m_traceCreditQpDequeue(p,lastQp);
                m_rdmaPktSent(lastQp, p, m_tInterframeGap);
            }
            else {
                p = m_rdmaEQ->DequeueQindex(qIndex);
                TransmitStart(p);
            }     
            //未加入token_bucket的速率调控
            // p = m_rdmaEQ->DequeueQindex(qIndex);
            // if(qIndex >= 0 ){
            //     Ptr<RdmaRxWorkQueue> lastQp = m_rdmaEQ->GetCreditQp(qIndex);
            //     std::cout << "qIndex: " << qIndex << std::endl;
            //     TransmitStart(p);
            //     m_traceCreditQpDequeue(p,lastQp);
            //     m_rdmaPktSent(lastQp, p, m_tInterframeGap);
            // }
            // else 
            //     TransmitStart(p);
        }
        else {
            // if (m_rdmaEQ->m_ackQ->GetNPackets() > 0) {
            //     p = m_rdmaEQ->DequeueAckQ();
                // p->RemoveHeader(ch);
                // uint32_t flowCount = this->GetNode()->GetObject<RdmaDriver>()->m_rdma->m_totalFlowCount;
                // uint32_t rand_num = u_int32_t( double(rand())/double(RAND_MAX) * 1000); //生成分布在[0,1000)的随机整数，均匀分布
                // u_int32_t prob;
                // u_int32_t qlen = m_rdmaEQ->m_ackQ->GetNPackets(); //同线卡上，egress的队列长度，单位Bytes
                // double backlog_ack = (double)qlen / (double)(p->GetSize()); //将qlen换算为ACK数量
                // bool newbie;
                // if(flowCount != 0){
                //     u_int32_t bdp = ch.ack.rtt * this->m_channel->GetDelay().GetNanoSeconds() * this->GetDataRate().GetBitRate() / 1e9 / 8; //所属流的base RTT，单位Bytes
                //     u_int32_t wnd_t = bdp / flowCount; //窗口大小的理想值
                //     u_int32_t threshold = m_alpha * wnd_t; //新手保护期阈值
                //     u_int32_t prob_t = backlog_ack * m_kp_nic; //根据qlen计算的概率值
                //     if(ch.ack.win_size <= threshold){
                //         newbie = true;
                //     }
                //     else{
                //         prob = (ch.ack.win_size - threshold) * prob_t / (wnd_t - threshold);
                //     }
                // }
                // else{
                //     prob = 0;
                // }
                // if(qlen == 0 || newbie){
                //     ch.ack.flags &= (~(0x03));
                //     ch.ack.flags += VALVE_FLAG_INC;//打标记
                //     m_shaping_pkt_num = 2;
                // }
                // else if (rand_num <= prob){
                //     ch.ack.flags &= (~(0x03));
                //     ch.ack.flags += VALVE_FLAG_ABE;//打标记
                //     m_shaping_pkt_num = 0;
                // }
                // else{
                //     ch.ack.flags &= (~(0x03));
                //     ch.ack.flags += VALVE_FLAG_REG;//打标记
                //     m_shaping_pkt_num = 1;
                // }
                // p->AddHeader(ch);
                // m_traceDequeue(p, 0);
                // TransmitStart(p);
                //速率调控xpass by sgh
                // Ptr<RdmaRxWorkQueue> lastQp = m_rdmaEQ->GetCreditQp(ACK_Q_IDX);
                // m_rdmaPktSent(lastQp, p, m_tInterframeGap);
            // }
            // else if (m_rdmaEQ->m_dataQ->GetNPackets() > 0) {
            //     p = m_rdmaEQ->DequeueDataQ();
            //     m_traceDequeue(p, 0);
            //     TransmitStart(p);
            // }

            //no packet to send
            // else {
            Time t = Simulator::GetMaximumSimulationTime();
            for (uint32_t i = 0; i < m_rdmaEQ->m_qpGrp->GetCreditN(); i++) {
                Ptr<RdmaRxWorkQueue> qp = m_rdmaEQ->m_qpGrp->GetCreditQp(i);
                //如果有多个流，就一个一个发
                t = Min(qp->c_nextAvail, t);

            }
            // std::cout<<"下一个数据包的发送时间："<<t<<std::endl;
            if (m_nextSend.IsExpired() && t < Simulator::GetMaximumSimulationTime() && t > Simulator::Now()) {
                m_nextSend = Simulator::Schedule(t - Simulator::Now(), &QbbNetDevice::DequeueAndTransmit, this);
            }
            // }
        }
    }
    /* switch */
    else
    {
        p = m_queue->DequeueRR(m_paused); // 从队列中取出一个分组
        if (p != NULL) {
            m_snifferTrace(p);
            m_promiscSnifferTrace(p);
            InterfaceTag t;

            CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
            p->PeekHeader(ch);

            uint32_t qIndex = m_queue->GetLastQueue();

            if(ch.l3Prot != 0xFE){
                m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p); // 执行egress逻辑
                p->RemovePacketTag(t);                             // 剥离interface tag
            }
            else{
            }

            m_traceDequeue(p, qIndex);
            TransmitStart(p); // 发送分组
        }
    }
    return;
}

bool
QbbNetDevice::IsQbb(void) const
{
    return true;
}

void
QbbNetDevice::Resume(unsigned qIndex)
{
    NS_LOG_FUNCTION(this << qIndex);
    NS_ASSERT_MSG(m_paused[qIndex], "Must be PAUSED");
    m_paused[qIndex] = false;
    NS_LOG_INFO("Node " << m_node->GetId() << " dev " << m_ifIndex << " queue " << qIndex
                        << " resumed at " << Simulator::Now().GetSeconds());
    DequeueAndTransmit();
}

void
QbbNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    m_rxCallback = cb;
}

bool
QbbNetDevice::ProcessHeader(Ptr<Packet> p, uint16_t& param)
{
    NS_LOG_FUNCTION(this << p << param);
    PppHeader ppp;
    p->RemoveHeader(ppp);                  // 剥离PPP帧头
    param = PppToEther(ppp.GetProtocol()); // 将协议字段转化为Ethernet格式输出
    return true;
}

uint16_t
QbbNetDevice::PppToEther(uint16_t proto)
{
    NS_LOG_FUNCTION_NOARGS();
    switch (proto)
    {
    case 0x0021:
        return 0x0800; // IPv4
    case 0x0057:
        return 0x86DD; // IPv6
    default:
        NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
        std::cout << "PPP Protocol number not defined!" << std::endl;
    }
    return 0;
}

uint16_t
QbbNetDevice::EtherToPpp(uint16_t proto)
{
    NS_LOG_FUNCTION_NOARGS();
    switch (proto)
    {
    case 0x0800:
        return 0x0021; // IPv4
    case 0x86DD:
        return 0x0057; // IPv6
    default:
        NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
    }
    return 0;
}

void
QbbNetDevice::Receive(Ptr<Packet> packet) // important!
{
    NS_LOG_FUNCTION(this << packet);
    // std::cout << this->m_node->GetId() << "\tRX" << std::endl;

    if (!m_linkUp)
    {
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

    totalBytesRcvd += packet->GetSize();

    CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
    packet->PeekHeader(ch);
    if (ch.l3Prot == 0xFE)
    { // PFC帧
        // if (!m_qbbEnabled)
        //     return;
        // unsigned qIndex = ch.pfc.qIndex;
        // if (ch.pfc.time > 0)
        // { // PAUSE帧
        //     m_tracePfc(1);
        //     m_paused[qIndex] = true;
        // }
        // else
        // { // RESUME帧
        //     m_tracePfc(0);
        //     Resume(qIndex);
        // }
        // std::cout << "node is " << m_node->GetId() << " " << (ch.pfc.time != 0 ? "PAUSED!" : "RESUMED!") << std::endl;
    }
    else
    { // non-PFC packets (data, ACK...)
        if (m_node->GetNodeType() != 0)
        { // switch
            Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(m_node);
            packet->AddPacketTag(InterfaceTag(m_ifIndex)); // 添加Interface Tag

            sw->SendToDev(packet, ch);
            // if (ch.cdt.mtype == 0x02)
            //     credit_bytes -= packet->GetSize();
            // m_node->SwitchReceiveFromDevice(this, packet, ch); //执行ingress逻辑
        }
        else
        {                                // NIC
            m_rdmaReceiveCb(packet, ch); // RdmaHw::Receive(packet, ch)
        }
    }
    return;
}

Address
QbbNetDevice::GetRemoteAddress(void) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_channel->GetNDevices() == 2);
    for (std::size_t i = 0; i < m_channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> tmp = m_channel->GetDevice(i);
        if (tmp != this)
        {
            return tmp->GetAddress();
        }
    }
    NS_ASSERT(false);
    // quiet compiler.
    return Address();
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

void
QbbNetDevice::SendPfc(uint32_t qIndex, uint32_t type)
{
    Ptr<Packet> p = Create<Packet>();
    PauseHeader pauseh((type == 0 ? m_pausetime : 0), m_queue->GetNBytes(qIndex), qIndex);
    p->AddHeader(pauseh);
    Ipv4Header ipv4h; // Prepare IPv4 header
    ipv4h.SetProtocol(0xFE);
    ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
    ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
    ipv4h.SetPayloadSize(p->GetSize());
    ipv4h.SetTtl(1);
    ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
    p->AddHeader(ipv4h);
    AddHeader(p, 0x800);
    // CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
    // p->PeekHeader(ch);
    m_tracePfc(type + 2); // 2 indicates PFC PAUSE sent.3 indicates RESUME sent

    m_macTxTrace(p);
    m_traceEnqueue(p, qIndex);
    m_queue->Enqueue(p, qIndex);
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

Ptr<BEgressQueue>
QbbNetDevice::GetQueue(void)
{
    return m_queue;
}

void
QbbNetDevice::TakeDown()
{
    // TODO: delete packets in the queue, set link down
    if (m_node->GetNodeType() == 0)
    {
        // clean the high prio queue
        m_rdmaEQ->CleanAckQ(m_traceDrop);
        m_rdmaEQ->CleanDataQ(m_traceDrop);
        // notify driver/RdmaHw that this link is down
        m_rdmaLinkDownCb(this);
    }
    else
    { // switch
        // clean the queue
        for (uint32_t i = 0; i < qCnt; i++)
            m_paused[i] = false;
        while (1)
        {
            Ptr<Packet> p = m_queue->DequeueRR(m_paused);
            if (p == NULL)
                break;
            m_traceDrop(p, m_queue->GetLastQueue());
        }
        // TODO: Notify switch that this link is down
    }
    m_linkUp = false;
}

void
QbbNetDevice::updateTokenBucket(){
    Time now = Simulator::Now();
    // std::cout << "now time : " << now << std::endl;
    if(now <= token_bucket_clock_) {
        return;
    }
    Time elapsed_time = now - token_bucket_clock_;
    int new_tokens = (int)(elapsed_time.GetSeconds() * token_refresh_rate_ / 8) ; //Byte
    
    tokens_ += new_tokens;
    tokens_ = tokens_ > max_tokens_ ? max_tokens_ : tokens_;
    // std::cout << "tokens : " << tokens_ << std::endl;
    token_bucket_clock_ += NanoSeconds(new_tokens * 1e9 * 8/ token_refresh_rate_ );
    // std::cout << "clock : " << token_bucket_clock_ << std::endl;
    
}

void
QbbNetDevice::UpdateCreditTokenBucket()
{
    Time now = Simulator::Now();
    if (now <= m_creditTokenBucketClock)
        return;

    Time elapsed = now - m_creditTokenBucketClock;
    uint64_t newTokens = (uint64_t)(elapsed.GetSeconds() * m_creditRateLimiter / 8);
    // std::cout << "newTokens: " << newTokens <<  std::endl;
    m_creditTokens = std::min(m_creditTokens + newTokens, m_creditMaxTokens);
    // std::cout << "M_Tokens: " << m_creditTokens <<  std::endl;
    m_creditTokenBucketClock = now;
}


} // namespace ns3
