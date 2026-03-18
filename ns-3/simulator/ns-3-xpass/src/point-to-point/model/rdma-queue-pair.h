#ifndef RDMA_QUEUE_PAIR_H
#define RDMA_QUEUE_PAIR_H

#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/ipv4-address.h>
#include <ns3/data-rate.h>
#include <ns3/event-id.h>
#include <ns3/custom-header.h>
#include <ns3/int-header.h>
#include <vector>
#include <map>

namespace ns3 {
	//代表data和request
	class RdmaTxWorkQueue : public Object {
		public:
			Time startTime;						// QP创建时间
			Time stopTime;						// QP完成时间
			Ipv4Address sip, dip;				// 源IP、目的IP
			uint16_t sport, dport;				// 源端口、目的端口
			uint16_t m_pg;						// priority group，即优先级信息
			uint64_t m_size;					// QP将要发送的字节数
			uint64_t snd_nxt, snd_una;			// 发送窗口指针，分别是：next seq to send, the highest unacked seq
			uint16_t m_ipid;
			uint64_t m_baseRtt;					// QP的base RTT
			DataRate m_rate;					// 当前发送rate
			DataRate m_max_rate;				// QP的最大injection rate
			Time m_nextAvail;					// 最近的下一次sending time
			uint32_t lastPktSize;				// 最近一次发送的packet size
			Callback<void> m_notifyAppFinish;	// callback函数：告知L5发送完成，在该仿真中，指向rdma-client.cc的Finish()函数
			uint32_t incastFlow;
			uint32_t c_next_credit;             //下一次接收到的credit序号
			// 暂留变量
			// uint32_t m_win;
			// bool m_var_win; // variable window size

			/***********
			 * methods
			 **********/
			//2/27 by sgh
			uint64_t numTxBytes = 0;
			uint64_t getFlowTxBytes(){
				uint64_t temp;
				temp = numTxBytes;
				numTxBytes = 0;
				return temp;
			}
			static TypeId GetTypeId (void);
			RdmaTxWorkQueue(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport);
			void SetSize(uint64_t size);
			void SetBaseRtt(uint64_t baseRtt);
			void SetAppNotifyCallback(Callback<void> notifyAppFinish);

			uint64_t GetBytesLeft();
			uint32_t GetHash(void);
			uint64_t GetOnTheFly();

			bool IsFinished();
			void Acknowledge(uint64_t ack);
	};
	//代表credit
	class RdmaRxWorkQueue : public Object {
		public:
			uint32_t sip, dip;					// 源IP、目的IP
			uint16_t sport, dport;				// 源端口、目的端口
			uint16_t m_ipid;
			DataRate m_rate;					// 当前发送rate
			DataRate max_rate;			// corresponding to the link capacity
			uint32_t expected_seq;				// 接收窗口指针，期待的下一个seq
			uint32_t credit_seq = 0;  // Initialize to 0
			uint32_t lastPktSize;				// 最近一次发送的packet size
			uint16_t c_pg; //c_pg = m_pg
			bool isFin; //表示是否结束，在接收到接收方fin时更改
			double alpha_; //调速参数，确定其初始的速率为最大速率的多少，应小于等于1

			// It shows that credit buffer size of eight is sufficient across the different number of flows.
			//需要加上其credit buffer size
			Time c_nextAvail; // credit最近的下一次sending time
			//用于端系统调速
			uint32_t c_recv_next_; //credit下一个准备接收的credit的序号
			// total number of credit = # credit received + # credit dropped.
			uint32_t credit_total_;
			// number of credit dropped.
			uint32_t credit_dropped_;
			// weighted-average round trip time,use ns,not s.
			int rtt_;
			Time last_credit_rate_update_;
			int credit_send_time;
			// target loss scaling factor.we use a small target loss rate of 10%.
  			// target loss = (1 - cur_credit_rate/max_credit_rate)*target_loss_scaling.
			double target_loss_scaling_;
			
			uint16_t credit_size;
			// aggressiveness factor
			// it determines how aggressively increase the credit sending rate. 0 < w <= 0.5
			/*
			when congestion is detected, we halve w in the decrease phase. When no
			congestion is detected for two update periods, we increase w by
			averaging its current value and the maximum value, 0.5.
			At steady state, a flow experiences increase and decrease phase alternatively,
			and as a result, w decreases exponentially.
			In all our experiments, we use wmin of 0.01.
			*/
			double w_;
			// initial value of w_
			double w_init_; //设置为0.5
			// minimum value of w_
			double min_w_; //设置为0.01，最大max_w_是0.5
			// whether feedback control can increase w or not.
			bool can_increase_w_;
			//We vary the jitter level, j, from 0.01 to 0.08 relative to the inter-credit gap
			// maximum jitter: -1.0 ~ 1.0 (wrt. inter-credit gap)
			double max_jitter_ = 0.08;
			// minimum jitter: -1.0 ~ 1.0 (wrt. inter-credit gap)
			double min_jitter_ = 0.01;
			static TypeId GetTypeId (void);
			RdmaRxWorkQueue();
			uint32_t GetHash(void);
			Ptr<Packet> GetNxtCredit();

			void update_rtt(CustomHeader ch);
			void credit_feedback_control();
	};

	class RdmaQueuePairGroup : public Object {
		public:
			std::vector<Ptr<RdmaTxWorkQueue> > m_qps;
			std::vector<Ptr<RdmaRxWorkQueue> > c_qps;//xpass by sgh

			static TypeId GetTypeId (void);
			RdmaQueuePairGroup(void);
			uint32_t GetN(void);
			uint32_t GetCreditN(void){
				return c_qps.size();
			};
			Ptr<RdmaTxWorkQueue> GetQp(uint32_t idx);
			Ptr<RdmaRxWorkQueue> GetCreditQp(uint32_t idx);
 			void AddQp(Ptr<RdmaTxWorkQueue> qp);
			void AddCreditQp(Ptr<RdmaRxWorkQueue> qp);
			void Clear(void);
	};
}

#endif /* RDMA_QUEUE_PAIR_H */
