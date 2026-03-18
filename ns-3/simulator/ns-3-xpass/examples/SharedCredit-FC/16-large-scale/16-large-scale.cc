/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#undef PGO_TRAINING
#define PATH_TO_PGO_CONFIG "path_to_pgo_config"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <time.h>
#include "ns3/core-module.h"
#include "ns3/qbb-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/packet.h"
#include "ns3/error-model.h"
#include <ns3/rdma.h>
#include <ns3/rdma-client.h>
#include <ns3/rdma-client-helper.h>
#include <ns3/rdma-driver.h>
#include <ns3/switch-node.h>
#include <ns3/sim-setting.h>
#include <ns3/switch-node.h>
#include "ns3/rdma-queue-pair.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("GENERIC_SIMULATION");

uint32_t packet_payload_size = 1000;
double pause_time = 5, simulator_stop_time = 1.01;
uint32_t shaping_pkt_size = 1000;

double alpha = 0.25;
double kp_nic = 50;
double kp_lc_i = 50;
double kp_lc_e = 50;

uint64_t max_credit_rate;
double alpha_;
double max_jitter_;
double min_jitter_;
double target_loss_scaling_; 
double w_init_;
double min_w_;

uint64_t max_tokens_;
uint64_t token_refresh_rate_;

std::string topology_file, flow_file;
std::string fct_output_file;
std::string pfc_output_file;

std::string throughput_output_file;
std::string qlen_output_file;
std::string qlen_dist_output_file;
std::string flowrate_output_file;

uint32_t qlen_dump_interval;
uint32_t qlen_mon_interval;
uint32_t qlen_mon_start;
uint32_t qlen_mon_end;

uint32_t throughput_dump_interval;
uint32_t throughput_mon_interval;
uint32_t throughput_mon_start;
uint32_t throughput_mon_end;

uint32_t enable_trace = 1;

uint32_t buffer_size = 40;

/************************************************
 * Runtime varibles
 ***********************************************/
std::ifstream topof, flowf, tracef;

NodeContainer n;
NetDeviceContainer nd;

NetDeviceContainer switchToSwitchInterfaces;
std::map< uint32_t, std::map< uint32_t, std::vector<Ptr<QbbNetDevice>> > > switchToSwitch;

// vamsi
std::map<uint32_t, uint32_t> switchNumToId;
std::map<uint32_t, uint32_t> switchIdToNum;
std::map<uint32_t, NetDeviceContainer> switchUp;
std::map<uint32_t, NetDeviceContainer> switchDown;
//NetDeviceContainer switchUp[switch_num];
std::map<uint32_t, NetDeviceContainer> sourceNodes;

NodeContainer servers;
NodeContainer tors;

uint64_t nic_rate;

uint64_t maxRtt, maxBdp;

struct Interface {
	uint32_t idx;
	bool up;
	uint64_t delay;
	uint64_t bw;

	Interface() : idx(0), up(false) {}
};
map<Ptr<Node>, map<Ptr<Node>, Interface> > nbr2if;
// Mapping destination to next hop for each node: <node, <dest, <nexthop0, ...> > >
map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node> > > > nextHop;
map<Ptr<Node>, map<Ptr<Node>, uint64_t> > pairDelay;
map<Ptr<Node>, map<Ptr<Node>, uint64_t> > pairTxDelay;
map<uint32_t, map<uint32_t, uint64_t> > pairBw;
map<Ptr<Node>, map<Ptr<Node>, uint64_t> > pairBdp;
map<uint32_t, map<uint32_t, uint64_t> > pairRtt;

std::vector<Ipv4Address> serverAddress;

// maintain port number for each host pair
std::unordered_map<uint32_t, unordered_map<uint32_t, uint16_t> > portNumder;

struct FlowInput {
	uint64_t src, dst, pg, maxPacketCount, port, dport;
	double start_time;
	uint32_t idx;
};
FlowInput flow_input = {0};
uint32_t flow_num;

