#ifndef SWITCH_MMU_H
#define SWITCH_MMU_H

#include <unordered_map>
#include <ns3/node.h>
#include "ppp-header.h"
#include "ns3/interface-tag.h"
#include "global-config.h"

namespace ns3 {

class Packet;

class SwitchMmu: public Object {
public:
	static const uint32_t pCnt = P_CNT;	// Number of ports used
	static const uint32_t qCnt = Q_CNT;	// Number of queues/priorities used 不是const，变为了可配置的
	static TypeId GetTypeId (void);

	SwitchMmu(void);

	bool CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	bool CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);

	Ptr<Packet> ConstructFcp(uint32_t port, uint32_t qIndex);

	bool ShouldSendCN(uint32_t ifindex, uint32_t qIndex);

	void ConfigEcn(uint32_t port, uint32_t _kmin, uint32_t _kmax, double _pmax);

	void SetBufferPool(uint64_t b);


	// config
	uint32_t node_id;
	uint32_t kmin[pCnt], kmax[pCnt];
	double pmax[pCnt];

	uint64_t ingress_bytes[pCnt][qCnt]; //vl当前正驻留在交换机的ingress buffer中的字节数
	uint64_t egress_bytes[pCnt][qCnt]; //vl当前正驻留在交换机的egress buffer中的字节数

	uint64_t total_used_buffer; //当前驻留在交换机的所有字节数

	uint32_t fctbs[pCnt][qCnt]; //vl已发送字节数
	uint32_t abr[pCnt][qCnt]; //vl已接收字节数
	uint32_t fccl[pCnt][qCnt]; //上游端口接收的下游端口的remaining buffer + abr
	uint32_t buffer_size[pCnt][qCnt]; //vl可用的buffer大小

	uint32_t total_buffer;

};

} /* namespace ns3 */

#endif /* SWITCH_MMU_H */

