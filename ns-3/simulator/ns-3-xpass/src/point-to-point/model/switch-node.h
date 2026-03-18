#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include <unordered_map>
#include <ns3/node.h>
#include "qbb-net-device.h"
#include "switch-mmu.h"
#include "pint.h"

#define LOSSLESS 0
#define LOSSY 1
#define DUMMY 2

namespace ns3 {

class Packet;

class SwitchNode : public Node{
	static const uint32_t pCnt = 64;	// Number of ports used
	static const uint32_t qCnt = 8;	// Number of queues/priorities used
	uint32_t m_ecmpSeed;
	std::unordered_map<uint32_t, std::vector<int> > m_rtTable; // map from ip address (u32) to possible ECMP port (index of dev)
	std::unordered_map<uint32_t, uint32_t > m_rtTable_for_ack; //map用于查找对称路由表项,first 是 dport(即接收端的端口),second 是 ingressid,

	// monitor of PFC
	uint32_t m_bytes[pCnt][pCnt][qCnt]; // m_bytes[inDev][outDev][qidx] is the bytes from inDev enqueued for outDev at qidx
	
	uint64_t m_txBytes[pCnt]; // counter of tx bytes for each port

	uint32_t m_lastPktSize[pCnt];

	// uint32_t m_perPortFlowCount[pCnt][pCnt]; // from port 1 to port 2
	uint32_t m_totalFlowCount_ingr[pCnt];
	uint32_t m_totalFlowCount_egr[pCnt];

private:
	int GetOutDev(Ptr<const Packet>, CustomHeader &ch);
	int GetOutDev_for_ack(Ptr<const Packet> p, CustomHeader &ch);
	static uint32_t EcmpHash(const uint8_t* key, size_t len, uint32_t seed);
	void CheckAndSendPfc(uint32_t inDev, uint32_t qIndex);
	void CheckAndSendResume(uint32_t inDev, uint32_t qIndex);
public:
	Ptr<SwitchMmu> m_mmu;

	static TypeId GetTypeId (void);
	SwitchNode();
	void SetMtu(uint32_t mtu);
	void SetEcmpSeed(uint32_t seed);
	void SetPidParameters(double kp, double ki, double kd);
	void SetAlpha(double alpha);
	void AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx);
	void AddTableEntry_for_ack(uint32_t fid, uint32_t intf_idx);
	void ClearTable();
	void SendToDev(Ptr<Packet>p, CustomHeader &ch);
	bool SwitchReceiveFromDevice(Ptr<QbbNetDevice> device, Ptr<Packet> packet, CustomHeader &ch);
	void SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p);
};

} /* namespace ns3 */

#endif /* SWITCH_NODE_H */