void ReadFlowInput() {
	if (flow_input.idx < flow_num) {
		flowf >> flow_input.src >> flow_input.dst >> flow_input.pg >> flow_input.dport >> flow_input.maxPacketCount >> flow_input.start_time;
		std::cout << "Flow " << flow_input.src << " " << flow_input.dst << " " << flow_input.pg << " " << flow_input.dport << " " << flow_input.maxPacketCount << " " << flow_input.start_time << " " << Simulator::Now().GetSeconds() << std::endl;
		NS_ASSERT(n.Get(flow_input.src)->GetNodeType() == 0 && n.Get(flow_input.dst)->GetNodeType() == 0);
	}
}
void ScheduleFlowInputs() {
	while (flow_input.idx < flow_num && Seconds(flow_input.start_time) <= Simulator::Now()) {
		uint32_t port = portNumder[flow_input.src][flow_input.dst]++; // get a new port number
		RdmaClientHelper clientHelper(flow_input.pg, serverAddress[flow_input.src], serverAddress[flow_input.dst], port, flow_input.dport, flow_input.maxPacketCount, 0, 0, Simulator::GetMaximumSimulationTime());
		ApplicationContainer appCon = clientHelper.Install(n.Get(flow_input.src));
//		appCon.Start(Seconds(flow_input.start_time));
		appCon.Start(Seconds(0)); // setting the correct time here conflicts with Sim time since there is already a schedule event that triggered this function at desired time.
		// get the next flow input
		flow_input.idx++;
		ReadFlowInput();
	}

	// schedule the next time to run this function
	if (flow_input.idx < flow_num) {
		Simulator::Schedule(Seconds(flow_input.start_time) - Simulator::Now(), ScheduleFlowInputs);
	} else { // no more flows, close the file
		flowf.close();
	}
}

Ipv4Address node_id_to_ip(uint32_t id) {
	return Ipv4Address(0x0b000001 + ((id / 256) * 0x00010000) + ((id % 256) * 0x00000100));
}

uint32_t ip_to_node_id(Ipv4Address ip) {
	return (ip.Get() >> 8) & 0xffff;
}

void qp_finish(FILE* fout, Ptr<RdmaTxWorkQueue> q) {
	uint32_t sid = ip_to_node_id(q->sip), did = ip_to_node_id(q->dip);
	uint64_t base_rtt = pairRtt[sid][did], b = pairBw[sid][did];
	uint32_t total_bytes = q->m_size + ((q->m_size - 1) / packet_payload_size + 1) * (CustomHeader::GetStaticWholeHeaderSize() - IntHeader::GetStaticSize()); // translate to the minimum bytes required (with header but no INT)
	uint64_t standalone_fct = base_rtt + total_bytes * 8000000000lu / b;
	// sip, dip, sport, dport, size (B), start_time, fct (ns), standalone_fct (ns)
	fprintf(fout, "%08x %08x %u %u %lu %lu %lu %lu\n", q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->m_size, q->startTime.GetTimeStep(), (Simulator::Now() - q->startTime).GetTimeStep(), standalone_fct);
	fflush(fout);

	// remove rxQp from the receiver
	Ptr<Node> dstNode = n.Get(did);
	Ptr<RdmaDriver> rdma = dstNode->GetObject<RdmaDriver> ();
	rdma->m_rdma->DeleteRxQp(q->sip.Get(), q->m_pg, q->sport);
}

// 
typedef struct {
	int64_t time_ns;
	uint16_t sw;
	double qlen;
} QlengthEntry;
std::vector<QlengthEntry> qlength_waveform;
void monitor_buffer(FILE* qlen_output, NodeContainer *n) {
	for (uint32_t i = 0; i < n->GetN(); i++) {
		if (n->Get(i)->GetNodeType() == 1) { // is switch
			Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n->Get(i));
			double size = 0;
			for (uint32_t j = 1; j < sw->GetNDevices(); j++) {
				for (uint32_t k = 0; k < SwitchMmu::qCnt; k++)
					size += sw->m_mmu->egress_bytes[j][k];
			}
			QlengthEntry new_item;
			new_item.time_ns = Simulator::Now().GetNanoSeconds();
			new_item.sw = sw->GetId();
			new_item.qlen = (double)size / 1000;
			qlength_waveform.push_back(new_item);
		}
	}
	if (Simulator::Now().GetTimeStep() % qlen_dump_interval == 0) {
		for (auto i = qlength_waveform.begin(); i != qlength_waveform.end(); ++i){
			fprintf(qlen_output, "%u %.2f %u\n", (*i).sw, ((*i).qlen), (*i).time_ns);
			fflush(qlen_output);
		}
		qlength_waveform.clear();
	}
	if (Simulator::Now().GetTimeStep() < qlen_mon_end)
		Simulator::Schedule(NanoSeconds(qlen_mon_interval), &monitor_buffer, qlen_output, n);
}


// struct QlenDistribution {
// 	vector<uint32_t> cnt; // cnt[i] is the number of times that the queue len is i KB

// 	void add(uint32_t qlen) {
// 		uint32_t kb = qlen / 1000;
// 		if (cnt.size() < kb + 1)
// 			cnt.resize(kb + 1);
// 		cnt[kb]++;

// 	}
// };

