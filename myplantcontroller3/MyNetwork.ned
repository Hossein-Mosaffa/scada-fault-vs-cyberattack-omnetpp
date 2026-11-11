import inet.common.scenario.ScenarioManager;
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import ned.DatarateChannel;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;

//
// TODO documentation
//
network MyNetwork
{
    types:
        channel ethline extends DatarateChannel
        {
            delay = 10ns;
            datarate = 10Mbps;
        }
    submodules:
        router: Router {
            @display("p=228,109");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=117,257");
        }
        plant: MyNode {
            @display("p=304,109");
        }
        controller: MyNode {
            @display("p=27,109");
        }
        router1: Router {
            @display("p=100,109");
        }
        router2: Router {
            @display("p=162,58");
        }
        router3: Router {
            @display("p=162,159");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=54,257");
        }
        attacker2: MyNode {
            @display("p=228,194");
        }
        attacker1: MyNode {
            @display("p=100,23");
        }
    connections allowunconnected:

        plant.ethg <--> ethline <--> router.ethg++;
        router.ethg++ <--> ethline <--> router2.ethg++;
        router2.ethg++ <--> ethline <--> router1.ethg++;
        router1.ethg++ <--> ethline <--> router3.ethg++;
        router3.ethg++ <--> ethline <--> router.ethg++;
        router1.ethg++ <--> ethline <--> controller.ethg;
        attacker2.ethg <--> ethline <--> router.ethg++;
        attacker1.ethg <--> ethline <--> router1.ethg++;
}

