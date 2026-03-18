#include <ns3/simulator.h>
#include <ns3/xpass-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include "ns3/ppp-header.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/pointer.h"
#include "rdma-hw.h"
#include "ppp-header.h"
#include "qbb-header.h"
#include "ns3/point-to-point-channel.h"
#include <iostream>

namespace ns3 {

	TypeId RdmaHw::GetTypeId (void)	{
		static TypeId tid = TypeId ("ns3::RdmaHw")
							.SetParent<Object> ()
							.AddAttribute("Mtu",
										"Mtu.",
										UintegerValue(1500),
										MakeUintegerAccessor(&RdmaHw::m_mtu),
										MakeUintegerChecker<uint32_t>());
		return tid;
	}
	/*
	* Function Description: RdmaHw构造函数
	*/
	RdmaHw::RdmaHw() {
	}

	void RdmaHw::SetNode(Ptr<Node> node) {
		m_node = node;
	}

	void RdmaHw::Setup(QpCompleteCallback cb) {
		for (uint32_t i = 0; i < m_nic.size(); i++) {
			Ptr<QbbNetDevice> dev = m_nic[i].dev;
			if (dev == NULL)
				continue;
			// share data with NIC
			dev->m_rdmaEQ->m_qpGrp = m_nic[i].qpGrp;
			// setup callback
			dev->m_rdmaReceiveCb = MakeCallback(&RdmaHw::Receive, this);
			dev->m_rdmaLinkDownCb = MakeCallback(&RdmaHw::SetLinkDown, this);
			dev->m_rdmaPktSent = MakeCallback(&RdmaHw::PktSent, this);
			// config NIC
			dev->m_rdmaEQ->m_rdmaGetNxtPkt = MakeCallback(&RdmaHw::GetNxtPacket, this);
			dev->m_rdmaEQ->m_rdmaGetNxtCreditPkt = MakeCallback(&RdmaHw::GetNxtCreditPacket, this);
		}
		// setup qp complete callback
		m_qpCompleteCallback = cb;
	}

	uint32_t RdmaHw::GetNicIdxOfQp(Ptr<RdmaTxWorkQueue> qp) {
		auto &v = m_rtTable[qp->dip.Get()];
		if (v.size() > 0) {
			return v[qp->GetHash() % v.size()];
		} else {
			NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
		}
	}

	uint64_t RdmaHw::GetQpKey(uint32_t dip, uint16_t sport, uint16_t pg) {
		return ((uint64_t)dip << 32) | ((uint64_t)sport << 16) | (uint64_t)pg;
	}

	Ptr<RdmaTxWorkQueue> RdmaHw::GetQp(uint32_t dip, uint16_t sport, uint16_t pg) {
		uint64_t key = GetQpKey(dip, sport, pg);
		auto it = m_qpMap.find(key);
		if (it != m_qpMap.end())
			return it->second;
		return NULL;
	}