void CalculateRoute(Ptr<Node> host) {
	// queue for the BFS.
	vector<Ptr<Node> > q;
	// Distance from the host to each node.
	map<Ptr<Node>, int> dis;
	map<Ptr<Node>, uint64_t> delay;
	map<Ptr<Node>, uint64_t> txDelay;
	map<Ptr<Node>, uint64_t> bw;
	// init BFS.
	q.push_back(host);
	dis[host] = 0;
	delay[host] = 0;
	txDelay[host] = 0;
	bw[host] = 0xfffffffffffffffflu;
	// BFS.
	for (int i = 0; i < (int)q.size(); i++) {
		Ptr<Node> now = q[i];
		int d = dis[now];
		for (auto it = nbr2if[now].begin(); it != nbr2if[now].end(); it++) {
			// skip down link
			if (!it->second.up)
				continue;
			Ptr<Node> next = it->first;
			// If 'next' have not been visited.
			if (dis.find(next) == dis.end()) {
				dis[next] = d + 1;
				delay[next] = delay[now] + it->second.delay;
				txDelay[next] = txDelay[now] + packet_payload_size * 1000000000lu * 8 / it->second.bw;
				bw[next] = std::min(bw[now], it->second.bw);
				// we only enqueue switch, because we do not want packets to go through host as middle point
				if (next->GetNodeType())
					q.push_back(next);
			}
			// if 'now' is on the shortest path from 'next' to 'host'.
			if (d + 1 == dis[next]) {
				nextHop[next][host].push_back(now);
			}
		}
	}
	for (auto it : delay)
		pairDelay[it.first][host] = it.second;
	for (auto it : txDelay)
		pairTxDelay[it.first][host] = it.second;
	for (auto it : bw)
		pairBw[it.first->GetId()][host->GetId()] = it.second;
}

void CalculateRoutes(NodeContainer &n) {
	for (int i = 0; i < (int)n.GetN(); i++) {
		Ptr<Node> node = n.Get(i);
		if (node->GetNodeType() == 0)
			CalculateRoute(node);
	}
}

void SetRoutingEntries() {
	// For each node.
	for (auto i = nextHop.begin(); i != nextHop.end(); i++) {
		Ptr<Node> node = i->first;
		auto &table = i->second;
		for (auto j = table.begin(); j != table.end(); j++) {
			// The destination node.
			Ptr<Node> dst = j->first;
			// The IP address of the dst.
			Ipv4Address dstAddr = dst->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
			// The next hops towards the dst.
			vector<Ptr<Node> > nexts = j->second;
			for (int k = 0; k < (int)nexts.size(); k++) {
				Ptr<Node> next = nexts[k];
				uint32_t interface = nbr2if[node][next].idx;
				if (node->GetNodeType())
					DynamicCast<SwitchNode>(node)->AddTableEntry(dstAddr, interface);
				else {
					node->GetObject<RdmaDriver>()->m_rdma->AddTableEntry(dstAddr, interface);
				}
			}
		}
	}
}

// take down the link between a and b, and redo the routing
void TakeDownLink(NodeContainer n, Ptr<Node> a, Ptr<Node> b) {
	if (!nbr2if[a][b].up)
		return;
	// take down link between a and b
	nbr2if[a][b].up = nbr2if[b][a].up = false;
	nextHop.clear();
	CalculateRoutes(n);
	// clear routing tables
	for (uint32_t i = 0; i < n.GetN(); i++) {
		if (n.Get(i)->GetNodeType() == 1)
			DynamicCast<SwitchNode>(n.Get(i))->ClearTable();
		else
			n.Get(i)->GetObject<RdmaDriver>()->m_rdma->ClearTable();
	}
	DynamicCast<QbbNetDevice>(a->GetDevice(nbr2if[a][b].idx))->TakeDown();
	DynamicCast<QbbNetDevice>(b->GetDevice(nbr2if[b][a].idx))->TakeDown();
	// reset routing table
	SetRoutingEntries();

	// redistribute qp on each host
	for (uint32_t i = 0; i < n.GetN(); i++) {
		if (n.Get(i)->GetNodeType() == 0)
			n.Get(i)->GetObject<RdmaDriver>()->m_rdma->RedistributeQp();
	}
}

uint64_t get_nic_rate(NodeContainer &n) {
	for (uint32_t i = 0; i < n.GetN(); i++)
		if (n.Get(i)->GetNodeType() == 0)
			return DynamicCast<QbbNetDevice>(n.Get(i)->GetDevice(1))->GetDataRate().GetBitRate();
}

