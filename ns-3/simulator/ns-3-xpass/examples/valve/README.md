# FDU-FNIL传输控制小组NS3使用手册

## examples/valve的目录结构

该目录下包含`_archive`、`_figure`、`_plot`三个固定文件夹，`CMakeLists.txt`、`config.sh`、`script.sh`、`output.txt`、`README.md`五个固定文件，以及一系列project directories，实例中包含`simple-incast`和`fairness_evaluation`两个project目录。

* `_archive`**的作用是将有价值的数据和脚本进行归档**，其内部包含由project name命名的子目录，子目录下是归档的目录和文件；
* `_plot`**含有四个实例绘图脚本，同时包含各个project的绘图脚本**，用由project name命名的子目录存储，可以直接以project name和output format作为参数产生草稿图像，定制化的绘图脚本也可以参考该文件；
* `_figure`**的作用是存储绘图脚本产生的图像**，用由project name命名的子目录存储；
* `CMakeLists.txt`的作用是将各个project directories包括进入cmake编译的范围内，因此如果新建了一个project，需要将其目录名加入到文件中；
* `config.sh`的作用是设置环境变量，比如ns3目录的路径；
* `script.sh`的作用是运行simulation，如果需要，可以在各自的project目录下，参考该文件编写自己的script.sh；
* `output.txt`的作用是当仿真输出信息过多时，可以被重定向用以接收输出信息；
* `project directories`包含该project源程序、配置txt文件、输出信息目录以及CMake文件等。

## 如何创建一个新的simulation project

1. 在valve目录下，创建project目录，假设project name是`new_proj`；
2. 从示例project中将三个配置txt文件、一个CMakeLists.txt文件以及cc文件复制到`valve/new_proj`目录下，并将三个配置txt文件改名为`new_proj_config.txt`、`new_proj_flow.txt`、`new_proj_topology.txt`，将cc文件改名为`new_proj.cc`；
3. 在`valve/new_proj`目录下创建`monitor_output`用于接收吞吐、队列、发送速率、FCT、PFC等信息，如果有自定义的信息输出，请也输出到该文件下；
4. 修改`valve/new_proj/CMakeLists.txt`，将source files的CC文件改为`new_proj.cc`；
5. 将`new_proj_config.txt`中的输入输出文件路径**全部更新**；
6. 在`valve/CMakeLists.txt`中添加`add_subdirectory(new_proj)`；
7. 调用valve目录下的script.sh即可：`./script.sh new_proj`。
* 【注意】如果需要定制化的源程序、shell脚本、绘图py脚本、数据分析脚本，请输出到各自的目录下不要随意放置。

## 目录结构实例

```
├── _archive
|   ├── simple-incast
│   │   ├── ...
│   │   └── ...
│   └── fairness_evaluation
│       ├── ...
│       └── ...
├── _figure
│   ├── fairness_evaluation
│   │   ├── ...
│   │   └── ...
│   └── simple-incast
│       ├── ...
│       └── ...
├── _plot
│   ├── fairness_evaluation
│   │   ├── ...
│   │   └── ...
│   ├── simple-incast
│   │   ├── ...
│   │   └── ...
│   ├── plot_qlen.py
│   ├── plot_sending_rate.py
│   ├── plot_throughput.py
│   └── plot.sh
├── CMakeLists.txt
├── config.sh
├── script.sh
├── output.txt
├── README.md
├── fairness_evaluation
│   ├── cdf.c
│   ├── cdf.h
│   ├── CMakeLists.txt
│   ├── monitor_output
│   │   ├── fct.txt
│   │   ├── pfc.txt
│   │   ├── qlen_dist.txt
│   │   ├── qlen.txt
│   │   ├── sending_rate.txt
│   │   └── throughput.txt
│   ├── fairness_evaluation.cc
│   ├── fairness_evaluation_config.txt
│   ├── fairness_evaluation_flow.txt
│   └── fairness_evaluation_topology.txt
└── simple-incast
├── cdf.c
├── cdf.h
├── CMakeLists.txt
├── monitor_output
│   ├── fct.txt
│   ├── pfc.txt
│   ├── qlen_dist.txt
│   ├── qlen.txt
│   ├── sending_rate.txt
│   └── throughput.txt
├── simple-incast.cc
├── simple-incast_config.txt
├── simple-incast_flow.txt
└── simple-incast_topology.txt
```

