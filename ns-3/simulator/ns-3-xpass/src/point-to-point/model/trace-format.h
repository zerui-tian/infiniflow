#ifndef TRACE_FORMAT_H
#define TRACE_FORMAT_H
#include <stdint.h>
#include <cstdio>
#include <cassert>
#include <vector>
#include <stddef.h>
#include <cstring>

namespace ns3{

enum PEvent{
	Recv = 0,
	Enqu = 1,
	Dequ = 2,
	Drop = 3
};

struct TraceFormat{
	uint64_t time;
	uint16_t node;
	uint8_t intf, qidx;
	uint32_t qlen;
	uint32_t sip, dip;
	uint16_t size;
	uint8_t l3Prot;
	uint8_t event;
	uint8_t ecn; // this is the ip ECN bits
	uint8_t nodeType; // 0: host, 1: switch
	union{
		struct {
			//tcp
			uint16_t sport, dport;
			uint16_t flags;
			uint16_t pg;
			uint32_t m_seq; // the qbb sequence number.
			uint32_t m_rtt;
	
			uint32_t recv_next;
			uint8_t mtype; //表示所发消息类型0x01表示request,0x02表示credit,0x03表示data
	
			//cmnh
			uint8_t ptype; //表示状态close,sending等
	
			//xph
			uint32_t credit_seq;
			uint32_t credit_sent_time;
			uint32_t sendbuffer;
		} cdt;
	};

	void Serialize(FILE *file){
		fwrite(this, sizeof(TraceFormat), 1, file);
	}
	int Deserialize(FILE *file){
		int ret = fread(this, sizeof(TraceFormat), 1, file);
		return ret;
	}
};

static inline const char* EventToStr(enum PEvent e){
	switch (e){
		case Recv:
			return "Recv";
		case Enqu:
			return "Enqu";
		case Dequ:
			return "Dequ";
		case Drop:
			return "Drop";
		default:
			return "????";
	}
}

}
#endif