void PrintResults(std::map<uint32_t, NetDeviceContainer> ToR, uint32_t numToRs, double delay) {
	for (uint32_t i = 0; i < numToRs; i++) {
		double throughputTotal = 0;
		uint64_t torBuffer = 0;
		double power;
		for (uint32_t j = 0; j < ToR[i].GetN(); j++) {
			Ptr<QbbNetDevice> nd = DynamicCast<QbbNetDevice>(ToR[i].Get(j));
//			uint64_t txBytes = nd->getTxBytes();
			uint64_t txBytes = nd->GetQueue()->getTxBytes();
			double rxBytes = nd->getNumRxBytes();

			uint64_t qlen = nd->GetQueue()->GetNBytesTotal();
			uint64_t bw = nd->GetDataRate().GetBitRate(); //maxRtt

			torBuffer += qlen;
			double throughput = double(txBytes * 8) / delay;
			if (j == 16) { //  ToDo. very ugly hardcode here specific to the burst evaluation scenario where 16 is the receiver in flow-burstExp.txt.
				throughputTotal += throughput;
				power = (rxBytes * 8.0 / delay) * (qlen + bw * maxRtt * 1e-9) / (bw * (bw * maxRtt * 1e-9));

			}
			// std::cout << "ToR " << i << " Port " << j << " throughput " << throughput << " txBytes " << txBytes << " qlen " << qlen << " time " << Simulator::Now().GetSeconds() << " normpower " << power << std::endl;
		}
		// std::cout << "ToR " << i << " Total " << 0 << " throughput " << throughputTotal << " buffer " << torBuffer <<  " time " << Simulator::Now().GetSeconds() << std::endl;
	}
	Simulator::Schedule(Seconds(delay), PrintResults, ToR, numToRs, delay);
}


void PrintResultsFlow(std::map<uint32_t, NetDeviceContainer> Src, uint32_t numFlows, double delay) {
	for (uint32_t i = 0; i < numFlows; i++) {
		double throughputTotal = 0;

		for (uint32_t j = 0; j < Src[i].GetN(); j++) {
			Ptr<QbbNetDevice> nd = DynamicCast<QbbNetDevice>(Src[i].Get(j));
//			uint64_t txBytes = nd->getTxBytes();
			uint64_t txBytes = nd->getNumTxBytes();

			uint64_t qlen = nd->GetQueue()->GetNBytesTotal();
			double throughput = double(txBytes * 8) / delay;
			throughputTotal += throughput;
			// std::cout << "Src " << i << " Port " << j << " throughput "<< throughput << " txBytes " << txBytes << " qlen " << qlen << " time " << Simulator::Now().GetSeconds() << std::endl;
		}
		// std::cout << "Src " << i << " Total " << 0 << " throughput " << throughputTotal <<  " time " << Simulator::Now().GetSeconds() << std::endl;
	}
	Simulator::Schedule(Seconds(delay), PrintResultsFlow, Src, numFlows, delay);
}




