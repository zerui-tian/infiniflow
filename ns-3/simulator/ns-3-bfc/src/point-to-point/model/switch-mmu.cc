#include <iostream>
#include <fstream>
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/object-vector.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/global-value.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "switch-mmu.h"

NS_LOG_COMPONENT_DEFINE("SwitchMmu");
namespace ns3 {
	TypeId SwitchMmu::GetTypeId(void){
		static TypeId tid = TypeId("ns3::SwitchMmu")
			.SetParent<Object>()
			.AddConstructor<SwitchMmu>();
		return tid;
	}

	SwitchMmu::SwitchMmu(void){
		// headroom
		// shared_used_bytes = 0;
		// memset(hdrm_bytes, 0, sizeof(hdrm_bytes));
		memset(ingress_bytes, 0, sizeof(ingress_bytes));
		memset(paused, 0, sizeof(paused));
		memset(egress_bytes, 0, sizeof(egress_bytes));
		memset(vl_states, 0, sizeof(vl_states));
		memset(pauseCounter, 0, sizeof(pauseCounter));
		memset(active_vl, 0, sizeof(active_vl));
		for(int i = 0;i < SwitchMmu::pCnt; i++) {
			threshold[i] = 2 * 100 * 1000 / 8;
		}

		
	}
	bool SwitchMmu::CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		return psize <= (buffer_size[port][qIndex] - ingress_bytes[port][qIndex]);
	}
	bool SwitchMmu::CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		return true;
	}
	void SwitchMmu::UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		ingress_bytes[port][qIndex] += psize;
	}
	void SwitchMmu::UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		egress_bytes[port][qIndex] += psize;
		// std::cout << "Time " << Simulator::Now().GetNanoSeconds() 
		// << " egress_bytes " << egress_bytes[port][qIndex]
		// << " node " << node_id 
		// << " port " << port 
		// << " qIndex " << qIndex 
		// << std::endl;
	}
	void SwitchMmu::RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		ingress_bytes[port][qIndex] -= psize;
	}
	void SwitchMmu::RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		egress_bytes[port][qIndex] -= psize;
		// std::cout << "Time " << Simulator::Now().GetNanoSeconds() 
		// << " egress_bytes " << egress_bytes[port][qIndex]
		// << " node " << node_id 
		// << " port " << port 
		// << " qIndex " << qIndex 
		// << std::endl;
	}
	bool SwitchMmu::CheckShouldPause(uint32_t port, uint32_t qIndex){
		return !paused[port][qIndex] && ingress_bytes[port][qIndex] >= xoff[port][qIndex];
	}
	bool SwitchMmu::CheckShouldResume(uint32_t port, uint32_t qIndex){
		if (!paused[port][qIndex])
			return false;
		return ingress_bytes[port][qIndex] <= xon[port][qIndex];
	}
	void SwitchMmu::SetPause(uint32_t port, uint32_t qIndex){
		paused[port][qIndex] = true;
	}
	void SwitchMmu::SetResume(uint32_t port, uint32_t qIndex){
		paused[port][qIndex] = false;
	}
	bool SwitchMmu::ShouldSendCN(uint32_t ifindex, uint32_t qIndex){
		if (qIndex == 0)
			return false;
		if (egress_bytes[ifindex][qIndex] > kmax[ifindex])
			return true;
		if (egress_bytes[ifindex][qIndex] > kmin[ifindex]){
			double p = pmax[ifindex] * double(egress_bytes[ifindex][qIndex] - kmin[ifindex]) / (kmax[ifindex] - kmin[ifindex]);
			if (UniformVariable(0, 1).GetValue() < p)
				return true;
		}
		return false;
	}
	void SwitchMmu::ConfigEcn(uint32_t port, double _kmin, double _kmax, double _pmax){
		kmin[port] = _kmin * 1000;
		kmax[port] = _kmax * 1000;
		pmax[port] = _pmax;
	}
	void SwitchMmu::ConfigBufferSize(uint64_t size){
		total_buffer_size = size;
		for(int i = 0; i < pCnt; i++){
			for(int j = 0; j < qCnt; j++){
				buffer_size[i][j] = size / qCnt / pCnt;
			}
		}
	}
	void SwitchMmu::ConfigThreshold(uint32_t port, uint32_t qIndex, uint32_t xon_th, uint32_t xoff_th){
		xon[port][qIndex] = xon_th;
		xoff[port][qIndex] = xoff_th;
	}
	uint32_t SwitchMmu::GetBufferSize(uint32_t port, uint32_t qIndex){
		return buffer_size[port][qIndex];
	}
}