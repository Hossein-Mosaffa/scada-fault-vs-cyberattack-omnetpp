
#include "Plant.h"
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

#include <list>

namespace inet {

Define_Module(Plant);

Plant::~Plant()
{
    cancelAndDelete(selfMsg);
}

void Plant::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {

        sensorValueSignal1 = registerSignal("sensor1");
        sensorValueSignal2 = registerSignal("sensor2");
        controlValueSignal = registerSignal("control");

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

void Plant::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    ApplicationBase::finish();
}

void Plant::setSocketOptions()
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

L3Address Plant::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    if (destAddresses[k].isUnspecified() || destAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

void Plant::sendPacket()
{
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    if(dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);

    const auto& payload = makeShared<PlantPacket>();
    payload->setChunkLength(B(par("messageLength")));


    if(simTime()<= 250 || simTime()>= 500)
    {
        payload->setSensorValue1(value1);
        payload->setSensorValue2(value2);

    } else
    {
        payload->setSensorValue1(value1 );
        payload->setSensorValue2(value2 );
    }

    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(payload);

    L3Address destAddr = chooseDestAddr();
    emit(packetSentSignal, packet);
    socket.sendTo(packet, destAddr, destPort);
    numSent++;
}

void Plant::processStart()
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

void Plant::processSend()
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

void Plant::processStop()
{
    socket.close();
}

void Plant::RealPlant(double u)
{
    p1 = Aa[0][0]*p1 + Aa[0][1]*p2 + Bb[0]*u;
    p2 = Aa[1][0]*p1 + Aa[1][1]*p2 + Bb[1]*u;

    value1 = p1;
    value2 = p2;

    emit(sensorValueSignal1, value1);
    emit(sensorValueSignal2, value2);
}

void Plant::DamagedPlant(double u)
{
//    p1 = Aa[0][0]*p1 + Aa[0][1]*(p2) + Bb[0]*(0);
//    p2 = Aa[1][0]*p1 + Aa[1][1]*(p2) + Bb[1]*(0);
//
//    value1 = p1;
//    value2 = p2;

    p1 = Aa[0][0]*p1 + Aa[0][1]*(p2) + Bb[0]*(u);
    p2 = Aa[1][0]*p1 + Aa[1][1]*(p2) + Bb[1]*(u);

    value1 = p1-5000;
    value2 = p2-2000;

//    p1 = Ad[0][0]*p1 + Ad[0][1]*p2 + Bd[0]*u;
//    p2 = Ad[1][0]*p1 + Ad[1][1]*p2 + Bd[1]*u;
//
//    value1 = p1;
//    value2 = p2;

    emit(sensorValueSignal1, value1);
    emit(sensorValueSignal2, value2);
}

void Plant::handleMessageWhenUp(cMessage *msg)
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

void Plant::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    processPacket(packet);
}

void Plant::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void Plant::socketClosed(UdpSocket *socket)
{
    if (operationalState == State::STOPPING_OPERATION)
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void Plant::refreshDisplay() const
{
    ApplicationBase::refreshDisplay();

    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void Plant::processPacket(Packet *pk)
{
    emit(packetReceivedSignal, pk);
    auto l3Addresses = pk->getTag<L3AddressInd>();
    L3Address srcAddr = l3Addresses->getSrcAddress();

    if((srcAddr.str() == controllerAddr))
        {
            auto& payload = pk->peekData<ControllerPacket>();
            EV_INFO << "controlSignal: " << payload->getControlValue() << endl;
            plantInput = payload->getControlValue();
        }
    emit(controlValueSignal, plantInput);

    if((simTime() <= 250) || (simTime() >= 500))
        {
            RealPlant(plantInput);
        } else
        {
            DamagedPlant(plantInput);
        }

    numReceived++;
    delete pk;
}

void Plant::handleStartOperation(LifecycleOperation *operation)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
}

void Plant::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void Plant::handleCrashOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    socket.destroy();         //TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
}


}