int main(int argc, char *argv[])
{
	clock_t begint, endt;
	begint = clock();
	std::ifstream conf;
	
	std::string confFile;

	CommandLine cmd;
	cmd.AddValue("conf", "config file path", confFile);

	cmd.Parse (argc, argv);
	conf.open(confFile.c_str());
	while (!conf.eof())
	{
		std::string key;
		conf >> key;
		if (key.compare("PAUSE_TIME") == 0)
		{
			double v;
			conf >> v;
			pause_time = v;
			std::cout << "PAUSE_TIME\t\t\t" << pause_time << "\n";
		} else if (key.compare("SHAPING_PKT_SIZE") == 0){
			conf >> shaping_pkt_size;
			std::cout << "SHAPING_PKT_SIZE\t\t\t" << shaping_pkt_size << "\n";

		} else if (key.compare("ALPHA") == 0){
			conf >> alpha;
			std::cout << "ALPHA\t\t\t" << alpha << "\n";
		} else if (key.compare("KP_NIC") == 0){
			conf >> kp_nic;
			std::cout << "ALPHA\t\t\t" << kp_nic << "\n";
		} else if (key.compare("KP_LC_I") == 0){
			conf >> kp_lc_i;
			std::cout << "KP_LC_I\t\t\t" << kp_lc_i << "\n";
		} else if (key.compare("KP_LC_E") == 0){
			conf >> kp_lc_e;
			std::cout << "KP_LC_E\t\t\t" << kp_lc_e << "\n";

		} else if (key.compare("MAX_CREDIT_RATE") == 0) {
			conf >> max_credit_rate;
			std::cout << "MAX_CREDIT_RATE\t\t\t\t" << max_credit_rate << '\n';
		} else if (key.compare("ALPHA_") == 0) {
			conf >> alpha_;
			std::cout << "ALPHA_\t\t\t\t\t" << alpha_ << '\n';
		} else if (key.compare("MAX_JITTER_") == 0) {
			conf >> max_jitter_;
			std::cout << "MAX_JITTER_\t\t\t\t" << max_jitter_ << '\n';
		} else if (key.compare("MIN_JITTER_") == 0) {
			conf >> min_jitter_;
			std::cout << "MIN_JITTER_\t\t\t\t" << min_jitter_ << '\n';
		} else if (key.compare("TARGET_LOSS_SCALING_") == 0) {
			conf >> target_loss_scaling_;
			std::cout << "TARGET_LOSS_SCALING_\t\t\t" << target_loss_scaling_ << '\n';
		} else if (key.compare("W_INIT_") == 0) {
			conf >> w_init_;
			std::cout << "W_INIT_\t\t\t\t\t" << w_init_ << '\n';
		} else if (key.compare("MIN_W_") == 0) {
			conf >> min_w_;
			std::cout << "MIN_W_\t\t\t\t\t" << min_w_ << '\n';
		
		} else if (key.compare("MAX_TOKENS") == 0) {
			conf >> max_tokens_;
			std::cout << "MAX_TOKENS\t\t\t\t" << max_tokens_ << '\n';
		}
		else if (key.compare("TOKEN_REFRESH_RATE_") == 0) {
			conf >> token_refresh_rate_;
			std::cout << "TOKEN_REFRESH_RATE_\t\t\t" << token_refresh_rate_ << '\n';


		} else if (key.compare("FLOW_FILE") == 0) {
			conf >> flow_file;
			std::cout << "FLOW_FILE\t\t\t" << flow_file << "\n";
		} else if (key.compare("TOPOLOGY_FILE") == 0) {
			conf >> topology_file;
			std::cout << "TOPOLOGY_FILE\t\t\t" << topology_file << "\n";
		}
		else if (key.compare("FLOW_FILE") == 0)
		{
			std::string v;
			conf >> v;
			flow_file = v;
			std::cout << "FLOW_FILE\t\t\t" << flow_file << "\n";
		}
		else if (key.compare("SIMULATOR_STOP_TIME") == 0) {
			double v;
			conf >> v;
			simulator_stop_time = v;
			std::cout << "SIMULATOR_STOP_TIME\t\t" << simulator_stop_time << "\n";
		} else if (key.compare("FCT_OUTPUT_FILE") == 0) {
			conf >> fct_output_file;
			std::cout << "FCT_OUTPUT_FILE\t\t" << fct_output_file << '\n';
		} else if (key.compare("PFC_OUTPUT_FILE") == 0) {
			conf >> pfc_output_file;
			std::cout << "PFC_OUTPUT_FILE\t\t\t" << pfc_output_file << '\n';

		} else if (key.compare("BUFFER_SIZE") == 0) {
			conf >> buffer_size;
			std::cout << "BUFFER_SIZE\t\t\t\t" << buffer_size << '\n';
			} else if (key.compare("QLEN_DUMP_INTERVAL") == 0) {
			conf >> qlen_dump_interval;
			std::cout << "QLEN_DUMP_INTERVAL\t\t" << qlen_dump_interval << "\n";
		} else if (key.compare("QLEN_MON_INTERVAL") == 0) {
			conf >> qlen_mon_interval;
			std::cout << "QLEN_MON_INTERVAL\t\t" << qlen_mon_interval << "\n";
		} else if (key.compare("QLEN_MON_START") == 0) {
			conf >> qlen_mon_start;
			std::cout << "QLEN_MON_START\t\t\t\t" << qlen_mon_start << '\n';
		} else if (key.compare("QLEN_MON_END") == 0) {
			conf >> qlen_mon_end;
			std::cout << "QLEN_MON_END\t\t\t\t" << qlen_mon_end << '\n';
		} else if (key.compare("QLEN_MON_FILE") == 0) {
			conf >> qlen_output_file;
			std::cout << "QLEN_MON_FILE\t\t\t\t" << qlen_output_file << '\n';
		} 
		fflush(stdout);
	}
	conf.close();

	topof.open(topology_file.c_str());
	flowf.open(flow_file.c_str());
	uint32_t node_num, switch_num, tors, link_num, trace_num;
	topof >> node_num >> switch_num >> tors >> link_num; // changed here. The previous order was node, switch, link // tors is not used. switch_num=tors for now.
	tors = switch_num;
	std::cout << node_num << " " << switch_num << " " << tors <<  " " << link_num << std::endl;
	flowf >> flow_num;

	NodeContainer serverNodes;
	NodeContainer torNodes;
	NodeContainer spineNodes;
	NodeContainer switchNodes;
	NodeContainer allNodes;

	std::vector<uint32_t> node_type(node_num, 0);
	std::cout << "switch_num " << switch_num << std::endl;
	for (uint32_t i = 0; i < switch_num; i++) {
		uint32_t sid;
		topof >> sid;
		std::cout << "sid " << sid << std::endl;
		switchNumToId[i] = sid;
		switchIdToNum[sid] = i;
		if (i < tors) {
			node_type[sid] = 1;
		}
		else
			node_type[sid] = 2;

	}

	for (uint32_t i = 0; i < node_num; i++) {
		if (node_type[i] == 0) {
			Ptr<Node> node = CreateObject<Node>();
			n.Add(node);
			allNodes.Add(node);
			serverNodes.Add(node);
		}
		else {
			Ptr<SwitchNode> sw = CreateObject<SwitchNode>();
			n.Add(sw);
			switchNodes.Add(sw);
			allNodes.Add(sw);
			if (node_type[i] == 1) {
				torNodes.Add(sw);
				sw->SetNodeType(1);
			}
			else {
				spineNodes.Add(sw);
				sw->SetNodeType(2);
			}
		}
	}


	NS_LOG_INFO("Create nodes.");

	InternetStackHelper internet;
	Ipv4GlobalRoutingHelper globalRoutingHelper;
	internet.SetRoutingHelper (globalRoutingHelper);
	internet.Install(n);

	//
	// Assign IP to each server
	//
	for (uint32_t i = 0; i < node_num; i++) {
		if (n.Get(i)->GetNodeType() == 0) { // is server
			serverAddress.resize(i + 1);
			serverAddress[i] = node_id_to_ip(i);
		}
	}

	NS_LOG_INFO("Create channels.");

	FILE *pfc_file = fopen(pfc_output_file.c_str(), "w");

	QbbHelper qbb;
	Ipv4AddressHelper ipv4;
	for (uint32_t i = 0; i < link_num; i++) {
		uint32_t src, dst;
		std::string data_rate, link_delay;
		double error_rate;
		topof >> src >> dst >> data_rate >> link_delay >> error_rate;

		std::cout << src << " " << dst << " " << n.GetN() << " " << data_rate << " " << link_delay << " " << error_rate << std::endl;
		
		Ptr<Node> snode = n.Get(src), dnode = n.Get(dst);

		qbb.SetDeviceAttribute("DataRate", StringValue(data_rate));
		qbb.SetChannelAttribute("Delay", StringValue(link_delay));

		fflush(stdout);

		// Assigne server IP
		// Note: this should be before the automatic assignment below (ipv4.Assign(d)),
		// because we want our IP to be the primary IP (first in the IP address list),
		// so that the global routing is based on our IP
		NetDeviceContainer d = qbb.Install(snode, dnode);
		DynamicCast<QbbNetDevice>(d.Get(0))->SetMtu(packet_payload_size);
		DynamicCast<QbbNetDevice>(d.Get(1))->SetMtu(packet_payload_size);
		// DynamicCast<QbbNetDevice>(d.Get(0))->SetShapingPktSize(shaping_pkt_size);
		// DynamicCast<QbbNetDevice>(d.Get(1))->SetShapingPktSize(shaping_pkt_size);

		DynamicCast<QbbNetDevice>(d.Get(0))->m_alpha = alpha;
		DynamicCast<QbbNetDevice>(d.Get(1))->m_alpha = alpha;

		DynamicCast<QbbNetDevice>(d.Get(0))->m_kp_nic = kp_nic;
		DynamicCast<QbbNetDevice>(d.Get(1))->m_kp_nic = kp_nic;

		DynamicCast<QbbNetDevice>(d.Get(0))->m_kp_lc_i = kp_lc_i;
		DynamicCast<QbbNetDevice>(d.Get(1))->m_kp_lc_i = kp_lc_i;

		DynamicCast<QbbNetDevice>(d.Get(0))->m_kp_lc_e = kp_lc_e;
		DynamicCast<QbbNetDevice>(d.Get(1))->m_kp_lc_e = kp_lc_e;

		DynamicCast<QbbNetDevice>(d.Get(0))->max_credit_rate = DataRate(max_credit_rate);
		DynamicCast<QbbNetDevice>(d.Get(1))->max_credit_rate = DataRate(max_credit_rate);

		DynamicCast<QbbNetDevice>(d.Get(0))->min_jitter_ = min_jitter_;
		DynamicCast<QbbNetDevice>(d.Get(1))->min_jitter_ = min_jitter_;

		DynamicCast<QbbNetDevice>(d.Get(0))->max_jitter_ = max_jitter_;
		DynamicCast<QbbNetDevice>(d.Get(1))->max_jitter_ = max_jitter_;

		DynamicCast<QbbNetDevice>(d.Get(0))->target_loss_scaling_ = target_loss_scaling_;
		DynamicCast<QbbNetDevice>(d.Get(1))->target_loss_scaling_ = target_loss_scaling_;

		DynamicCast<QbbNetDevice>(d.Get(0))->w_init_ = w_init_;
		DynamicCast<QbbNetDevice>(d.Get(1))->w_init_ = w_init_;

		DynamicCast<QbbNetDevice>(d.Get(0))->min_w_  = min_w_ ;
		DynamicCast<QbbNetDevice>(d.Get(1))->min_w_  = min_w_ ;

		DynamicCast<QbbNetDevice>(d.Get(0))->alpha_  = alpha_ ;
		DynamicCast<QbbNetDevice>(d.Get(1))->alpha_  = alpha_ ;

		DynamicCast<QbbNetDevice>(d.Get(0))->max_tokens_ = max_tokens_;
		DynamicCast<QbbNetDevice>(d.Get(1))->max_tokens_ = max_tokens_;

		DynamicCast<QbbNetDevice>(d.Get(0))->m_creditMaxTokens = max_tokens_;
		DynamicCast<QbbNetDevice>(d.Get(1))->m_creditMaxTokens = max_tokens_;

		DynamicCast<QbbNetDevice>(d.Get(0))->token_refresh_rate_ = token_refresh_rate_;
		DynamicCast<QbbNetDevice>(d.Get(1))->token_refresh_rate_ = token_refresh_rate_;

		DynamicCast<QbbNetDevice>(d.Get(0))->m_creditRateLimiter = token_refresh_rate_;
		DynamicCast<QbbNetDevice>(d.Get(1))->m_creditRateLimiter = token_refresh_rate_;

		if (snode->GetNodeType() == 0) {
			Ptr<Ipv4> ipv4 = snode->GetObject<Ipv4>();
			ipv4->AddInterface(d.Get(0));
			ipv4->AddAddress(1, Ipv4InterfaceAddress(serverAddress[src], Ipv4Mask(0xff000000)));
		}
		if (dnode->GetNodeType() == 0) {
			Ptr<Ipv4> ipv4 = dnode->GetObject<Ipv4>();
			ipv4->AddInterface(d.Get(1));
			ipv4->AddAddress(1, Ipv4InterfaceAddress(serverAddress[dst], Ipv4Mask(0xff000000)));
		}
		if (!snode->GetNodeType()) {
			sourceNodes[src].Add(DynamicCast<QbbNetDevice>(d.Get(0)));
		}

		if (!snode->GetNodeType() && dnode->GetNodeType()) {
			switchDown[switchIdToNum[dst]].Add(DynamicCast<QbbNetDevice>(d.Get(1)));
		}

		if (snode->GetNodeType() && dnode->GetNodeType()) {
			switchToSwitchInterfaces.Add(d);
			switchUp[switchIdToNum[src]].Add(DynamicCast<QbbNetDevice>(d.Get(0)));
			switchUp[switchIdToNum[dst]].Add(DynamicCast<QbbNetDevice>(d.Get(1)));
			switchToSwitch[src][dst].push_back(DynamicCast<QbbNetDevice>(d.Get(0)));
			switchToSwitch[src][dst].push_back(DynamicCast<QbbNetDevice>(d.Get(1)));
		}

		nd.Add(d);

		// used to create a graph of the topology
		nbr2if[snode][dnode].idx = DynamicCast<QbbNetDevice>(d.Get(0))->GetIfIndex();
		nbr2if[snode][dnode].up = true;
		nbr2if[snode][dnode].delay = DynamicCast<QbbChannel>(DynamicCast<QbbNetDevice>(d.Get(0))->GetChannel())->GetDelay().GetTimeStep();
		nbr2if[snode][dnode].bw = DynamicCast<QbbNetDevice>(d.Get(0))->GetDataRate().GetBitRate();
		nbr2if[dnode][snode].idx = DynamicCast<QbbNetDevice>(d.Get(1))->GetIfIndex();
		nbr2if[dnode][snode].up = true;
		nbr2if[dnode][snode].delay = DynamicCast<QbbChannel>(DynamicCast<QbbNetDevice>(d.Get(1))->GetChannel())->GetDelay().GetTimeStep();
		nbr2if[dnode][snode].bw = DynamicCast<QbbNetDevice>(d.Get(1))->GetDataRate().GetBitRate();

		// This is just to set up the connectivity between nodes. The IP addresses are useless
		// char ipstring[16];
		std::stringstream ipstring;
		ipstring << "10." << i / 254 + 1 << "." << i % 254 + 1 << ".0";
		// sprintf(ipstring, "10.%d.%d.0", i / 254 + 1, i % 254 + 1);
		ipv4.SetBase(ipstring.str().c_str(), "255.255.255.0");
		ipv4.Assign(d);
	}
	nic_rate = get_nic_rate(n);
	uint32_t packet_size = packet_payload_size + CustomHeader::GetStaticWholeHeaderSize();
	// config switch
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 1){ // is switch
			Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n.Get(i));
			uint32_t shift = 3; // by default 1/8
			for (uint32_t j = 1; j < sw->GetNDevices(); j++){
				Ptr<QbbNetDevice> dev = DynamicCast<QbbNetDevice>(sw->GetDevice(j));
					uint64_t rate = dev->GetDataRate().GetBitRate();
				// set pfc
				// uint64_t delay = DynamicCast<QbbChannel>(dev->GetChannel())->GetDelay().GetTimeStep();
				// uint32_t headroom = rate * delay / 8 / 1000000000 * 3;
				// sw->m_mmu->ConfigHdrm(j, headroom);

				// // set pfc alpha, proportional to link bw
				// sw->m_mmu->pfc_a_shift[j] = shift;
				// while (rate > nic_rate && sw->m_mmu->pfc_a_shift[j] > 0){
				// 	sw->m_mmu->pfc_a_shift[j]--;
				// 	rate /= 2;
				// }
			}
			sw->m_mmu->ConfigNPort(sw->GetNDevices()-1);
			sw->m_mmu->ConfigBufferSize(buffer_size* 1000 * 1000);
			sw->m_mmu->node_id = sw->GetId();
		}
	}

	FILE *fct_output = fopen(fct_output_file.c_str(), "w");
	//
	// install RDMA driver
	//
	for (uint32_t i = 0; i < node_num; i++) {
		if (n.Get(i)->GetNodeType() == 0) { // is server
			// create RdmaHw
			Ptr<RdmaHw> rdmaHw = CreateObject<RdmaHw>();
			rdmaHw->SetAttribute("Mtu", UintegerValue(packet_payload_size));
			// create and install RdmaDriver
			Ptr<RdmaDriver> rdma = CreateObject<RdmaDriver>();
			Ptr<Node> node = n.Get(i);
			rdma->SetNode(node);
			rdma->SetRdmaHw(rdmaHw);

			node->AggregateObject (rdma);
			rdma->Init();
			rdma->TraceConnectWithoutContext("QpComplete", MakeBoundCallback (qp_finish, fct_output));
				}
			}

	// setup routing
	CalculateRoutes(n);
	SetRoutingEntries();

	//
	// get BDP and delay
	//
	maxRtt = maxBdp = 0;
	uint64_t minRtt = 1e9;
	for (uint32_t i = 0; i < node_num; i++) {
		if (n.Get(i)->GetNodeType() != 0)
			continue;
		for (uint32_t j = 0; j < node_num; j++) {
			if (n.Get(j)->GetNodeType() != 0)
				continue;
			if (i == j)
				continue;
			uint64_t delay = pairDelay[n.Get(i)][n.Get(j)];
			uint64_t txDelay = pairTxDelay[n.Get(i)][n.Get(j)];
			uint64_t rtt = delay * 2 + txDelay;
			uint64_t bw = pairBw[i][j];
			uint64_t bdp = rtt * bw / 1000000000 / 8;
			pairBdp[n.Get(i)][n.Get(j)] = bdp;
			pairRtt[i][j] = rtt;
			if (bdp > maxBdp)
				maxBdp = bdp;
			if (rtt > maxRtt)
				maxRtt = rtt;
			if (rtt < minRtt)
				minRtt = rtt;
		}
	}
	printf("maxRtt=%lu maxBdp=%lu minRtt=%lu\n", maxRtt, maxBdp, uint64_t(minRtt));

	//
	// setup switch CC
	//

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	// Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
	// globalRoutingHelper.PrintRoutingTableAt (Seconds (0.0), n.Get(0), routingStream);

	NS_LOG_INFO("Create Applications.");

	// maintain port number for each host
	for (uint32_t i = 0; i < node_num; i++) {
		if (n.Get(i)->GetNodeType() == 0)
			for (uint32_t j = 0; j < node_num; j++) {
				if (n.Get(j)->GetNodeType() == 0)
					portNumder[i][j] = 10000; // each host pair use port number from 10000
			}
	}


	flow_input.idx = 0;
	if (flow_num > 0) {
		ReadFlowInput();
		std::cout << flow_input.start_time << std::endl;
		Simulator::Schedule(Seconds(flow_input.start_time) - Simulator::Now(), ScheduleFlowInputs);
	}

	topof.close();
	tracef.close();
	double delay = 1.5 * minRtt * 1e-9; // 10 micro seconds
	FILE *qlen_file = fopen(qlen_output_file.c_str(), "w");
	Simulator::Schedule(NanoSeconds(qlen_mon_start), monitor_buffer, qlen_file, &n);
	// AsciiTraceHelper ascii;
	//     qbb.EnableAsciiAll (ascii.CreateFileStream ("eval.tr"));
	std::cout << "Running Simulation.\n";
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(simulator_stop_time));
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");
	endt = clock();
	std::cout << (double)(endt - begint) / CLOCKS_PER_SEC << "\n";
}
