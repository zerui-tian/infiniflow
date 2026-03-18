#ifndef SWITCH_MMU_H
#define SWITCH_MMU_H

#include <unordered_map>
#include <ns3/node.h>
#include "global-config.h"

namespace ns3 {

class Packet;

class SwitchMmu: public Object{
public:
	static const uint32_t pCnt = P_CNT;	// Number of ports used
	static const uint32_t qCnt = Q_CNT;	// Number of queues/priorities used

	static TypeId GetTypeId (void);

	SwitchMmu(void);

	bool CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	bool CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);
	void RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize);

	bool CheckShouldPause(uint32_t port, uint32_t qIndex);
	bool CheckShouldResume(uint32_t port, uint32_t qIndex);
	void SetPause(uint32_t port, uint32_t qIndex);
	void SetResume(uint32_t port, uint32_t qIndex);

	bool ShouldSendCN(uint32_t ifindex, uint32_t qIndex);

	void ConfigEcn(uint32_t port, double _kmin, double _kmax, double _pmax);
	void ConfigBufferSize(uint64_t size);
	void ConfigThreshold(uint32_t port, uint32_t qIndex, uint32_t xon_th, uint32_t xoff_th);

	uint32_t GetBufferSize(uint32_t port, uint32_t qIndex);

	// config
	uint32_t node_id;
	uint64_t total_buffer_size;
	uint32_t buffer_size[pCnt][qCnt];
	uint32_t kmin[pCnt], kmax[pCnt];
	double pmax[pCnt];

	// runtime
	uint32_t ingress_bytes[pCnt][qCnt];
	uint32_t paused[pCnt][qCnt];
	uint32_t egress_bytes[pCnt][qCnt];

	u_int32_t xon[pCnt][qCnt];
	u_int32_t xoff[pCnt][qCnt];

	std::map<uint32_t,uint32_t> bytes_in_switch;//flow bytes in switch 
	uint32_t vl_states[pCnt][qCnt]; 
	std::map<uint32_t,uint32_t> flow_qIndex_map; // flowid -> qindex
	uint32_t pauseCounter[pCnt][qCnt];
	uint32_t threshold[pCnt];
	uint16_t active_vl[pCnt];
};

} /* namespace ns3 */

#endif /* SWITCH_MMU_H */