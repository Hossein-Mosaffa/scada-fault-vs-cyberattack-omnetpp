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

import inet.common.packet.recorder.PcapRecorder;
import inet.applications.contract.IApp;
import inet.common.MessageDispatcher;
import inet.linklayer.contract.IEthernetInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.INetworkLayer;
import inet.transportlayer.contract.IUdp;
import inet.linklayer.contract.IPppInterface;


module MyNode
{
    parameters:
        int numPcapRecorders = default(0);
        *.interfaceTableModule = default(absPath(".interfaceTable"));

        bool forwarding = default(false);
        bool multicastForwarding = default(false);
        *.forwarding = forwarding;
        *.multicastForwarding = multicastForwarding;

        ipv4.arp.proxyArpInterfaces = default(""); // proxy arp is disabled on hosts by default
       // *.routingTableModule = default("^.ipv4.routingTable");

        @networkNode;

    gates:
        inout ethg;
        inout pppg;
    submodules:
        app: <> like IApp {
            @display("p=100,34");
        }
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp {
            @display("p=132,129");
        }
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer {
            @display("p=151,215");
        }
        nl: MessageDispatcher {
            @display("p=189,268");
        }
        interfaceTable: InterfaceTable {
            @display("p=353,164;is=s");
        }
        eth: <default("EthernetInterface")> like IEthernetInterface {
            @display("p=139,333");
        }
        ppp: <default("PppInterface")> like IPppInterface {
            parameters:
                @display("p=231,333,row,150;q=txQueue");
        }
        pcapRecorder[numPcapRecorders]: PcapRecorder {
            parameters:
                @display("p=353,215;is=s");
        }
        tn: MessageDispatcher {
            @display("p=208,173");
        }
        at: MessageDispatcher {
            @display("p=162,77");
        }
    connections allowunconnected: // network node internal connections

        app.socketOut --> at.in++;
        at.out++ --> app.socketIn;

        at.out++ --> udp.appIn;
        udp.appOut --> at.in++;

        udp.ipOut --> tn.in++;
        tn.out++ --> udp.ipIn;

        tn.out++ --> ipv4.transportIn;
        ipv4.transportOut --> tn.in++;

        ipv4.ifOut --> nl.in++;
        nl.out++ --> ipv4.ifIn;

        nl.out++ --> eth.upperLayerIn;
        eth.upperLayerOut --> nl.in++;
        
        ppp.upperLayerOut --> nl.in++;
        nl.out++ --> ppp.upperLayerIn;

        eth.phys <--> ethg;
        pppg <--> ppp.phys;
}



