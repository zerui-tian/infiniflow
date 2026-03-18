#include <ns3/hash.h>
#include <ns3/uinteger.h>
#include <ns3/seq-ts-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include <ns3/simulator.h>
#include "ns3/ppp-header.h"
#include "rdma-queue-pair.h"

namespace ns3 {

	/**************************
	 * RdmaTxWorkQueue
	 *************************/
	TypeId RdmaTxWorkQueue::GetTypeId (void) {
		static TypeId tid = TypeId ("ns3::RdmaTxWorkQueue")
							.SetParent<Object>();
		return tid;
	}

	RdmaTxWorkQueue::RdmaTxWorkQueue(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport) {
		startTime = Simulator::Now();
		stopTime = Simulator::GetMaximumSimulationTime();
		sip = _sip;
		dip = _dip;
		sport = _sport;
		dport = _dport;
		m_size = 0;
		snd_nxt = snd_una = 0;
		m_pg = pg;
		m_ipid = 0;
		m_baseRtt = 0;
		m_max_rate = 0;
		m_rate = 0;
		m_nextAvail = Time(0);
	}

	void RdmaTxWorkQueue::SetSize(uint64_t size) {
		m_size = size;
	}

	void RdmaTxWorkQueue::SetBaseRtt(uint64_t baseRtt) {
		m_baseRtt = baseRtt;
	}

	void RdmaTxWorkQueue::SetAppNotifyCallback(Callback<void> notifyAppFinish) {
		m_notifyAppFinish = notifyAppFinish;
	}

	/*
	* Function Description: 获得该QP的剩余未ACKed字节数
	*/
	uint64_t RdmaTxWorkQueue::GetBytesLeft() {
		return m_size >= snd_nxt ? m_size - snd_nxt : 0;
	}

	uint32_t RdmaTxWorkQueue::GetHash(void) {
		union {
			struct {
				uint32_t sip, dip;
				uint16_t sport, dport;
			};
			char c[12];
		} buf;
		buf.sip = sip.Get();
		buf.dip = dip.Get();
		buf.sport = sport;
		buf.dport = dport;
		return Hash32(buf.c, 12);
	}

	/*
	* Function Description: 收到ACK packet后维护窗口指针
	*/
	void RdmaTxWorkQueue::Acknowledge(uint64_t ack_seq) {
		if (ack_seq > snd_una) {
			snd_una = ack_seq;
		}
	}

	/*
	* Function Description: 获得已transmitted但未被ACKed的packet
	*/
	uint64_t RdmaTxWorkQueue::GetOnTheFly() {
		return snd_nxt - snd_una;
	}

	/*
	* Function Description: 判断该QP是否已经完成发送
	*/
	bool RdmaTxWorkQueue::IsFinished() {

		if (Simulator::Now() > stopTime)
			return true;
		else
			return snd_una >= m_size;
	}

	/*********************
	 * RdmaRxWorkQueue
	 ********************/
	TypeId RdmaRxWorkQueue::GetTypeId (void) {
		static TypeId tid = TypeId ("ns3::RdmaRxWorkQueue")
							.SetParent<Object> ()
							;
		return tid;
	}

	RdmaRxWorkQueue::RdmaRxWorkQueue() {
		credit_seq = 0;
		expected_seq = 0;
		lastPktSize = 0;
		m_ipid = 0;
		isFin = false;
		sport = dport = 0;
		
		rtt_ = 0;
		c_recv_next_ = 0;
		
		can_increase_w_ = true;
		credit_size = 68;
		// max_rate = DataRate(50000000000); // 50Gbps
		// alpha_ = 0.8;  
		// max_jitter_ = 0.08;
		// min_jitter_ = 0.01;
		// target_loss_scaling_ = 0.1; // target loss scaling factor
		// w_init_ = 0.6;
		// min_w_ = 0.01; // minimum w
	}

	uint32_t RdmaRxWorkQueue::GetHash(void) {
		union {
			struct {
				uint32_t sip, dip;
				uint16_t sport, dport;
			};
			char c[12];
		} buf;
		buf.sip = sip;
		buf.dip = dip;
		buf.sport = sport;
		buf.dport = dport;
		return Hash32(buf.c, 12);
	}

