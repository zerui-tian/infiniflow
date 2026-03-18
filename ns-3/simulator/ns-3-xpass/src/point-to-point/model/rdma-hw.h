#ifndef RDMA_HW_H
#define RDMA_HW_H

// #include <ns3/rdma.h>
#include <ns3/rdma-queue-pair.h>
#include <ns3/node.h>
#include <ns3/custom-header.h>
#include "qbb-net-device.h"
#include <unordered_map>
#include "pint.h"

namespace ns3 {

	struct RdmaInterfaceMgr {
		Ptr<QbbNetDevice> dev;
		Ptr<RdmaQueuePairGroup> qpGrp;

		RdmaInterfaceMgr() : dev(NULL), qpGrp(NULL) {}
		RdmaInterfaceMgr(Ptr<QbbNetDevice> _dev){
			dev = _dev;
		}
	};

	class RdmaHw : public Object {
	public:

		static TypeId GetTypeId (void);
		RdmaHw();

		Ptr<Node> m_node;
		uint32_t m_mtu;
		std::vector<RdmaInterfaceMgr> m_nic; // list of running nic controlled by this RdmaHw
		std::unordered_map<uint64_t, Ptr<RdmaTxWorkQueue> > m_qpMap; // mapping from uint64_t to qp
		std::unordered_map<uint64_t, Ptr<RdmaRxWorkQueue> > m_rxQpMap; // mapping from uint64_t to rx qp
		std::unordered_map<uint32_t, std::vector<int> > m_rtTable; // map from ip address (u32) to possible ECMP port (index of dev)

		std::unordered_map<uint32_t, uint32_t> m_lastArrivalTime; // record the last arrival time of each flow

		uint32_t m_totalFlowCount = 0;

		// qp complete callback
		typedef Callback<void, Ptr<RdmaTxWorkQueue> > QpCompleteCallback;
		QpCompleteCallback m_qpCompleteCallback;

		void SetNode(Ptr<Node> node);
		void Setup(QpCompleteCallback cb); // setup shared data and callbacks with the QbbNetDevice
		static uint64_t GetQpKey(uint32_t dip, uint16_t sport, uint16_t pg); // get the lookup key for m_qpMap
		Ptr<RdmaTxWorkQueue> GetQp(uint32_t dip, uint16_t sport, uint16_t pg); // get the qp
		uint32_t GetNicIdxOfQp(Ptr<RdmaTxWorkQueue> qp); // get the NIC index of the qp
		void AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish,Time stopTime); // add a new qp (new send)
		void DeleteQueuePair(Ptr<RdmaTxWorkQueue> qp);

		Ptr<RdmaRxWorkQueue> GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create); // get a rxQp
		uint32_t GetNicIdxOfRxQp(Ptr<RdmaRxWorkQueue> q); // get the NIC index of the rxQp
		void DeleteRxQp(uint32_t dip, uint16_t pg, uint16_t dport);
		// void ReceiveUdp(Ptr<Packet> p, CustomHeader &ch);
		// void ReceiveAck(Ptr<Packet> p, CustomHeader &ch); // handle both ACK and NACK
		void ReceiveCdt(Ptr<Packet> p, CustomHeader &ch); // handle request 、credit and data
		void recv_credit_request(Ptr<Packet> p, CustomHeader &ch);
		void recv_credit(Ptr<Packet> p, CustomHeader &ch);
		void recv_data(Ptr<Packet> p, CustomHeader &ch);
		void recv_ack(Ptr<Packet> p, CustomHeader &ch);
		void recv_fin1 (Ptr<Packet> p, CustomHeader &ch);
		void recv_fin2 (Ptr<Packet> p, CustomHeader &ch);
		void Receive(Ptr<Packet> p, CustomHeader &ch); // callback function that the QbbNetDevice should use when receive packets. Only NIC can call this function. And do not call this upon PFC

		bool ReceiverCheckSeq(uint32_t seq, Ptr<RdmaRxWorkQueue> q, uint32_t size);
		void AddHeader (Ptr<Packet> p, uint16_t protocolNumber);
		static uint16_t EtherToPpp (uint16_t protocol); // 将L3协议字段由Ethernet格式转化为PPP格式

		void QpComplete(Ptr<RdmaTxWorkQueue> qp);
		void SetLinkDown(Ptr<QbbNetDevice> dev);

		// call this function after the NIC is setup
		void AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx);
		void ClearTable();
		void RedistributeQp();

		Ptr<Packet> GetNxtPacket(Ptr<RdmaTxWorkQueue> qp); // get next packet to send, inc snd_nxt

		Ptr<Packet> GetNxtCreditPacket(Ptr<RdmaRxWorkQueue> qp); // get next packet to send, inc snd_nxt

		
		//控速逻辑在接收端，所以都是RxWorkQueue
		void PktSent(Ptr<RdmaRxWorkQueue> qp, Ptr<Packet> pkt, Time interframeGap); //
		void UpdateNextAvail(Ptr<RdmaRxWorkQueue> qp, Time interframeGap, uint32_t pkt_size);
	};

} /* namespace ns3 */

#endif /* RDMA_HW_H */