	void RdmaHw::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish, Time stopTime) { // important!
		// create qp,发起credit request
		Ptr<RdmaTxWorkQueue> qp = CreateObject<RdmaTxWorkQueue>(pg, sip, dip, sport, dport);
		qp->SetSize(size);
		qp->SetBaseRtt(baseRtt);
		qp->SetAppNotifyCallback(notifyAppFinish);
		qp->stopTime = stopTime;
		if (stopTime == Simulator::GetMaximumSimulationTime()-MicroSeconds(1)){
			qp->incastFlow = 1;
		}
		else{
			qp->incastFlow = 0;
		}
		
		// add qp
		uint32_t nic_idx = GetNicIdxOfQp(qp);
		m_nic[nic_idx].qpGrp->AddQp(qp);
		uint64_t key = GetQpKey(dip.Get(), sport, pg);
		m_qpMap[key] = qp;

		// set init variables
		// DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
		
		// qp->m_rate = m_bps; //最开始是线速
		// qp->m_max_rate = m_bps;

		// Notify Nic 触发发送一个分组,我们得包头逐个套上
		//我们通过发起request请求来开始流
		XpassHeader xph;
		xph.SetSeq(qp->snd_nxt);
		xph.SetPG(pg);
		xph.SetSport(sport);
		xph.SetDport(dport);
		// seqh.SetWin(ch.udp.win_size);
		
		xph.SetRtt(baseRtt);
		//设置为request包
		xph.SetMtype(0x01);	
		xph.SetCredit_Sent_Time(Simulator::Now().GetNanoSeconds());
		//pkt_remaining
		xph.SetSendBuffer(ceil(qp->GetBytesLeft()/1500.0));

		Ptr<Packet> newp = Create<Packet>(std::max(60 - 14 - 20 - (int)xph.GetSerializedSize(), 0));
		
		newp->AddHeader(xph);

		Ipv4Header head;	// Prepare IPv4 header
		head.SetDestination(Ipv4Address(dip));
		head.SetSource(Ipv4Address(sip));
		head.SetProtocol(0x01); //ack=0xFC,0x01表示使用的xpass算法
		head.SetTtl(64);
		head.SetPayloadSize(newp->GetSize());

		head.SetIdentification(qp->m_ipid);

		newp->AddHeader(head);
		AddHeader(newp, 0x800);	// Attach PPP header

		// printf("Flow %d is transmitting...\n", dport);
		m_nic[nic_idx].dev->m_rdmaEQ->m_traceRdmaDequeue(newp, pg);
		m_nic[nic_idx].dev->m_rdmaEQ->EnqueueDataQ(newp); //request是从sender发出，应在DataQ
		m_nic[nic_idx].dev->DequeueAndTransmit();
	}

	void RdmaHw::DeleteQueuePair(Ptr<RdmaTxWorkQueue> qp) {
		// remove qp from the m_qpMap
		uint64_t key = GetQpKey(qp->dip.Get(), qp->sport, qp->m_pg);
		m_qpMap.erase(key);
	}

	Ptr<RdmaRxWorkQueue> RdmaHw::GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create) {
		uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
		auto it = m_rxQpMap.find(key);
		if (it != m_rxQpMap.end())
			return it->second;
		if (create) {
			// create new rx qp
			Ptr<RdmaRxWorkQueue> q = CreateObject<RdmaRxWorkQueue>();
			// init the qp
			q->sip = sip;
			q->dip = dip;
			q->sport = sport;
			q->dport = dport;
			q->c_pg = pg;
			q->credit_seq = 0;
			q->isFin = false;
			q->c_nextAvail = Time(0);
			// store in map
			m_rxQpMap[key] = q;
			q->credit_total_ = 0;
			q->credit_dropped_ = 0;
			q->c_recv_next_ = 0;
			return q;
		}
		return NULL;
	}
	uint32_t RdmaHw::GetNicIdxOfRxQp(Ptr<RdmaRxWorkQueue> q) {
		auto &v = m_rtTable[q->dip];
		if (v.size() > 0) {
			return v[q->GetHash() % v.size()];
		}
		else {
			NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
		}
	}
	void RdmaHw::DeleteRxQp(uint32_t dip, uint16_t pg, uint16_t dport) {
		uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
		m_rxQpMap.erase(key);
	}

	// void RdmaHw::ReceiveUdp(Ptr<Packet> p, CustomHeader &ch) {	
	// 	uint32_t payload_size = p->GetSize() - ch.GetSerializedSize(); // 读取分组长度
	// 	uint8_t syn = (ch.udp.flags >> CustomHeader::FLAG_SYN) & 1; //解析SYN标记
	// 	uint8_t fin = (ch.udp.flags >> CustomHeader::FLAG_FIN) & 1; //解析FIN标记

	// 	Ptr<RdmaRxWorkQueue> rxQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, true); // 根据five tuple找到相应的QP
		
	// 	if (ReceiverCheckSeq(ch.udp.seq, rxQp,  payload_size)) { // 检查序号
		
	// 		qbbHeader seqh;
	// 		seqh.SetSeq(rxQp->expected_seq);
	// 		seqh.SetPG(ch.udp.pg);
	// 		seqh.SetSport(ch.udp.dport);
	// 		seqh.SetDport(ch.udp.sport);
	// 		// seqh.SetWin(ch.udp.win_size);
	// 		seqh.SetRtt((64 - (ch.m_ttl - 1)) * 2);
	// 		if(syn)
	// 		{
	// 			seqh.SetSyn();
	// 			m_totalFlowCount++;
	// 			// printf("receive syn packet of flow %d @ %d\n", ch.udp.dport, Simulator::Now().GetNanoSeconds());
	// 		}
	// 		if(fin)
	// 		{
	// 			seqh.SetFin();
	// 			m_totalFlowCount--;
	// 			// printf("receive fin packet of flow %d @ %d\n", ch.udp.dport, Simulator::Now().GetNanoSeconds());
	// 		}

	// 		Ptr<Packet> newp = Create<Packet>(std::max(60 - 14 - 20 - (int)seqh.GetSerializedSize(), 0));
	// 		newp->AddHeader(seqh);

	// 		Ipv4Header head;	// Prepare IPv4 header
	// 		head.SetDestination(Ipv4Address(ch.sip));
	// 		head.SetSource(Ipv4Address(ch.dip));
	// 		head.SetProtocol(0xFC); //ack=0xFC
	// 		head.SetTtl(64);
	// 		head.SetPayloadSize(newp->GetSize());
	// 		head.SetIdentification(rxQp->m_ipid++);

	// 		newp->AddHeader(head);
	// 		AddHeader(newp, 0x800);	// Attach PPP header
	// 		// send
	// 		uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
	// 		m_nic[nic_idx].dev->m_rdmaEQ->EnqueueAckQ(newp);
	// 		m_nic[nic_idx].dev->DequeueAndTransmit();
	// 	}
	// 	return;
	// }

	/*
	* Function Description: 从该TxWQ中提取出1或2个分组，推入dataQ
	* note：在仿真中，由于调度是逐个QP的，因此无需per-rtt Queues
	*/
	// void RdmaHw::ReceiveAck(Ptr<Packet> p, CustomHeader &ch) {
	// 	uint16_t qIndex = ch.ack.pg; //Priority Group，即优先级
	// 	uint16_t dport = ch.ack.dport; //分组目的端口
	// 	uint32_t sip = ch.sip;
	// 	uint32_t seq = ch.ack.seq; //分组序号seq
	// 	uint8_t rc = (ch.ack.flags & 0x03);

	// 	uint8_t inc = (rc == VALVE_FLAG_INC); //解析INC标记
	// 	uint8_t dec = (rc == VALVE_FLAG_DBE || rc == VALVE_FLAG_ABE); //解析DEC标记
	// 	uint8_t syn = (ch.ack.flags >> qbbHeader::FLAG_SYN) & 1; //解析SYN标记
	// 	uint8_t fin = (ch.ack.flags >> qbbHeader::FLAG_FIN) & 1; //解析FIN标记

	// 	uint8_t n; //本次ACK触发的data packet数目
	// 	uint32_t ttl = ch.m_ttl; //解析TTL

	// 	Ptr<RdmaTxWorkQueue> qp = GetQp(sip, dport, qIndex); //根据源IP和目的端口以及队列优先级找到Queue Pair
	// 	if (qp == NULL) { //如果没有找到QP则报error
	// 		std::cout << "ERROR: " << "node:" << m_node->GetId() << ' ' << (ch.l3Prot == 0xFC ? "ACK" : "NACK") << " NIC cannot find the flow\n";
	// 		return;
	// 	}
	// 	uint32_t nic_idx = GetNicIdxOfQp(qp); //找到TxWQ所在的NIC的index
	// 	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev; //找到NIC所在的device

	// 	if(syn){
	// 		std::cout << "Receiving SYN of flow " << ch.ack.sport << " @" << Simulator::Now().GetNanoSeconds() << "ns" << std::endl; 
	// 	}
	// 	if(fin){
	// 		std::cout << "Receiving FIN of flow " << ch.ack.sport << " @" << Simulator::Now().GetNanoSeconds() << "ns" << std::endl; 
	// 	}

	// 	std::cout << "Receiving " << (dec!=0?"DEC":(inc!=0?"INC":"REG")) << " ACK " << "fid:" << ch.ack.sport << " wnd:" << ch.ack.win_size << " @ " << Simulator::Now().GetNanoSeconds() << "ns" << std::endl;
		
	// 	qp->Acknowledge(seq); //维护发送窗口信息

	// 	// std::cout << "Window size of flow " << ch.ack.sport << " is " << (qp->snd_nxt - qp->snd_una) << std::endl;

	// 	if (qp->IsFinished()) {
	// 		QpComplete(qp);
	// 		std::cout << "Flow " << qp->dport << " is finished!" << std::endl;
	// 	}
	// 	else {
			
	// 		if (dec) { // DEC flag will take effects firstly
	// 			n = 0;
	// 		}
	// 		else if (inc) {
	// 			n = 2;
	// 		}
	// 		else {
	// 			n = 1;
	// 		}

	// 		Ptr<Packet> data_packet;
	// 		while(n > 0){
	// 			data_packet = dev->m_rdmaEQ->m_rdmaGetNxtPkt(qp); //从对应QP中提取pakcet
	// 			dev->m_rdmaEQ->m_traceRdmaDequeue(p, qp->m_pg);
	// 			if(data_packet != NULL){
	// 				dev->m_rdmaEQ->EnqueueDataQ(data_packet); //推入DataQ
	// 				n--;
	// 			}
	// 			else{
	// 				break;
	// 			}
	// 		}
	// 		// ACK may advance the on-the-fly window, allowing more packets to send
	// 		dev->DequeueAndTransmit(); //尝试转发分组
	// 	}
	// 	return;
	// }

	// //xpass 3/25
	void RdmaHw::recv_credit_request(Ptr<Packet> p, CustomHeader &ch) {
		double lalpha;
		// w_ = w_init_; //it determines how aggressively increase the credit sending rate.
		Time last_credit_rate_update_ = Simulator::Now();
		
		// fst_ = xph->credit_sent_time();
		// need to start to send credits.
		
		// credit_send_state_ = XPASS_SEND_CREDIT_SENDING; 
		// Ptr<RdmaRxWorkQueue> rxQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, true); // 根据five tuple找到相应的QP

		//这里的rxworkqueue在xpass算法中指代credit流
		
		Ptr<RdmaRxWorkQueue> rxQp = GetRxQp(ch.dip, ch.sip, ch.cdt.dport, ch.cdt.sport, ch.cdt.pg, true); // 根据five tuple找到相应的QP
	
		// send
		uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);

		//INIT 
		rxQp->max_rate = DataRate((uint64_t) m_nic[nic_idx].dev->max_credit_rate.GetBitRate()); //credit max rate.

		rxQp->alpha_ = m_nic[nic_idx].dev->alpha_;
		rxQp->max_jitter_ = m_nic[nic_idx].dev->max_jitter_;
		rxQp->min_jitter_ = m_nic[nic_idx].dev->min_jitter_;
		rxQp->target_loss_scaling_ = m_nic[nic_idx].dev->target_loss_scaling_;
		rxQp->w_init_ = m_nic[nic_idx].dev->w_init_;
		rxQp->w_ = rxQp->w_init_;
		rxQp->min_w_ = m_nic[nic_idx].dev->min_w_;
	
		//设置线速
		uint32_t sendBf = ch.cdt.sendbuffer;

		if(sendBf >= 40) {
			lalpha = rxQp->alpha_;
		}
		else {
			lalpha = rxQp->alpha_ * sendBf / 40.0;
		}
		
		rxQp->m_rate = (lalpha * rxQp->max_rate);

		//这里未记录fst_流开始时间
		m_nic[nic_idx].qpGrp->AddCreditQp(rxQp);
		m_nic[nic_idx].dev->DequeueAndTransmit();
	}
	void RdmaHw::recv_credit(Ptr<Packet> p, CustomHeader &ch) {
		//接收到一个credit之后，一一对应去传数据包，类似接收udp的数据包
		uint16_t qIndex = ch.cdt.pg; //Priority Group，即优先级
		uint16_t sport = ch.cdt.dport; //分组目的端口
		uint32_t dip = ch.sip;
		// std::cout << "receive credit seq :" << ch.cdt.credit_seq << " , at " << Simulator::Now() << std::endl;
		uint32_t ttl = ch.m_ttl; //解析TTL
		
		Ptr<RdmaTxWorkQueue> qp = GetQp(dip, sport, qIndex);
		if (qp == NULL) { //如果没有找到QP则报error
			std::cout << "ERROR: " << "node:" << m_node->GetId() << ' ' << (ch.cdt.mtype == 0x02 ? "credit" : "data") << " NIC cannot find the flow\n";
			return;
		}
		//如果流没完成，即nxt < m_size时
		if(qp->snd_nxt < qp->m_size) {
			uint32_t nic_idx = GetNicIdxOfQp(qp); //找到TxWQ所在的NIC的index
			Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev; //找到NIC所在的device
			// qp->Acknowledge(seq); //维护发送窗口信息
			// std::cout << "snd_una : " << qp->snd_una << ", m_size : "<< qp->m_size << std::endl;
			Ptr<Packet> data_packet;
			data_packet = dev->m_rdmaEQ->m_rdmaGetNxtPkt(qp); //从对应QP中提取pakcet
			CustomHeader header(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
			data_packet->RemoveHeader(header);
			//添加在data包头credit序号
			header.cdt.credit_seq = ch.cdt.credit_seq;
			data_packet->AddHeader(header);
			dev->m_rdmaEQ->m_traceRdmaDequeue(p, qp->m_pg);
			dev->m_rdmaEQ->EnqueueDataQ(data_packet); //推入DataQ
			
			dev->DequeueAndTransmit(); //尝试转发分组
		}
		
	}
	void RdmaHw::recv_ack(Ptr<Packet> p, CustomHeader &ch) {
		//接收到ack后，不用进行发包，而是更新窗口信息即可
		uint16_t qIndex = ch.cdt.pg; //Priority Group，即优先级
		uint16_t sport = ch.cdt.dport; //分组目的端口
		uint32_t dip = ch.sip;
		uint32_t seq = ch.cdt.m_seq; //分组序号seq

		Ptr<RdmaTxWorkQueue> qp = GetQp(dip, sport, qIndex);
		if (qp == NULL) { //如果没有找到QP则报error
			std::cout << "ERROR: " << "node:" << m_node->GetId() << " ack " << " NIC cannot find the flow\n";
			return;
		}
		qp->Acknowledge(seq); //维护发送窗口信息
		if (qp->IsFinished()) {
			// std::cout << "send fin1" << std::endl;
			//构造Fin包，告诉接收方发送结束
			// Ptr<Packet> finp = GetNxtPacket(qp);
			// CustomHeader header(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
			// finp->RemoveHeader(header);

			// header.cdt.credit_seq = 0xffffffff;
			// header.cdt.mtype = 0x05;
			// std::cout << "send sport: " << header.cdt.sport << " dport" << header.cdt.dport << "\n";
			// finp->AddHeader(header);
			
			XpassHeader xph;
			xph.SetSeq(qp->snd_nxt);
			xph.SetPG(qIndex);
			xph.SetSport(sport);
			xph.SetDport(ch.cdt.sport);
			// seqh.SetWin(ch.udp.win_size);
			xph.SetRtt(ch.cdt.m_rtt);
			//设置为request包
			xph.SetMtype(0x05);	 //fin1是结束0x05

			Ptr<Packet> newp = Create<Packet>(std::max(60  - 14 - 20 - (int)xph.GetSerializedSize(), 0));
			
			newp->AddHeader(xph);

			Ipv4Header head;	// Prepare IPv4 header
			head.SetDestination(Ipv4Address(dip));
			head.SetSource(Ipv4Address(ch.dip));
			head.SetProtocol(0x01); //ack=0xFC,0x01表示使用的xpass算法
			head.SetTtl(64);
			head.SetPayloadSize(newp->GetSize());

			head.SetIdentification(qp->m_ipid);

			newp->AddHeader(head);
			AddHeader(newp, 0x800);	// Attach PPP header

			uint32_t nic_idx = GetNicIdxOfQp(qp); //找到TxWQ所在的NIC的index
			// std::cout << "send sport: " << sport << " dip: " << dip.Get() << " qindex: " << ACK_Q_IDX << std::endl;
			m_nic[nic_idx].dev->m_rdmaEQ->m_traceRdmaDequeue(newp, qIndex);
			m_nic[nic_idx].dev->m_rdmaEQ->EnqueueDataQ(newp); //request是从sender发出
			m_nic[nic_idx].dev->DequeueAndTransmit();
		}
	}
	void RdmaHw::recv_fin1 (Ptr<Packet> p, CustomHeader &ch){
		// std::cout << "recv fin1 dport : " << ch.cdt.dport << " , sport : " << ch.cdt.sport << std::endl;
		Ptr<RdmaRxWorkQueue> rxQp = GetRxQp(ch.dip, ch.sip, ch.cdt.dport, ch.cdt.sport, ch.cdt.pg, false); // 根据five tuple找到相应的QP
		// std::cout << "查找对应的rxqp : " << rxQp << std::endl;
		rxQp->isFin = true;

		XpassHeader xqh;
		xqh.SetSeq(0xffffffff); //结束发送全f
		xqh.SetPG(ch.cdt.pg);
		xqh.SetSport(ch.cdt.dport);
		xqh.SetDport(ch.cdt.sport);
		// seqh.SetWin(ch.udp.win_size);
		xqh.SetRtt((64 - (ch.m_ttl - 1)) * 2);
		xqh.SetMtype(0x06); //0x06表示fin2

		Ptr<Packet> newp = Create<Packet>(std::max(60 - 14 - 20 - (int)xqh.GetSerializedSize(), 0));
		newp->AddHeader(xqh);

		Ipv4Header head;	// Prepare IPv4 header
		head.SetDestination(Ipv4Address(ch.sip));
		head.SetSource(Ipv4Address(ch.dip));
		head.SetProtocol(0x01); //为xpass
		head.SetTtl(64);
		head.SetPayloadSize(newp->GetSize());
		head.SetIdentification(rxQp->m_ipid++);

		newp->AddHeader(head);
		AddHeader(newp, 0x800);	// Attach PPP header
		
		uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
		m_nic[nic_idx].dev->m_rdmaEQ->EnqueueAckQ(newp);
		m_nic[nic_idx].dev->DequeueAndTransmit();

		DeleteRxQp(ch.sip,ch.cdt.pg, ch.cdt.sport);
	} 
	void RdmaHw::recv_fin2 (Ptr<Packet> p, CustomHeader &ch) {
		uint16_t qIndex = ch.cdt.pg; //Priority Group，即优先级
		uint16_t sport = ch.cdt.dport; //分组目的端口
		uint32_t dip = ch.sip;
		
		Ptr<RdmaTxWorkQueue> qp = GetQp(dip, sport, qIndex);
		if(qp == NULL) 
			std::cout << "ERROR: " << "node:" << m_node->GetId() << " fin2 NIC cannot find the flow\n";
		else {
			QpComplete(qp);
			// std::cout << "Flow " << qp->dport << " is finished!" << std::endl;
		}
	}
	void RdmaHw::recv_data(Ptr<Packet> p, CustomHeader &ch) {
		//收到data报文后，继续发credit帧
		uint32_t payload_size = p->GetSize() - ch.GetSerializedSize(); // 读取分组长度
		// std::cout << "receive data at " << ch.cdt.dport << std::endl;
		Ptr<RdmaRxWorkQueue> rxQp = GetRxQp(ch.dip, ch.sip, ch.cdt.dport, ch.cdt.sport, ch.cdt.pg, false); // 根据five tuple找到相应的QP
		
		//计算丢包率
		uint32_t distance = ch.cdt.credit_seq - rxQp->c_recv_next_;
		if(distance < 0) {
			std::cout << "ERROR: " << "node:" << m_node->GetId() << " receive data credit seq is less than c_recv_next\n";
			return;
		}
		rxQp->credit_total_ += (distance + 1);
		rxQp->credit_dropped_ += distance;
		rxQp->c_recv_next_ = ch.cdt.credit_seq + 1;
		// std::cout << "receive data credit seq" << ch.cdt.credit_seq << std::endl;
		// std::cout << "distance:"  << distance << " , credit_recv_next : " << rxQp->c_recv_next_ << " credit_total_ : " << rxQp->credit_total_ << " credit_dropped_ : " << rxQp->credit_dropped_ << std::endl;
		rxQp->update_rtt(ch);
		rxQp->expected_seq += payload_size;
		XpassHeader xqh;
		xqh.SetSeq(rxQp->expected_seq);
		xqh.SetPG(ch.cdt.pg);
		xqh.SetSport(ch.cdt.dport);
		xqh.SetDport(ch.cdt.sport);
		// seqh.SetWin(ch.udp.win_size);
		xqh.SetRtt((64 - (ch.m_ttl - 1)) * 2);
		xqh.SetMtype(0x04); //0x04表示ack
		xqh.SetCreditSeq(ch.cdt.credit_seq);
		Ptr<Packet> newp = Create<Packet>(std::max(60 - 14 - 20 - (int)xqh.GetSerializedSize(), 0));
		newp->AddHeader(xqh);

		Ipv4Header head;	// Prepare IPv4 header
		head.SetDestination(Ipv4Address(ch.sip));
		head.SetSource(Ipv4Address(ch.dip));
		head.SetProtocol(0x01); //为xpass
		head.SetTtl(64);
		head.SetPayloadSize(newp->GetSize());
		head.SetIdentification(rxQp->m_ipid++);

		newp->AddHeader(head);
		AddHeader(newp, 0x800);	// Attach PPP header
		
		uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
		m_nic[nic_idx].dev->m_rdmaEQ->EnqueueAckQ(newp);
		m_nic[nic_idx].dev->DequeueAndTransmit();
	}


	void RdmaHw::Receive(Ptr<Packet> p, CustomHeader &ch) { // important!
		// if (ch.l3Prot == 0xFC) { // ACK
		// 	ReceiveAck(p, ch);
		// }
		// else if (ch.l3Prot == 0x11) { // UDP
		// 	ReceiveUdp(p, ch);
		// }
		if (ch.l3Prot == 0x01) { //xpass报文
			// std::cout << p->GetSize() << std::endl;
			switch (ch.cdt.mtype) {
				//credit_request请求
				case 0x01:
					recv_credit_request(p,ch);
					break;
				//credit
				case 0x02:
					recv_credit(p,ch);
					break;
				//data
				case 0x03:
					recv_data(p,ch);
					break;
				case 0x04:
					recv_ack(p,ch);
					break;
				case 0x05:
					recv_fin1(p, ch);
					break;
				case 0x06:
					recv_fin2(p,ch);
					break;
			}//3/25
			
		}
		else { // drop dummy packet
			
		}
		return;
	}


	bool RdmaHw::ReceiverCheckSeq(uint32_t seq, Ptr<RdmaRxWorkQueue> rxwq, uint32_t size) {
		uint32_t expected = rxwq->expected_seq;
		
		if (seq == expected) {
			rxwq->expected_seq = expected + size;
			return true;
		}
		else {
			return false;
		}
	}

	void RdmaHw::AddHeader (Ptr<Packet> p, uint16_t protocolNumber) {
		PppHeader ppp;
		ppp.SetProtocol (EtherToPpp (protocolNumber));
		p->AddHeader (ppp);
	}
	uint16_t RdmaHw::EtherToPpp (uint16_t proto) {
		switch (proto) {
			case 0x0800: return 0x0021;   //IPv4
			case 0x86DD: return 0x0057;   //IPv6
			default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
		}
		return 0;
	}

	void RdmaHw::QpComplete(Ptr<RdmaTxWorkQueue> qp) {
		NS_ASSERT(!m_qpCompleteCallback.IsNull());

		// This callback will log info
		// It may also delete the rxQp on the receiver
		m_qpCompleteCallback(qp);

		qp->m_notifyAppFinish();

		// delete the qp
		DeleteQueuePair(qp);
	}

	void RdmaHw::SetLinkDown(Ptr<QbbNetDevice> dev) {
		printf("RdmaHw: node:%u a link down\n", m_node->GetId());
	}

	void RdmaHw::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx) {
		uint32_t dip = dstAddr.Get();
		m_rtTable[dip].push_back(intf_idx);
	}

	void RdmaHw::ClearTable() {
		m_rtTable.clear();
	}

	void RdmaHw::RedistributeQp() {
		// clear old qpGrp
		for (uint32_t i = 0; i < m_nic.size(); i++) {
			if (m_nic[i].dev == NULL)
				continue;
			m_nic[i].qpGrp->Clear();
		}

		// redistribute qp
		for (auto &it : m_qpMap) {
			Ptr<RdmaTxWorkQueue> qp = it.second;
			uint32_t nic_idx = GetNicIdxOfQp(qp);
			m_nic[nic_idx].qpGrp->AddQp(qp);
			// Notify Nic
			m_nic[nic_idx].dev->DequeueAndTransmit();
		}
	}

	Ptr<Packet> RdmaHw::GetNxtPacket(Ptr<RdmaTxWorkQueue> qp) {
		uint32_t payload_size = qp->GetBytesLeft();
		if (payload_size == 0){
			return NULL;
		}
		if (m_mtu < payload_size)
			payload_size = m_mtu;
		Ptr<Packet> p = Create<Packet> (payload_size);
	
		// add XpassHeader
		XpassHeader xph;
		xph.SetSeq(qp->snd_nxt);
		xph.SetPG(qp->m_pg);
		xph.SetSport(qp->sport);
		xph.SetDport(qp->dport);
		//设置为data包
		xph.SetMtype(0x03);

		xph.SetCredit_Sent_Time(Simulator::Now().GetNanoSeconds());
		p->AddHeader(xph);

		// update state
		qp->snd_nxt += payload_size; 

		// add udp header
		// UdpHeader udpHeader;
		// udpHeader.SetDestinationPort (qp->dport);
		// udpHeader.SetSourcePort (qp->sport);
		// p->AddHeader (udpHeader);
		// add ipv4 header
		Ipv4Header ipHeader;
		ipHeader.SetSource (qp->sip);
		ipHeader.SetDestination (qp->dip);
		ipHeader.SetProtocol (0x01);
		ipHeader.SetPayloadSize (p->GetSize());
		ipHeader.SetTtl (64);
		ipHeader.SetTos (0);
		ipHeader.SetIdentification (qp->m_ipid);
		p->AddHeader(ipHeader);
		// add ppp header
		PppHeader ppp;
		ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
		p->AddHeader (ppp);

		// std::cout << "Sending UDP " << "fid:" << qp->dport << " wnd:" << winSize << " @ " << Simulator::Now().GetNanoSeconds() << "ns"  << std::endl;
		qp->m_ipid++;

		// CustomHeader header(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		// p->RemoveHeader(header);

		// printf("remaining bytes of flow %d: %d\n",header.udp.dport, qp->GetBytesLeft());

		// if(qp->GetBytesLeft() == 0){
		// 	// set FIN flag
		// 	header.udp.flags |= (1 << CustomHeader::FLAG_FIN);
		// }
		// else{
		// 	header.udp.flags &= ~(1 << CustomHeader::FLAG_FIN);
		// }
		// header.udp.flags &= ~(1 << CustomHeader::FLAG_SYN);
		// p->AddHeader(header);
		// CustomHeader header(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		// p->PeekHeader(header);
		// printf("%d %d\n", header.l3Prot, header.cdt.mtype);

		qp->numTxBytes += payload_size;

		// return
		return p;
	}

	Ptr<Packet> RdmaHw::GetNxtCreditPacket(Ptr<RdmaRxWorkQueue> qp) {
		// add XpassHeader
		XpassHeader xph;
		xph.SetCreditSeq(qp->credit_seq);
		xph.SetPG(qp->c_pg);
		xph.SetSport(qp->sport);
		xph.SetDport(qp->dport);
		//设置为credit包
		xph.SetMtype(0x02);
		
		uint32_t payload_size = std::max(60 - 14 - 20 - (int)xph.GetSerializedSize(), 0);
		if (m_mtu < payload_size)
			payload_size = m_mtu;

		Ptr<Packet> p = Create<Packet> (payload_size);
		p->AddHeader (xph);
		// update state
		// qp->credit_seq += payload_size; 
		// std::cout << "send credit packet seq: " << qp->credit_seq << std::endl;
		qp->credit_seq++;//credit包需要加1
		
		// add ipv4 header
		Ipv4Header ipHeader;
		ipHeader.SetSource(Ipv4Address(qp->sip));
		ipHeader.SetDestination(Ipv4Address(qp->dip));
		ipHeader.SetProtocol (0x01);
		ipHeader.SetPayloadSize (p->GetSize());
		ipHeader.SetTtl (64);
		ipHeader.SetTos (0);
		ipHeader.SetIdentification (qp->m_ipid);
		p->AddHeader(ipHeader);
		// add ppp header
		PppHeader ppp;
		ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
		p->AddHeader (ppp);

		// std::cout << "Sending UDP " << "fid:" << qp->dport << " wnd:" << winSize << " @ " << Simulator::Now().GetNanoSeconds() << "ns"  << std::endl;
		qp->m_ipid++;
		
		// qp->numTxBytes += payload_size;
		
		// return
		return p;                             
	}

	void RdmaHw::PktSent(Ptr<RdmaRxWorkQueue> qp, Ptr<Packet> pkt, Time interframeGap) {
		qp->lastPktSize = pkt->GetSize();
		// uint32_t seq = qp->snd_nxt;
		UpdateNextAvail(qp, interframeGap, pkt->GetSize());
	}

	void RdmaHw::UpdateNextAvail(Ptr<RdmaRxWorkQueue> qp, Time interframeGap, uint32_t pkt_size) {
		//xpass by sgh
		// Time sendingTime = interframeGap + qp->m_max_rate.CalculateBytesTxTime(pkt_size);
		// Time sendingTime = interframeGap + qp->m_rate.CalculateBytesTxTime(pkt_size);
		//5/27 by sgh
		qp->credit_feedback_control();
		Time sendingTime = interframeGap + NanoSeconds(qp->credit_size * 8 *  1e9 / qp->m_rate);
		// add jitter（防止同步导致的不公平性）
		if (qp->max_jitter_ > qp->min_jitter_) {
			double jitter = (double)rand()/(double)RAND_MAX;
			jitter = jitter * (qp->max_jitter_ - qp->min_jitter_) + qp->min_jitter_;
			
			// jitter is in the range between min_jitter_ and max_jitter_
			sendingTime = sendingTime * ( 1 + jitter);
		}else if (qp->max_jitter_ < qp->min_jitter_) {+
			fprintf(stderr, "ERROR: max_jitter_ should be larger than min_jitter_");
			exit(1);
		}
		qp->c_nextAvail = Simulator::Now() + sendingTime;
	}
	
}
