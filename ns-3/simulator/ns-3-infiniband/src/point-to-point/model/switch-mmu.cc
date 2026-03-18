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
#include "point-to-point-net-device.h"
#include "point-to-point-channel.h"
#include "ns3/fcp-header.h"
#include "ns3/ethernet-header.h"
#include "ppp-header.h"
#include "ns3/interface-tag.h"

NS_LOG_COMPONENT_DEFINE("SwitchMmu");

#define OUTPUT_1 0

namespace ns3 {
TypeId SwitchMmu::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::SwitchMmu")
	                    .SetParent<Object>()
	                    .AddConstructor<SwitchMmu>();
	return tid;
}

/*
We model the switch shared memory (purely based on our understanding and experience).
The switch has an on-chip buffer which has `bufferPool` size.
This buffer is shared across all port and queues in the switch.

`bufferPool` is further split into multiple pools at the ingress and egress.

It would be easier to understand from here on if you consider Ingress/Egress are merely just counters.
These are not separate buffer locations or chips...!

First, `ingressPool` (size) accounts for ingress buffering shared by both lossy and lossless traffic.
Additionally, there exists a headroom pool of size xoffTotal,
and each queue may use xoff[port][q] configurable amount at each port p and queue q.
When a queue at the ingress exceeds its ingress threshold, a PFC pause message is sent and
any incoming packets can use upto a maximum of xoff[port][q] headroom.

Second, at the egress, `egressPool[LOSSY]` (size) accounts for buffering lossy traffic at the egress and
similarly `egressPool[LOSSLESS]` for lossless traffic.
*/


SwitchMmu::SwitchMmu(void) {

	// Here we just initialize some default values.
	// The buffer can be configured using Set functions through the simulation file later.

	memset(ingress_bytes, 0, sizeof(ingress_bytes));
	memset(egress_bytes, 0, sizeof(egress_bytes));
	total_used_buffer = 0;
	memset(fctbs, 0, sizeof(fctbs));
	memset(abr, 0, sizeof(abr));
}

bool SwitchMmu::CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize) {
	//new 10/11 by sgh
	if(buffer_size[port][qIndex] - ingress_bytes[port][qIndex] < psize){
		std::cout << "buffer " << buffer_size[port][qIndex] << " occupyed " << ingress_bytes[port][qIndex] << std::endl;
		std::cout << "Buffer overflow, packet size is larger than buffer size!" << std::endl;
		return false;
	}
	else{
		return true;
	}
}

bool SwitchMmu::CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize) {
	return true;
}

void SwitchMmu::UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize) {
	abr[port][qIndex] += psize;
	ingress_bytes[port][qIndex] += psize;
	#if OUTPUT_1
		std::cout << "Time " << Simulator::Now().GetNanoSeconds() << " abr " << abr[port][qIndex] <<
					" remainBS " << buffer_size[port][qIndex] - ingress_bytes[port][qIndex] << 
					" node " << node_id << " port " << port << " qIndex " << qIndex << std::endl;
	#endif
	total_used_buffer += psize; // IMPORTANT: total_used_buffer is only updated in the ingress. No need to update in egress. Avoid double counting.
}

void SwitchMmu::UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize) {
	egress_bytes[port][qIndex] += psize;
}

void SwitchMmu::RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize) {
	// If else are simply unnecessary but its a safety check to avoid magic scenarios (if a packet vanishes in the buffer) where we
	// might assign negative value to unsigned intergers.
	if (ingress_bytes[port][qIndex] >= psize)
		ingress_bytes[port][qIndex] -= psize;
	else
		ingress_bytes[port][qIndex] = 0;

	if (total_used_buffer >= psize) // IMPORTANT: total_used_buffer is only updated in the ingress. No need to update in egress. Avoid double counting.
		total_used_buffer -= psize;
	else
		total_used_buffer = 0;
	#if OUTPUT_1
		std::cout << "Time " << Simulator::Now().GetNanoSeconds() << " abr " << abr[port][qIndex] <<
					" remainBS " << buffer_size[port][qIndex] - ingress_bytes[port][qIndex] << 
					" node " << node_id << " port " << port << " qIndex " << qIndex << std::endl;
	#endif
}

void SwitchMmu::RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize) {
	fctbs[port][qIndex] += psize;
	if (egress_bytes[port][qIndex] >= psize)
		egress_bytes[port][qIndex] -= psize;
	else
		egress_bytes[port][qIndex] = 0;
	//这里对应交换机发送数据包，更新fctbs
	#if OUTPUT_1
		std::cout << "Time " << Simulator::Now().GetNanoSeconds() << " fccl " << fccl[port][qIndex] <<
					" fctbs " << fctbs[port][qIndex] << 
					" node " << node_id << " port " << port << " qIndex " << qIndex << std::endl;
	#endif
	//测量incast的其中一条流在victim topu下左边交换机的buffer情况
	// std::cout << "Time " << Simulator::Now().GetNanoSeconds() 
	// << " egress_bytes " << egress_bytes[port][qIndex]
	// << " node " << node_id 
	// << " port " << port 
	// << " qIndex " << qIndex 
	// << std::endl;
}

//发送credit包
Ptr<Packet> SwitchMmu::ConstructFcp(uint32_t inDev, uint32_t qIndex){
	FcpHeader fcph = FcpHeader();
	fcph.SetVl(qIndex);
	//设置传递的字节数
	uint32_t fccl_ =  abr[inDev][qIndex] + (buffer_size[inDev][qIndex] - ingress_bytes[inDev][qIndex]);
	fcph.SetFccl(fccl_);
	Ptr<Packet> p = Create<Packet> (0);
	p->AddHeader(fcph);
	return p;
}

bool SwitchMmu::ShouldSendCN(uint32_t ifindex, uint32_t qIndex) {
	if (qIndex == 0)
		return false;
	if (egress_bytes[ifindex][qIndex] > kmax[ifindex])
		return true;
	if (egress_bytes[ifindex][qIndex] > kmin[ifindex]) {
		// std::cout << "egress_bytes > kmin" << std::endl;
		double p = pmax[ifindex] * double(egress_bytes[ifindex][qIndex] - kmin[ifindex]) / (kmax[ifindex] - kmin[ifindex]);
		if (UniformVariable(0, 1).GetValue() < p)
			return true;
	}
	return false;
}
void SwitchMmu::ConfigEcn(uint32_t port, uint32_t _kmin, uint32_t _kmax, double _pmax) {
	kmin[port] = _kmin * 1000;
	kmax[port] = _kmax * 1000;
	pmax[port] = _pmax;
}

void SwitchMmu::SetBufferPool(uint64_t b) {
	total_buffer = b;
	uint32_t per_queue_buffer_size = b / pCnt / qCnt;
	for(int p = 0; p < pCnt; p++){
		for(int q = 0; q < qCnt; q++){
			buffer_size[p][q] = per_queue_buffer_size;
			fccl[p][q] = buffer_size[p][q]; //这里本应该是每个port对侧的fccl，但是由于我们仿真中所有链路都设置为相同参数，因此，直接使用这个值即可
		}
	}
}

}
