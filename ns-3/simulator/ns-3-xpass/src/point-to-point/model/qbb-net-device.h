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
 * Author: Yibo Zhu <yibzh@microsoft.com>
 */
#ifndef QBB_NET_DEVICE_H
#define QBB_NET_DEVICE_H

#include "ns3/point-to-point-net-device.h"
#include "ns3/qbb-channel.h"
// #include "ns3/fivetuple.h"
#include "ns3/broadcom-egress-queue.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
#include "ns3/rdma-queue-pair.h"
#include "ns3/udp-header.h"

#include <map>
#include <vector>

// #include <ns3/rdma.h>
// #define ENABLE_QP 1

//其他queue pair index >= 0
#define QINDEX_ACK -1
#define QINDEX_DATA -2

#define ACK_Q_IDX 0
#define CREDIT_Q_IDX 1
#define DATA_Q_IDX 2


namespace ns3
{

class RdmaEgressQueue : public Object
{
  public:
    Ptr<QbbNetDevice> qb_dev; // 指向承载Egress Queue的NIC
    static TypeId GetTypeId(void);
    RdmaEgressQueue();
    TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaEnqueue;
    TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaDequeue;

    /* for host NIC */
    // NIC发送队列
    Ptr<DropTailQueue<Packet>> m_ackQ;
    Ptr<DropTailQueue<Packet>> m_dataQ;
    void EnqueueAckQ(Ptr<Packet> p);
    Ptr<Packet> DequeueAckQ(void);
    void CleanAckQ(TracedCallback<Ptr<const Packet>, uint32_t> dropCb);
    void EnqueueDataQ(Ptr<Packet> p);
    Ptr<Packet> DequeueDataQ(void);
    void CleanDataQ(TracedCallback<Ptr<const Packet>, uint32_t> dropCb);
    
    int GetNextQindex();
    Ptr<Packet> DequeueQindex(int q);
    Ptr<Packet> DequeueCredit(uint32_t q);
    // QP及其轮询变量
    int m_qlast;
    uint32_t m_rrlast;               // Round Robin指针

    //xpass by sgh
    int c_qlast;
    uint32_t c_rrlast;

    Ptr<RdmaQueuePairGroup> m_qpGrp; // QP group,include tx and rx group(xpass)
    Callback<Ptr<Packet>, Ptr<RdmaTxWorkQueue>> m_rdmaGetNxtPkt; // callback for get next packet
    Callback<Ptr<Packet>, Ptr<RdmaRxWorkQueue>> m_rdmaGetNxtCreditPkt;  // callback for get next credit packet

    int GetLastQueue();
    int GetLastCreditQueue() {
      return c_qlast;
    };
    Ptr<RdmaTxWorkQueue> GetQp(uint32_t i);

    Ptr<RdmaRxWorkQueue> GetCreditQp(uint32_t i);
    uint32_t GetBytesLeftQindex(uint32_t qIndex);
};

/**
 * \class QbbNetDevice
 * \brief A Device for a IEEE 802.1Qbb Network Link.
 */
class QbbNetDevice : public PointToPointNetDevice
{

  public:
    static TypeId GetTypeId(void);

    QbbNetDevice();
    virtual ~QbbNetDevice();

   

    /* for switch Line Card */

    static const uint32_t qCnt = 8; // Number of queues/priorities used

    /* byte counters */
    uint64_t numTxBytes = 0;
    uint64_t numTxBytesLast = 0;
    uint64_t totalBytesSent = 0;

    //控制token 5/18 by sgh
    // Maximum size of credit queue (in bytes)
    int credit_q_limit_;
    // 当前credit已有的
    int credit_bytes;
    uint16_t credit_size = 68;
    // Maximum **size of data queue** (in bytes)
    uint64_t data_q_limit_;

    // Token Bucket Related Varaibles
    // Number of tokens remaning (in bytes)
    uint64_t tokens_;
    // Maximum number of tokens (in bytes)
    uint64_t max_tokens_;
    Time token_bucket_clock_;//（相当于上次更新tokens的时间）
    // Token Refresh Rate (in bytes per sec)
    // == Credit Throttling Rate
    uint64_t token_refresh_rate_;

     // for host NIC
    // 速率限制相关
    uint64_t m_creditRateLimiter; // 限制 credit 流总速率
    uint64_t m_creditMaxTokens;
    uint64_t m_creditTokens;
    Time m_creditTokenBucketClock;
    
    //rxworkqueue init
    // initial value of w_
    double w_init_; //设置为0.5
    // minimum value of w_
    double min_w_; //设置为0.01，最大max_w_是0.5
    // whether feedback control can increase w or not.
    //We vary the jitter level, j, from 0.01 to 0.08 relative to the inter-credit gap
    // maximum jitter: -1.0 ~ 1.0 (wrt. inter-credit gap)
    double max_jitter_;
    // minimum jitter: -1.0 ~ 1.0 (wrt. inter-credit gap)
    double min_jitter_;
    DataRate max_credit_rate;
    double target_loss_scaling_;
    double alpha_; 
    //调速参数，确定其初始的速率为最大速率的多少，应小于等于1
    uint64_t getNumTxBytes() {
        uint64_t temp;
        temp = totalBytesSent - numTxBytesLast;
        numTxBytesLast = totalBytesSent;
        return temp;
    }

