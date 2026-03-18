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
//#include "ns3/fivetuple.h"
#include "ns3/event-id.h"
#include "ns3/broadcom-egress-queue.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/rdma-queue-pair.h"
#include <vector>
#include<map>
#include "global-config.h"
// #include <ns3/rdma.h>
// #define ENABLE_QP 1

namespace ns3 {

class RdmaEgressQueue : public Object{
public:
  static const uint32_t qCnt = Q_CNT;
	int m_qlast;
	uint32_t m_rrlast;
	Ptr<DropTailQueue<Packet>> m_ackQ;
  Ptr<DropTailQueue<Packet>> m_fcpQ;
	Ptr<RdmaQueuePairGroup> m_qpGrp; // queue pairs

  uint32_t m_mtu;

	// callback for get next packet
	typedef Callback<Ptr<Packet>, Ptr<RdmaQueuePair> > RdmaGetNxtPkt;
	RdmaGetNxtPkt m_rdmaGetNxtPkt;

	static TypeId GetTypeId (void);
	RdmaEgressQueue();
	Ptr<Packet> DequeueQindex(int qIndex);
	int GetNextQindex(uint32_t credits[]);
	int GetLastQueue();
	uint32_t GetNBytes(uint32_t qIndex);
	uint32_t GetFlowCount(void);
	Ptr<RdmaQueuePair> GetQp(uint32_t i);
	void RecoverQueue(uint32_t i);
	void EnqueueAckQ(Ptr<Packet> p);
	void EnqueueFcpQ(Ptr<Packet> p);

	TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaEnqueue;
	TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaDequeue;

	Ptr<QbbNetDevice> qb_dev;
};

/**
 * \class QbbNetDevice
 * \brief A Device for a IEEE 802.1Qbb Network Link.
 */
class QbbNetDevice : public PointToPointNetDevice 
{
public:
  static const uint32_t qCnt = Q_CNT;// Number of queues/priorities used
  std::unordered_map<uint32_t, uint64_t> m_txBytesPerFlow;
  static TypeId GetTypeId (void);

  QbbNetDevice ();
  virtual ~QbbNetDevice ();

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
  virtual void Receive (Ptr<Packet> p);

  /**
   * Send a packet to the channel by putting it to the queue
   * of the corresponding priority class
   *
   * @param packet Ptr to the packet to send
   * @param dest Unused
   * @param protocolNumber Protocol used in packet
   */
  virtual bool Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SwitchSend (uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch);

  Address GetRemote (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  // 获得同链路上另块线卡或NIC的地址
  Ptr<NetDevice> GetRemoteDevice(void) const;

  DataRate GetDataRate();

  /**
   * Get the size of Tx buffer available in the device
   *
   * @return buffer available in bytes
   */
  //virtual uint32_t GetTxAvailable(unsigned) const;

  /**
   * TracedCallback hooks
   */
  void ConnectWithoutContext(const CallbackBase& callback);
  void DisconnectWithoutContext(const CallbackBase& callback);

  bool Attach (Ptr<QbbChannel> ch);

   virtual Ptr<Channel> GetChannel (void) const;

   void SetQueue (Ptr<BEgressQueue> q);
   Ptr<BEgressQueue> GetQueue ();

   void SetQueueFifo (Ptr<DropTailQueue<Packet>> q);
   Ptr<DropTailQueue<Packet>> GetQueueFifo ();

   virtual bool IsQbb(void) const;
   void NewQp(Ptr<RdmaQueuePair> qp);
   void ReassignedQp(Ptr<RdmaQueuePair> qp);
   void TriggerTransmit(void);

	// void SendPfc(uint32_t qIndex, uint32_t type); // type: 0 = pause, 1 = resume

  void SendFcp(Ptr<Packet> p);
	TracedCallback<Ptr<const Packet>, uint32_t> m_traceEnqueue;
	TracedCallback<Ptr<const Packet>, uint32_t> m_traceDequeue;
	TracedCallback<Ptr<const Packet>, uint32_t> m_traceDrop;
	TracedCallback<uint32_t> m_tracePfc; // 0: resume, 1: pause

	uint64_t numTxBytesLast=0;
	uint64_t totalBytesSent=0;

	uint64_t getNumTxBytes(){
		uint64_t temp;
		temp = totalBytesSent-numTxBytesLast;
		numTxBytesLast=totalBytesSent;
		return temp;
	}

	uint64_t numRxBytes=0;
	uint64_t numRxBytesLast=0;
	uint64_t totalBytesRcvd=0;
	
	uint64_t getNumRxBytes(){
		uint64_t temp;
		temp = totalBytesRcvd-numRxBytesLast;
		numRxBytesLast=totalBytesRcvd;
		return temp;
	}

  uint64_t getNumTxBytesByFlow(uint32_t flow_id) {
    auto it = m_txBytesPerFlow.find(flow_id);
    if (it != m_txBytesPerFlow.end()) return it->second;
    return 0;
  }
    //cbfc
  uint32_t perpq_fctbs[qCnt]; //per priority queue每个队列发送的
  uint32_t perpq_fccl[qCnt];  //per priority queue每个队列接收的下游fccl值

  uint32_t target_fid = 10000;
  uint32_t target_swid = 4;
protected:

	//Ptr<Node> m_node;

  bool TransmitStart (Ptr<Packet> p);
  
  virtual void DoDispose(void);

  /// Reset the channel into READY state and try transmit again
  virtual void TransmitComplete(void);

  /// Look for an available packet and send it using TransmitStart(p)
  virtual void DequeueAndTransmit(void);

  /**
   * The queues for each priority class.
   * @see class Queue
   * @see class InfiniteQueue
   */
  Ptr<BEgressQueue> m_queue;

  Ptr<QbbChannel> m_channel;
  
  //pfc
  bool m_qbbEnabled;	//< PFC behaviour enabled
  bool m_qcnEnabled;
  
  //qcn

  /* RP parameters */
  EventId  m_nextSend;		//< The next send event
  /* State variable for rate-limited queues */

  //qcn

  struct ECNAccount{
	  Ipv4Address source;
	  uint32_t qIndex;
	  uint32_t port;
	  uint8_t ecnbits;
	  uint16_t qfb;
	  uint16_t total;
  };

  std::vector<ECNAccount> *m_ecn_source;

public:
	Ptr<RdmaEgressQueue> m_rdmaEQ;
	void RdmaEnqueueAckQ(Ptr<Packet> p);
	void RdmaEnqueueFcpQ(Ptr<Packet> p);

	// callback for processing packet in RDMA
	typedef Callback<int, Ptr<Packet>, CustomHeader&> RdmaReceiveCb;
	RdmaReceiveCb m_rdmaReceiveCb;
	// callback for link down
	typedef Callback<void, Ptr<QbbNetDevice> > RdmaLinkDownCb;
	RdmaLinkDownCb m_rdmaLinkDownCb;
	// callback for sent a packet
	typedef Callback<void, Ptr<RdmaQueuePair>, Ptr<Packet>, Time> RdmaPktSent;
	RdmaPktSent m_rdmaPktSent;

	Ptr<RdmaEgressQueue> GetRdmaQueue();
	// void TakeDown(); // take down this device
	void UpdateNextAvail(Time t);

	TracedCallback<Ptr<const Packet>, Ptr<RdmaQueuePair> > m_traceQpDequeue; // the trace for printing dequeue

	uint64_t hostDequeueIndex;
};

} // namespace ns3

#endif // QBB_NET_DEVICE_H
