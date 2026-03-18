#include "ns3/ipv4.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/pause-header.h"
#include "ns3/interface-tag.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "switch-node.h"
#include "qbb-net-device.h"
#include "ppp-header.h"
#include "qbb-header.h"
#include "ns3/int-header.h"
#include <cmath>

namespace ns3 {

TypeId SwitchNode::GetTypeId (void) { // NS3原生函数，配置Switch Node参数
	static TypeId tid = TypeId ("ns3::SwitchNode")
	.SetParent<Node> ();
	return tid;
}

SwitchNode::SwitchNode() { // 构造函数，初始化
	m_ecmpSeed = m_id;
	m_node_type = 1;
	m_mmu = CreateObject<SwitchMmu>();
	for (uint32_t i = 0; i < pCnt; i++)
		for (uint32_t j = 0; j < pCnt; j++)
			for (uint32_t k = 0; k < qCnt; k++)
				m_bytes[i][j][k] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_txBytes[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_lastPktSize[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++){
				// for (uint32_t j = 0; j < pCnt; j++){
		// 	m_perPortFlowCount[i][j] = 0;
		// }
		m_totalFlowCount_ingr[i] = 0;
		m_totalFlowCount_egr[i] = 0;
	}
}

//24/10/12 根据flowid查找其对应的ingressid(选择对称路由链路的出端口)
int SwitchNode::GetOutDev_for_ack(Ptr<const Packet> p, CustomHeader &ch){
	auto entry = m_rtTable_for_ack.find(ch.cdt.dport);
	if(entry == m_rtTable_for_ack.end()){
		return -1;
	}
	else{
		return entry->second;
	}
}

int SwitchNode::GetOutDev(Ptr<const Packet> p, CustomHeader &ch){ // important!
	// look up entries
	auto entry = m_rtTable.find(ch.dip);

	// no matching entry
	if (entry == m_rtTable.end())
		return -1;

	// entry found
	auto &nexthops = entry->second;

	// pick one next hop based on hash
	union {
		uint8_t u8[4+4+2+2];
		uint32_t u32[3];
	} buf;
	buf.u32[0] = ch.sip;
	buf.u32[1] = ch.dip;
	if (ch.l3Prot == 0x01)
		buf.u32[2] = ch.cdt.sport | ((uint32_t)ch.cdt.dport << 16);

	uint32_t idx = EcmpHash(buf.u8, 12, m_ecmpSeed) % nexthops.size();
	return nexthops[idx];
}

void SwitchNode::CheckAndSendPfc(uint32_t inDev, uint32_t qIndex){
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
	if (m_mmu->CheckShouldPause(inDev, qIndex)){
		device->SendPfc(qIndex, 0); // 0代表PAUSE帧
		m_mmu->SetPause(inDev, qIndex);
	}
}

void SwitchNode::CheckAndSendResume(uint32_t inDev, uint32_t qIndex){
	
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
	if (m_mmu->CheckShouldResume(inDev, qIndex)){
		device->SendPfc(qIndex, 1); // 1代表RESUME帧
		m_mmu->SetResume(inDev, qIndex);
	}
}

void SwitchNode::SendToDev(Ptr<Packet> p, CustomHeader &ch){ //ingress逻辑 // important!

	int idx;

	if(ch.m_ttl <= 0){
		return;
	}
	InterfaceTag t;
	p->RemovePacketTag(t);
	uint32_t inDevId = t.GetPortId();
	//credit包,request包和credit包都不需要按照对称路由,xpass by sgh
	if(ch.l3Prot == 0x01) {
		if(ch.cdt.mtype == 0x03){
			idx = GetOutDev_for_ack(p, ch);
		}
		else{
			idx = GetOutDev(p, ch);
			// 	//如果rtTable_for_ack匹配失败则增加一个entry
			if(ch.cdt.mtype == 0x02){//这里是credit，要用源port
				auto entry = m_rtTable_for_ack.find(ch.cdt.sport);
				if(entry == m_rtTable_for_ack.end()){
					AddTableEntry_for_ack(ch.cdt.sport, inDevId);
				}
			}
		}	
	}//数据包
	// idx = GetOutDev(p, ch); //计算得到出端口

	// if(ch.l3Prot == 0xFC){ // ACK分组
	// 	idx = GetOutDev_for_ack(p, ch); //计算得到出端口
	// }
	// else{
	// 	idx = GetOutDev(p, ch); //计算得到出端口
	// }

	Ptr<QbbNetDevice> output_device = DynamicCast<QbbNetDevice>(m_devices[idx]);
	// uint32_t qIndex = ch.l3Prot == 0xFC ? ch.ack.pg : ch.udp.pg;

	// 10/14
	// if (qIndex != 0) {
	// 	// if (m_mmu->CheckIngressAdmission(inDevId, qIndex, p->GetSize()) 
	// 	//  && m_mmu->CheckEgressAdmission(idx, qIndex, p->GetSize())) {
	// 		m_mmu->UpdateIngressAdmission(inDevId, qIndex, p->GetSize());
			// m_mmu->UpdateEgressAdmission(idx, qIndex, p->GetSize());
	// 	// }
	// 	// else {
	// 	// 	return; // Drop
	// 	// }
	// 	// CheckAndSendPfc(inDevId, qIndex);
	// }

	if (idx >= 0) {
		CustomHeader header(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		p->RemoveHeader(header);
		m_mmu->UpdateEgressAdmission(idx, ch.cdt.pg, p->GetSize());
		// NS_ASSERT_MSG(output_device->IsLinkUp(), "The routing table look up should return link that is up");
		// Ptr<QbbNetDevice> input_device = DynamicCast<QbbNetDevice>(m_devices[inDevId]);
		// if(ch.l3Prot == 0xFC){ // ACK分组，执行DBE逻辑
			
		// 	//维护flow计数器
		// 	if((header.ack.flags >> qbbHeader::FLAG_SYN) & 1)
		// 		m_totalFlowCount_ingr[inDevId]++;
		// 	if((header.ack.flags >> qbbHeader::FLAG_FIN) & 1)
		// 		m_totalFlowCount_ingr[inDevId]--;
	
		// 	uint8_t rc = (header.ack.flags & 0x03);
		// 	uint8_t inc = (rc == VALVE_FLAG_INC); //解析INC标记
		// 	uint8_t reg = (rc == VALVE_FLAG_INC); //解析INC标记
		// 	uint8_t dec = (rc == VALVE_FLAG_DBE || rc == VALVE_FLAG_ABE); //解析DEC标记

		// 	if(m_totalFlowCount_ingr[inDevId] != 0){
		// 		if(rc == VALVE_FLAG_INC || rc == VALVE_FLAG_REG){
		// 			u_int32_t bdp = ch.ack.rtt * input_device->m_channel->GetDelay().GetNanoSeconds() * input_device->GetDataRate().GetBitRate() / 1e9 / 8; //所属流的base RTT，单位Bytes
		// 			u_int32_t wnd_t = bdp / m_totalFlowCount_ingr[inDevId]; //窗口大小的理想值
		// 			u_int32_t threshold = input_device->m_alpha * wnd_t; //新手保护期阈值

		// 			u_int32_t qlen = input_device->m_queue->GetNBytes(DATA_Q_IDX); //同线卡上，egress的队列长度，单位Bytes
		// 			double backlog_data = (double)qlen / (double)(input_device->m_mtu); //将qlen换算为MTU数量

		// 			u_int32_t rand_num = u_int32_t( double(rand())/double(RAND_MAX) * 1000); //生成分布在[0,1000)的随机整数，均匀分布
		// 			u_int32_t prob_t = backlog_data * input_device->m_kp_lc_i; //根据qlen计算的概率值
		// 			u_int32_t prob;

		// 			if(ch.ack.win_size <= threshold){
		// 				prob = 0;
		// 			}
		// 			else{
		// 				prob = (ch.ack.win_size - threshold) * prob_t / (wnd_t - threshold);
		// 			}
		// 			if (rand_num <= prob){
		// 				if(rc == VALVE_FLAG_INC){
		// 					header.ack.flags &= (~(0x03));
		// 					header.ack.flags += VALVE_FLAG_REG;//打标记
		// 				}
		// 				else{
		// 					header.ack.flags &= (~(0x03));
		// 					header.ack.flags += VALVE_FLAG_DBE;//打标记
		// 				}
		// 			}
		// 		}
		// 		else{

		// 		}
		// 		// double backlog_d = backlog_data - input_device->m_backlog_last;
		// 		// uint64_t deltat = Simulator::Now().GetNanoSeconds() - input_device->m_dec_time_last;
		// 		// input_device->m_backlog_last = backlog_data;
		// 		// input_device->m_dec_time_last = Simulator::Now().GetNanoSeconds();
		// 	}
		// }
		// else if(ch.l3Prot == 0x11){ // DATA分组

		// 	//如果rtTable_for_ack匹配失败则增加一个entry
		// 	auto entry = m_rtTable_for_ack.find(ch.udp.dport);
		// 	if(entry == m_rtTable_for_ack.end()){
		// 		AddTableEntry_for_ack(ch.udp.dport, inDevId);
		// 	}
		// }
		// else{ // dummy分组
		// 	return;
		// }

		// header.m_ttl--;
		p->AddHeader(header);
		// std::cout << "receiving a packet!\n";
		p->AddPacketTag(t);
		//是credit包就根据token去转发
		if(ch.cdt.mtype == 0x02) {
			// std::cout << "credit packet seq " << ch.cdt.credit_seq << " arrive at switch in "<< Simulator::Now() << std::endl;
			output_device->updateTokenBucket();
			
			if (output_device->tokens_ >= p->GetSize()){
				// output_device->DequeueAndTransmit();
				output_device->m_queue->Enqueue(p, ch.cdt.pg);
				output_device->tokens_ -= p->GetSize();
				// std::cout << "remaining token " << output_device->tokens_ << std::endl;
				output_device->DequeueAndTransmit();
				// std::cout << "发送后的token_" << output_device->tokens_ <<std::endl;
			}
			else {
				// std::cout << "token_不够了" << std::endl;
				// Time delay = (Time) NanoSeconds((p->GetSize() - output_device->tokens_ ) * 1e9 / output_device->token_refresh_rate_);
				// //在token够时去发送credit包
				// std::cout << "delay :: " << delay << std::endl;
				// std::cout << "now :: " << Simulator::Now() << std::endl;
                // Simulator::Schedule(Simulator::Now() + delay, &QbbNetDevice::DequeueAndTransmit, output_device);
				// std::cout << "drop credit_seq: " << ch.cdt.credit_seq << std::endl;
				return;
			}
		}
		//数据包就直接转发
		else{
			output_device->m_queue->Enqueue(p, ch.cdt.pg);
			output_device->DequeueAndTransmit();
			// std::cout << "transmit data or ack packet at " << Simulator::Now() << std::endl;
		}
	}
	else {
		/* do nothing */
		return;
	}
}

uint32_t SwitchNode::EcmpHash(const uint8_t* key, size_t len, uint32_t seed) {
	uint32_t h = seed;
	if (len > 3) {
		const uint32_t* key_x4 = (const uint32_t*) key;
		size_t i = len >> 2;
		do {
			uint32_t k = *key_x4++;
			k *= 0xcc9e2d51;
			k = (k << 15) | (k >> 17);
			k *= 0x1b873593;
			h ^= k;
			h = (h << 13) | (h >> 19);
			h += (h << 2) + 0xe6546b64;
		} while (--i);
		key = (const uint8_t*) key_x4;
	}

	if (len & 3) {
		size_t i = len & 3;
		uint32_t k = 0;
		key = &key[i - 1];
		do {
			k <<= 8;
			k |= *key--;
		} while (--i);
		k *= 0xcc9e2d51;
		k = (k << 15) | (k >> 17);
		k *= 0x1b873593;
		h ^= k;
	}

	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

void SwitchNode::SetEcmpSeed(uint32_t seed){
	m_ecmpSeed = seed;
}

void SwitchNode::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx){ // important!
	uint32_t dip = dstAddr.Get();
	m_rtTable[dip].push_back(intf_idx);
}

void SwitchNode::AddTableEntry_for_ack(uint32_t fid, uint32_t intf_idx){
	m_rtTable_for_ack[fid] = intf_idx;
}

void SwitchNode::ClearTable(){
	m_rtTable.clear();
}

// NON FUNCTIONAL This function can only be called in switch mode
bool SwitchNode::SwitchReceiveFromDevice(Ptr<QbbNetDevice> input_device, Ptr<Packet> packet, CustomHeader &ch){
	SendToDev(packet, ch);
	return true;
}

void SwitchNode::SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p){ // egress逻辑 // important!

	InterfaceTag t;
	p->RemovePacketTag(t);
	uint32_t input_device_id = t.GetPortId();

	// CustomHeader header(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	// p->RemoveHeader(header);

	// Ptr<QbbNetDevice> output_device = DynamicCast<QbbNetDevice>(m_devices[ifIndex]);

	// if(header.l3Prot == 0xFC){ // ACK分组

	// 	if((header.ack.flags >> qbbHeader::FLAG_SYN) & 1)
	// 		m_totalFlowCount_egr[ifIndex]++;
	// 	if((header.ack.flags >> qbbHeader::FLAG_FIN) & 1)
	// 		m_totalFlowCount_egr[ifIndex]--;

	// 	if(m_totalFlowCount_egr[ifIndex] != 0){

	// 		uint8_t rc = (header.ack.flags & 0x03);
	// 		uint8_t inc = (rc == VALVE_FLAG_INC); //解析INC标记
	// 		uint8_t reg = (rc == VALVE_FLAG_INC); //解析INC标记
	// 		uint8_t dec = (rc == VALVE_FLAG_DBE || rc == VALVE_FLAG_ABE); //解析DEC标记

	// 		if(m_totalFlowCount_egr[ifIndex] != 0){
	// 			if(rc == VALVE_FLAG_INC || rc == VALVE_FLAG_REG){
	// 				u_int32_t bdp = header.ack.rtt * output_device->m_channel->GetDelay().GetNanoSeconds() * output_device->GetDataRate().GetBitRate() / 1e9 / 8; //所属流的base RTT，单位Bytes
	// 				u_int32_t wnd_t = bdp / m_totalFlowCount_egr[ifIndex]; //窗口大小的理想值
	// 				u_int32_t threshold = output_device->m_alpha * wnd_t; //新手保护期阈值

	// 				u_int32_t qlen = output_device->m_queue->GetNBytes(ACK_Q_IDX); //同线卡上，egress的队列长度，单位Bytes
	// 				double backlog_ack = (double)qlen / (double)(p->GetSize()); //将qlen换算为ACK数量

	// 				u_int32_t rand_num = u_int32_t( double(rand())/double(RAND_MAX) * 1000); //生成分布在[0,1000)的随机整数，均匀分布
	// 				u_int32_t prob_t = backlog_ack * output_device->m_kp_lc_e; //根据qlen计算的概率值
	// 				u_int32_t prob;

	// 				if(header.ack.win_size <= threshold){
	// 					prob = 0;
	// 				}
	// 				else{
	// 					prob = (header.ack.win_size - threshold) * prob_t / (wnd_t - threshold);
	// 				}
	// 				if (rand_num <= prob){
	// 					if(rc == VALVE_FLAG_INC){
	// 						header.ack.flags &= (~(0x03));
	// 						header.ack.flags += VALVE_FLAG_REG;//打标记
	// 						output_device->m_shaping_pkt_num = 1;
	// 					}
	// 					else{
	// 						header.ack.flags &= (~(0x03));
	// 						header.ack.flags += VALVE_FLAG_ABE;//打标记
	// 						output_device->m_shaping_pkt_num = 0;
	// 					}
	// 				}
	// 			}
	// 			else if(rc == VALVE_FLAG_DBE){
	// 				output_device->m_shaping_pkt_num = 1;
	// 			}
	// 			else{ //ABE
	// 				output_device->m_shaping_pkt_num = 0;
	// 			}
	// 		}
	// 	}
	// }
	// else{ // DATA分组和dummy分组

	// }

	// p->AddHeader(header);

	// m_mmu->RemoveFromIngressAdmission(input_device_id, qIndex, p->GetSize());
	m_mmu->RemoveFromEgressAdmission(ifIndex, qIndex, p->GetSize());
	m_bytes[input_device_id][ifIndex][qIndex] -= p->GetSize();
	// CheckAndSendResume(input_device_id, qIndex);
	
	m_txBytes[ifIndex] += p->GetSize();
	m_lastPktSize[ifIndex] = p->GetSize();

    p->AddPacketTag(t); //添加Interface Tag
}

} /* namespace ns3 */
