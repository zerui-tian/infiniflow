# InfiniFlow: Decoupling Virtual Channel Scalability from Buffer Requirements in Lossless Datacenter Networks

InfiniFlow is a credit-based, hop-by-hop flow-control method for lossless datacenter networks. It is designed to provide fine-grained flow isolation without making packet-buffer requirements grow linearly with the number of virtual channels (VCs).

Traditional PFC and CBFC reserve buffer space for every VC. InfiniFlow instead introduces Upstream Allocates Buffer for Downstream (UABD): the upstream port maintains a shared credit pool representing the downstream port's available buffer and allocates credits to active VCs as packets are scheduled. A Buffer Usage Control Protocol (BUCP) uses real-time per-VC backlog feedback to adapt credit limits, preventing a throttled VC from monopolizing the shared buffer.

## Citation

If you use InfiniFlow in your research, please cite:

```bibtex
@inproceedings{infiniflow,
  author = {Tian, Zerui and Liu, Sen and Xue, Minkun and Shangguan, Hao and Yao, Ruyi and Mei, Hao and Huang, Deli and Xue, Songchen and Xu, Yang},
  title = {InfiniFlow: Decoupling Virtual Channel Scalability from Buffer Requirements in Lossless Datacenter Networks},
  year = {2026},
  month = {August},
  booktitle = {Proceedings of the ACM SIGCOMM 2026 Conference},
  address = {Denver, CO},
  publisher = {Association for Computing Machinery},
  pages={1228--1245}
}
```

## Demo

We provide an animation that visualizes one of our NS-3 experiments. It shows how traffic and congestion evolve during the experiment, making it easy to see how InfiniFlow isolates flows, contains backpressure, and mitigates head-of-line blocking.

<!-- Replace DEMO_URL with the URL of the animation. -->

▶ Watch the InfiniFlow NS-3 demo

[![InfiniFlow in Action](https://img.youtube.com/vi/0BS08mCHE2A/0.jpg)](https://www.youtube.com/watch?v=0BS08mCHE2A)
