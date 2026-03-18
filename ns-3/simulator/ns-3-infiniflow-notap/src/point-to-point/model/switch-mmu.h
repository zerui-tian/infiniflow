#ifndef SWITCH_MMU_H
#define SWITCH_MMU_H

#include <unordered_map>
#include <ns3/node.h>
#include "ppp-header.h"
#include "ns3/interface-tag.h"
#include "ns3/global-config.h"

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

	void SetInflightThreshold(u_int32_t port, uint32_t qIndex, uint32_t threshold);
	void SetQlenMax(u_int32_t value);
	void SetQlenMin(u_int32_t value);
	void SetAiStep(u_int32_t value);

	// config
	uint32_t node_id;
	uint32_t kmin[pCnt], kmax[pCnt];
	double pmax[pCnt];

	uint64_t ingress_bytes[pCnt][qCnt]; 
	uint64_t egress_bytes[pCnt][qCnt]; 
	uint32_t accumu_used_credit[pCnt][qCnt]; 
	uint32_t accumu_recycled_credit[pCnt][qCnt]; 
	uint32_t accumu_drained_bytes[pCnt][qCnt]; 

	uint32_t inflight_threshold[pCnt][qCnt];
	uint32_t maxThreshold;

	bool need_set_tap[pCnt][qCnt];
	bool need_ret_tap[pCnt][qCnt];
	bool rate_adjusting[pCnt][qCnt];

	uint64_t total_used_buffer; 

	uint32_t fctbs[pCnt];
	uint32_t abr[pCnt]; 
	uint32_t fccl[pCnt];
	uint32_t buffer_size[pCnt];
	uint32_t used_bytes[pCnt]; 

	uint32_t total_buffer;

	uint32_t qlen_max;
	uint32_t qlen_min;
	uint32_t ai_step;
};

} /* namespace ns3 */

#endif /* SWITCH_MMU_H */

