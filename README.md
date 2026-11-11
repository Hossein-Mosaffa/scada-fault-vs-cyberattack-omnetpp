# Networked Control System in OMNeT++: Telling Faults from Cyber Attacks

An OMNeT++ / INET simulation of a two-tank fluid system controlled over a UDP network. A Beard fault detection filter (BFDF) finds anomalies in the plant. Network traffic then shows whether an anomaly is a physical fault or a cyber attack.

Paper: H. Mosaffa and H. Khaloozadeh, "Detection and isolation of fault from cyber attack in SCADA systems using network parameters", *Tabriz Journal of Electrical Engineering*, 51(4), 2021, pp. 423-430. [Journal page](https://tjee.tabrizu.ac.ir/article_14668.html?lang=en)

## The idea

A deviation in a sensor can come from a broken sensor or from an attacker on its channel. Physical data alone cannot separate the two, so the method uses two independent signals.

1. The BFDF residual shows where the anomaly is: actuator, sensor 1, sensor 2, or the plant itself.
2. Network traffic shows what it is. If traffic is above its known normal level, it is an attack.

## System model

- Plant (`Plant.cc`): two coupled tanks, discrete model with A = [[0.9999, 8.69e-5], [8.69e-5, 0.9999]] and B = [6366, 0.2769]. The plant updates every time a control packet arrives, and sends its two sensor values back every 10 ms.
- Controller (`Controller.cc`): PI controller (Ts = 0.01 s, Kp = 3.22e-7, Ki = 2.26e-10) that makes the pressure of tank 2 follow a reference. The reference is 0 until 220 s, ramps up to 10000 at 380 s, then stays there.
- Network (`MyNetwork.ned`): plant and controller exchange 20-byte UDP packets every 10 ms through a router. Normal traffic per node is about 16,000 bps. Two attacker hosts (`attacker1` towards the plant, `attacker2` towards the controller) can flood the links.
- Filter: G = 0.8 I, D = A - G, and a matrix Q that separates the two sensor directions. The controller computes four correlations (`corr1` to `corr4`) between the residual and each anomaly direction. A value above 0.98 marks that anomaly.

The code passes the residual through a leaky accumulator (`Y = R + 0.99*Y`) before computing the correlations. The paper's appendix shows the simplified form without it.

## Repository layout

```
.
├── README.md
├── LICENSE
├── .gitignore
├── paper/                 # paper PDF
└── myplantcontroller3/    # the OMNeT++ project
    ├── Controller.cc / .h / .ned
    ├── Plant.cc / .h / .ned
    ├── MyPlantNode.ned
    ├── MyNetwork.ned
    ├── ControllerPacket.msg
    ├── PlantPacket.msg
    ├── newRoute.xml
    ├── omnetpp.ini
    └── makefrag
```

Files that OMNeT++ creates (`out/`, `Makefile`, `*_m.cc`, `*_m.h`, `.exe` files, result files) are not in the repo. They are rebuilt on your machine. The hidden files `.project`, `.cproject` and `.oppbuildspec` must stay inside `myplantcontroller3/`, because the IDE needs them to import the project.

## Requirements

- OMNeT++ (WRITE YOUR VERSION HERE, for example 5.6 or 6.0).
- INET Framework 4.x (WRITE YOUR VERSION HERE). The code uses INET's `Packet` and `UdpSocket` API, so INET 3.x will not work.
- A C++ compiler (g++ on Linux, the MinGW toolchain that ships with OMNeT++ on Windows).

## Run it in the OMNeT++ IDE

1. Install OMNeT++, start the IDE, and install INET when it asks. Let it build INET once, which takes a while.
2. Clone this repo: `git clone https://github.com/Hossein-Mosaffa/NetworkedControlSystemOmnet-.git`
3. In the IDE choose File > Import > General > Existing Projects into Workspace. Select the `myplantcontroller3` folder and finish.
4. Right-click the project > Properties > Project References, and tick `inet`. Without this the build fails with missing INET headers.
5. Build with Project > Build All (Ctrl+B). The IDE generates `ControllerPacket_m.*` and `PlantPacket_m.*` from the `.msg` files.
6. Right-click `omnetpp.ini` > Run As > OMNeT++ Simulation. The network is `MyNetwork`. Pick Qtenv for the graphical view.
7. The simulation lasts 800 s of simulated time. Use the fast-run button in Qtenv.
8. Open the `.vec` file from the `results` folder in the Analysis Tool. Plot `corrSignal1` to `corrSignal4`, `normSignal`, `DevSignal1`, `DevSignal2`, `sensor1`, `sensor2`, `control` and `reference`. Network traffic and end-to-end delay come from the INET host statistics.

## Run it from the command line (Linux)

```
cd myplantcontroller3
opp_makemake -f --deep -O out -KINET_PROJ=../../inet -DINET_IMPORT -I$\(INET_PROJ\)/src -L$\(INET_PROJ\)/src -lINET
make MODE=release
./myplantcontroller3 -u Cmdenv -n .:../../inet/src -l ../../inet/src/INET omnetpp.ini
```

Adjust the INET path to where INET lives on your machine. If the IDE already made a working Makefile, use that one.

## Scenarios

As committed, the project runs one scenario: a plant fault between 250 s and 500 s. In `Plant::processPacket`, the plant calls `DamagedPlant()` in that window, which shifts the sensor readings (sensor 1 by -5000, sensor 2 by -2000). Outside the window it uses `RealPlant()`. The two attacker hosts are off, because `startTime` and `stopTime` are both 0 s in `omnetpp.ini`.

To run the other cases, change the code or the ini file and rebuild:

| Case | How to create it |
|---|---|
| Normal | Make the window in `Plant::processPacket` never true, for example `simTime() >= 1000`. |
| Fault | Default behavior (`DamagedPlant()` between 250 s and 500 s). |
| DoS attack | Set `startTime = 250s` and `stopTime = 500s` for `attacker1` or `attacker2` in `omnetpp.ini`, and use a `sendInterval` much smaller than 10 ms. |
| FDI on sensor data | In `Plant::sendPacket`, put modified values in the branch for 250 s to 500 s. Both branches are identical at the moment, so add an offset there. |
| FDI on control data | Same idea in `Controller::sendPacket`, with the control value. |

How to read the result (the paper's decision rule): a residual above a threshold means an anomaly. The largest correlation above 0.98 says where it is. Traffic above the expected level (for example twice normal) says attack, otherwise fault. A DoS on the sensor side is also flagged when the control signal changes but the sensor values stay constant for 50 samples, about 0.5 s.

## Expected results

Correlations at the time of each anomaly, from the paper:

| Anomaly | Corr1 | Corr2 | Corr3 | Corr4 |
|---|---|---|---|---|
| Actuator | 1 | 0.02 | 0.74 | 0.66 |
| Plant (pipe) | 0 | 1 | 0.66 | 0.74 |
| Sensor 1 | 0.75 | 0.65 | 1 | 0.1 |
| Sensor 2 | 0.65 | 0.75 | 0.1 | 1 |

The plots are in the paper. Run the simulation to reproduce them.

## Limits

- A small two-tank system in simulation, with no real data.
- Thresholds (0.98, traffic level, 50 samples) are chosen by hand.
- Stealthy attacks that keep traffic normal are not covered.

## Future work

Train a machine learning classifier on residuals and network features (delay, jitter, packet loss, traffic) and compare it with the rules above, as the paper suggests.

## Citation

```
@article{mosaffa2021scada,
  author  = {Mosaffa, Hossein and Khaloozadeh, Hamid},
  title   = {Detection and isolation of fault from cyber attack in SCADA systems using network parameters},
  journal = {Tabriz Journal of Electrical Engineering},
  volume  = {51},
  number  = {4},
  pages   = {423--430},
  year    = {2021},
  url     = {https://tjee.tabrizu.ac.ir/article_14668.html?lang=en}
}
```

## License

MIT. See the `LICENSE` file.
