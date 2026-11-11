/*
 * Controller.cc
 *
 *  Created on: Feb 24, 2021
 *      Author: Hosein
 */
#include "inet/networklayer/common/L3AddressTag_m.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/chunk/FieldsChunk.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "PlantPacket_m.h"
#include "ControllerPacket_m.h"

#include "Controller.h"
#include <list>
#include <typeinfo>

namespace inet {

Define_Module(Controller);

Controller::~Controller()
{
    cancelAndDelete(selfMsg);
}

void Controller::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {

        referenceValueSignal = registerSignal("reference");
        sensorValueSignal1 = registerSignal("sensor1");
        sensorValueSignal2 = registerSignal("sensor2");
        controlValueSignal = registerSignal("control");
        errorValueSignal = registerSignal("Error");
        DEVSignal1 = registerSignal("DevSignal1");
        DEVSignal2 = registerSignal("DevSignal2");
        normSignal = registerSignal("normSignal");
        corrSignal1 = registerSignal("corrSignal1");
        corrSignal2 = registerSignal("corrSignal2");
        corrSignal3 = registerSignal("corrSignal3");
        corrSignal4 = registerSignal("corrSignal4");

        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);

        localPort = par("localPort");
        destPort = par("destPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");
        dontFragment = par("dontFragment");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("sendTimer");
    }
}

void Controller::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    ApplicationBase::finish();
}

void Controller::setSocketOptions()
{
    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket.setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket.setDscp(dscp);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0]) {
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        InterfaceEntry *ie = ift->findInterfaceByName(multicastInterface);
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    bool joinLocalMulticastGroups = par("joinLocalMulticastGroups");
    if (joinLocalMulticastGroups) {
        MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
    }
    socket.setCallback(this);
}

L3Address Controller::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    if (destAddresses[k].isUnspecified() || destAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

void Controller::sendPacket()
{
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    if(dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);

    const auto& payload = makeShared<ControllerPacket>();
    payload->setChunkLength(B(par("messageLength")));

    if(simTime()<= 250 || simTime()>= 500)
    {
        payload->setControlValue(controlSignal);
    } else
    {
        payload->setControlValue(controlSignal);
    }

    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(payload);

    L3Address destAddr = chooseDestAddr();
    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    numSent++;
}

void Controller::processStart()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != nullptr) {
        destAddressStr.push_back(token);
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        destAddresses.push_back(result);
    }

    if (!destAddresses.empty()) {
        selfMsg->setKind(SEND);
        processSend();
    }
    else {
        if (stopTime >= SIMTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleAt(stopTime, selfMsg);
        }
    }
}

void Controller::processSend()
{
    sendPacket();
    simtime_t d = simTime() + par("sendInterval");
    if (stopTime < SIMTIME_ZERO || d < stopTime) {
        selfMsg->setKind(SEND);
        scheduleAt(d, selfMsg);
    }
    else {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void Controller::processStop()
{
    socket.close();
}

void Controller::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
                processStart();
                break;

            case SEND:
                processSend();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else
        socket.processMessage(msg);
}

void Controller::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    processPacket(packet);
}

void Controller::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void Controller::socketClosed(UdpSocket *socket)
{
    if (operationalState == State::STOPPING_OPERATION)
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void Controller::refreshDisplay() const
{
    ApplicationBase::refreshDisplay();

    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void Controller::processPacket(Packet *pk)
{
    auto l3Addresses = pk->getTag<L3AddressInd>();
    L3Address srcAddr = l3Addresses->getSrcAddress();
//&&(simTime()<= 650 || simTime()>= 800)
    if((srcAddr.str() == plantAddr))
    {
        auto& payload = pk->peekData<PlantPacket>();
      EV_INFO << "value1: " << payload->getSensorValue1() << endl;
      EV_INFO << "value2: " << payload->getSensorValue2() << endl;
        value1 = payload->getSensorValue1();
        value2 = payload->getSensorValue2();
    }

    emit(packetReceivedSignal, pk);

    x = controlSignal;
    PI_Controller(value2);

    Plant_Model();
    u[0] = x;
    u[1] = controlSignal;
    Evaluation();

    emit(sensorValueSignal1, value1);
    emit(sensorValueSignal2, value2);
    emit(controlValueSignal, controlSignal);
    numReceived++;
    delete pk;
}

void Controller::handleStartOperation(LifecycleOperation *operation)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
}

void Controller::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void Controller::handleCrashOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    socket.destroy();         //TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
}

void Controller::PI_Controller(double y)
{
    referenceValue = ReferenceSignal();
    controlError = referenceValue - y;
    Integral = Integral + Ts*controlError;
    controlSignal = Kp*controlError + Ki*Integral;

    if (controlSignal<0) controlSignal=0;

    emit(referenceValueSignal, referenceValue);
    emit(errorValueSignal, controlError);
}

double Controller::ReferenceSignal() {
    double time = simTime().dbl();

    if( (((time-t1)>0) && ((t2-time)>=0)) ) {
        return (62.5)*(time-t1);
    } else if( (time-t2)>0 ) {
        return 10000;
    } else {
        return 0;
    }
}

void Controller::Plant_Model()
{
    p1 = value1;
    p2 = value2;

    R1 = (p1 - z1);
    R2 = (p2 - z2);

    Y1 = R1 + 0.99*Y1;
    Y2 = R2 + 0.99*Y2;

    S1 = (Q[0][0]*Y1 + Q[0][1]*Y2);
    S2 = (Q[1][1]*Y2 + Q[1][0]*Y1);


    emit(DEVSignal1, Y1);
    emit(DEVSignal2, Y2);

    z1 = G[0][0]*z1 + G[0][1]*z2 + D[0][0]*p1 + D[0][1]*p2 + Bb[0]*u[0];
    z2 = G[1][0]*z1 + G[1][1]*z2 + D[1][0]*p1 + D[1][1]*p2 + Bb[1]*u[0];

}

void Controller::Evaluation()
{
    norm_Y = sqrt(Y1*Y1 + Y2*Y2);
    norm_R = sqrt(S1*S1 + S2*S2);

    corr1 = abs(Y1)/(norm_Y+0.001);
    corr2 = abs(Y2)/(norm_Y+0.001);
    corr3 = abs(S1)/(norm_R+0.001);
    corr4 = abs(S2)/(norm_R+0.001);

    emit(normSignal, norm_Y);
    emit(corrSignal1, corr1);
    emit(corrSignal2, corr2);
    emit(corrSignal3, corr3);
    emit(corrSignal4, corr4);

}

}