	void RdmaRxWorkQueue::update_rtt(CustomHeader ch){
		// std::cout << "in update rtt" << std::endl;
		int32_t rtt = (int32_t) Simulator::Now().GetNanoSeconds() - ch.cdt.credit_sent_time;
		// std::cout << "rtt: " << rtt << std::endl;
		if (rtt_ > 0) {
			rtt_ = 0.8*rtt_ + 0.2*rtt;
		}else {
			rtt_ = rtt;
  		}
		// std::cout << "rtt_ : " << rtt_ << std::endl;
	}

	void RdmaRxWorkQueue::credit_feedback_control(){
		// std::cout << "in credit feedback control" << std::endl;
		if (rtt_ <= 0) {
			// std::cout << "rtt_ <= 0, return" << std::endl;
			return;
		}
		if ((Simulator::Now().GetNanoSeconds() - last_credit_rate_update_.GetNanoSeconds()) < rtt_) {
			// std::cout << "not enough time passed, return" << std::endl;
			return;
		}
		if (credit_total_ == 0) {
			// std::cout << "credit_total_ == 0, return" << std::endl;
			return;
		}

		DataRate old_rate = m_rate;

		double loss_rate = credit_dropped_/(double)credit_total_;
		// std::cout << "loss_rate: " << loss_rate << std::endl;
		double target_loss = (1.0 - (double) m_rate.GetBitRate() / max_rate.GetBitRate()) * target_loss_scaling_;
		// std::cout << "target_loss: " << target_loss << std::endl;
		int min_rate = (int)(credit_size * 8 * 1e9 / rtt_);
		// std::cout << "min_rate: " << min_rate << std::endl;
		if (loss_rate > target_loss) {
			// congestion has been detected!
			if (loss_rate >= 1.0) {
				m_rate = (int)(credit_size * 8 * 1e9 / rtt_);
				// std::cout << "after congestion 1, m_rate: " << m_rate << std::endl;
			} 
			else {
				m_rate = (int)(credit_size * (credit_total_ - credit_dropped_) * 8 * 1e9
								/ (Simulator::Now().GetNanoSeconds() - last_credit_rate_update_.GetNanoSeconds())
								* (1.0 + target_loss));
				// std::cout << "after congestion 2, m_rate: " << m_rate << std::endl;
			}
			if (m_rate > old_rate) {
				m_rate = old_rate;
			}
			w_ = std::max(w_/2.0, min_w_);
			can_increase_w_ = false;
		}else {
			// there is no congestion.
			if (can_increase_w_) {
				w_ = std::min(w_ + 0.05, 0.5);
			}else {
				can_increase_w_ = true;
			}
			if (m_rate < max_rate) {
				m_rate = (w_ * max_rate + (1 - w_) * m_rate);
			}
		}

		if (m_rate > max_rate) {
			m_rate = max_rate;
		}
		if (m_rate < min_rate) {
			m_rate = min_rate;
		}
		credit_total_ = 0;
		credit_dropped_ = 0;
		last_credit_rate_update_ = Simulator::Now();
		// std::cout << "update time:" << last_credit_rate_update_ << " m_rate : " << m_rate.GetBitRate() << std::endl; 
	}

	// Ptr<Packet> RdmaRxWorkQueue::GetNxtCredit() {
	// 	Ptr<Packet> p;
		
	// }

	/*********************
	 * RdmaQueuePairGroup
	 ********************/
	TypeId RdmaQueuePairGroup::GetTypeId (void) {
		static TypeId tid = TypeId ("ns3::RdmaQueuePairGroup")
							.SetParent<Object> ();
		return tid;
	}

	RdmaQueuePairGroup::RdmaQueuePairGroup(void) {
	}

	uint32_t RdmaQueuePairGroup::GetN(void) {
		return m_qps.size();
	}

	Ptr<RdmaTxWorkQueue> RdmaQueuePairGroup::GetQp(uint32_t idx) {
		return m_qps[idx];
	}
	Ptr<RdmaRxWorkQueue> RdmaQueuePairGroup::GetCreditQp(uint32_t idx) {
		return c_qps[idx];
	}
	
	void RdmaQueuePairGroup::AddQp(Ptr<RdmaTxWorkQueue> qp) {
		m_qps.push_back(qp);
	}
	void RdmaQueuePairGroup::AddCreditQp(Ptr<RdmaRxWorkQueue> qp) {
		c_qps.push_back(qp);
	}

	void RdmaQueuePairGroup::Clear(void) {
		m_qps.clear();
		c_qps.clear();
	}

}