    uint64_t numRxBytes = 0;
    uint64_t numRxBytesLast = 0;
    uint64_t totalBytesRcvd = 0;
    uint64_t totalCreditBytes = 0; // total credit bytes sent
    uint64_t totalDataBytes = 0; // total data bytes sent
    uint64_t totalCreditBytesLast = 0; // last credit bytes sent
    uint64_t totalDataBytesLast = 0; // last data bytes sent

    uint64_t getNumRxBytes() {
        uint64_t temp;
        temp = totalBytesRcvd - numRxBytesLast;
        numRxBytesLast = totalBytesRcvd;
        return temp;
    }
    
    uint64_t getNumCreditBytes() {
      uint64_t temp;
      temp = totalCreditBytes - totalCreditBytesLast;
      totalCreditBytesLast = totalCreditBytes;
      return temp;
    }

    uint64_t getNumDataBytes() {
      uint64_t temp;
      temp = totalDataBytes - totalDataBytesLast;
      totalDataBytesLast = totalDataBytes;
      return temp;
    }
    uint32_t m_mtu;
    double m_trans_time_mtu;
    void SetMtu(uint32_t mtu);

    /**
     * The queues for each priority class in SWITCH.
     * @see class Queue
     * @see class InfiniteQueue
     */

    Ptr<Node> m_node;

    Ptr<BEgressQueue> m_queue;

    Ptr<QbbChannel> m_channel;

    Ptr<RdmaEgressQueue> m_rdmaEQ;

    EventId m_nextSend; // The next send event

    uint8_t m_last_valid_flag;
    uint64_t m_last_valid_ack_arrival_time; // 最近有data分组到达device的时间
    
    double m_alpha = 0.25;
    double m_kp_nic = 50;
    double m_kp_lc_i = 50;
    double m_kp_lc_e = 50;

    // double m_backlog_last = 0;
    // u_int64_t m_dec_time_last = 0;


    /* PFC */
    bool m_qbbEnabled;                            // PFC behaviour enabled
    uint32_t m_pausetime;                         // Time for each Pause
    bool m_paused[qCnt];                          // Whether a queue paused
    void SendPfc(uint32_t qIndex, uint32_t type); // type: 0 = pause, 1 = resume
    /// Resume a paused queue and call DequeueAndTransmit()
    virtual void Resume(unsigned qIndex);

    //xpass
    TracedCallback<Ptr<const Packet>, Ptr<RdmaRxWorkQueue>>
        m_traceCreditQpDequeue; // the trace for printing Credit dequeue
    TracedCallback<Ptr<const Packet>, Ptr<RdmaTxWorkQueue>>
        m_traceQpDequeue; // the trace for printing dequeue
    TracedCallback<Ptr<const Packet>, uint32_t> m_traceEnqueue;
    TracedCallback<Ptr<const Packet>, uint32_t> m_traceDequeue;
    TracedCallback<Ptr<const Packet>, uint32_t> m_traceDrop;
    TracedCallback<uint32_t> m_tracePfc; // 0: resume, 1: pause

    // callback for processing packet in RDMA
    Callback<void, Ptr<Packet>, CustomHeader&> m_rdmaReceiveCb;
    // callback for link down
    Callback<void, Ptr<QbbNetDevice>> m_rdmaLinkDownCb;
    // callback for sent a packet
    Callback<void, Ptr<RdmaRxWorkQueue>, Ptr<Packet>, Time> m_rdmaPktSent; //改为RxWorkQueue

    /**
     * Receive a packet from a connected PointToPointChannel.
     *
     * This is to intercept the same call from the PointToPointNetDevice
     * so that the pause messages are honoured without letting
     * PointToPointNetDevice::Receive(p) know
     *
     * @see PointToPointNetDevice
     * @param p Ptr to the received packet.
     */
    virtual void Receive(Ptr<Packet> p);

    bool IsQbb(void) const;

    // 获得同链路上另块线卡或NIC的地址
    Address GetRemoteAddress(void) const;
    Ptr<NetDevice> GetRemoteDevice(void) const;

    virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb);

    DataRate GetDataRate();

    Ptr<BEgressQueue> GetQueue();
   void SetQueue (Ptr<BEgressQueue> q);

    bool Attach(Ptr<QbbChannel> ch);
    virtual Ptr<Channel> GetChannel(void) const;

    bool TransmitStart(Ptr<Packet> p);
    /// Reset the channel into READY state and try transmit again
    virtual void TransmitComplete(void);
    /// Look for an available packet and send it using TransmitStart(p)
    virtual void DequeueAndTransmit(void);

    virtual void DoDispose(void);

    bool ProcessHeader(Ptr<Packet> p, uint16_t& param);

    static uint16_t PppToEther(uint16_t proto);
    static uint16_t EtherToPpp(uint16_t proto);

    void TakeDown(); // take down this device

    void updateTokenBucket();

    void UpdateCreditTokenBucket();

};  

} // namespace ns3

#endif   // QBB_NET_DEVICE_H
