//
// Copyright (C) 2012 Calin Cerchez
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

#include "LTERadio.h"
#include "FWMath.h"
#include "PhyControlInfo_m.h"
#include "Radio80211aControlInfo_m.h"
#include "BasicBattery.h"
#include "InterfaceTableAccess.h"
#include "IPv4InterfaceData.h"
#include "LTEChannelControl.h"
#include "MACMessage.h"
#include "MACControlInfo_m.h"
//#include "HARQProcess.h"

Define_Module(LTERadio);

LTERadio::LTERadio() : rs(this->getId()) {
    radioModel = NULL;
    receptionModel = NULL;
    ttiTimer = NULL;
}

LTERadio::~LTERadio() {
    delete radioModel;
    delete receptionModel;

    if (ttiTimer != NULL) {
        if (ttiTimer->getContextPointer() != NULL)
            this->cancelEvent(ttiTimer);
        delete ttiTimer;
    }
}

void LTERadio::initialize(int stage) {

    ChannelAccess::initialize(stage);

    if (stage == 0) {
//    	rs.setChannelNumber((int)par("channelNumber"));
//    	rs.setState(RadioState::IDLE);

    	receptionModel = createReceptionModel();
    	radioModel = createRadioModel();
    	ueId = this->getParentModule()->getId();

    	nb = NotificationBoardAccess().get();

    	lteCfg = LTEConfigAccess().get();
    	lteSched = LTESchedulerAccess().get();
    } else if (stage == 2) {
        //cc->setRadioChannel(myRadioRef, rs.getChannelNumber());
        if (!strncmp(this->getParentModule()->getComponentType()->getName(), "ENB", 3)) {
            EV << "LTE-Radio: LTE physical module for ENB.\n";
            cc->setRadioChannel(myRadioRef, PRACH);
            cc->setRadioChannel(myRadioRef, PUSCH);
            // TODO rest of the table 3GPP TS 36211 Table 5.7.1-2 pag. 32
            switch (lteCfg->getPRACHCfgIndex() % 16) {
                case 0:
                    lteSched->scheduleMessage(UL_SCHEDULING, RAP_MSG_ID, 0, 2, UINT32_MAX, prachCfgIndex0TTIs, 1, lteCfg->getPRACHFreqOffset(), 6);
                    break;
                default:
                    break;
            }
//            cc->setRadioChannel(myRadioRef, Uplink);
        } else if (!strncmp(this->getParentModule()->getComponentType()->getName(), "UE", 2)) {
            EV << "LTE-Radio: LTE physical module for UE.\n";
            cc->setRadioChannel(myRadioRef, PDSCH);
            cc->setRadioChannel(myRadioRef, PDSCH0);
            cc->setRadioChannel(myRadioRef, PDSCH1);
            cc->setRadioChannel(myRadioRef, PHICH);
            cc->setRadioChannel(myRadioRef, PBCH);
            cc->setRadioChannel(myRadioRef, PDCCH);
//            cc->setRadioChannel(myRadioRef, Downlink);
        }
    }
//    else  if(stage == 4)
//	{
//    	if(rs.getChannelNumber()!=10)
//    	{
//			ift = InterfaceTableAccess().get();
//			InterfaceEntry *entry = new InterfaceEntry(this);
//			IPv4InterfaceData *ipv4data = new IPv4InterfaceData();
//			entry->setIPv4Data(ipv4data);
//			entry->setName("UERadioInterface");
//			entry->setMACAddress(MACAddress::generateAutoAddress());
//			ift->addInterface(entry);
//    	}
//	}
    if (stage == 4) {
        ttiTimer = new cMessage("TTI-TIMER");
        this->scheduleAt(simTime() + TTI_VALUE, ttiTimer);
        ttiTimer->setContextPointer(this);

//        for (unsigned i = 0; i < PRB_MAX_SIZE; i++) {
//            PhysicalResourceBlock *prb = new PhysicalResourceBlock();
//            prb->setName(prbNames[i]);
//            subFrame[i] = prb;
//        }

        // request scheduling from MAC layer
    }
}

void LTERadio::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == ttiTimer) {
//            nb->fireChangeNotification(NF_PHY_PER_CB, NULL);

            int msgId = -1;
            if (!strncmp(this->getParentModule()->getComponentType()->getName(), "ENB", 3)) {
                msgId = lteSched->getMessageId(DL_SCHEDULING);
                if (lteSched->getTTI() == 0)
                    sendDCIFormats();
            } else {
                msgId = lteSched->getMessageId(UL_SCHEDULING);
            }

            if (msgId != -1) {
                EV << "LTE-Radio: Sending scheduled message with id " << msgId << ".\n";
                TransportBlock *tb = queue[msgId];
                if (msgId == RAP_MSG_ID) {
                    tb->setRnti(1 + lteSched->getTTI() + 10 * tb->getRnti());
//                  rarLimit = lteSched->getSFN() + 3 + lteCfg->getRaRespWdwSize();
                }
//                this->send(tb->dup(), gate("lowerLayerOut"));
                sendDown(tb->dup());
            }

            this->cancelEvent(ttiTimer);
            this->scheduleAt(simTime() + TTI_VALUE, ttiTimer);
            lteSched->incrementTTI();
//            for (unsigned i = 0; i < PRB_MAX_SIZE; i++) {
//                sendToChannel(subFrame[i]);
//                subFrame[i] = new PhysicalResourceBlock();
//                subFrame[i]->setName(prbNames[i]);
//            }
//            this->cancelEvent(ttiTimer);
//            this->scheduleAt(simTime() + TTI_VALUE, ttiTimer);

            // request scheduling from MAC layer
        }
//        } else {
//            LTESubFrame *subFrame = check_and_cast<LTESubFrame*>(msg);
//            if (!strncmp(this->getParentModule()->getComponentType()->getName(), "ENB", 3)) {
//                EV << "LTE-Radio: Successfully received LTESubFrame on uplink direction.\n";
//            } else if (!strncmp(this->getParentModule()->getComponentType()->getName(), "UE", 2)) {
//                EV << "LTE-Radio: Successfully received LTESubFrame on downlink direction.\n";
//            }
//
//        }
////        HARQFrame *frame = check_and_cast<HARQFrame*>(msg);
////        sendToChannel(frame);
    } else if (msg->arrivedOn("radioIn")) {    // from radio interface
//        if (!strncmp(this->getParentModule()->getComponentType()->getName(), "ENB", 3)) {
//            EV << "LTE-Radio: Beginning to receive LTESubFrame on uplink direction.\n";
//        } else if (!strncmp(this->getParentModule()->getComponentType()->getName(), "UE", 2)) {
//            EV << "LTE-Radio: Beginning to receive LTESubFrame on downlink direction.\n";
//        }
        handleRadioMessage(msg);
    } else { // from upper layer
        handleUpperMessage(msg);
    }
}

void LTERadio::sendToRadio(cMessage *msg, int channel) {

}

void LTERadio::sendDCIFormats() {
//    bool direction = !strncmp(this->getParentModule()->getComponentType()->getName(), "ENB", 3) ? DL_SCHEDULING : UL_SCHEDULING;
    for (unsigned i = 0; i < 10; i++) {
        int msgId = lteSched->getMessageId(DL_SCHEDULING, i);
        if (msgId != -1 && queue[msgId] != NULL) {
            TransportBlock *tb = queue[msgId];
            if (tb->getChannel() == DLSCH1) {
                EV << "LTE-Radio: Sending DCIFormat1A for message with id = " << msgId <<".\n";
                DCIFormat1A *dci = new DCIFormat1A();
                dci->setName("DCIFormat1A");
                dci->setRnti(tb->getRnti());
                dci->setRntiType(tb->getRntiType());
                sendToChannel(dci);
            }
        }
    }
}

//AirFrame *LTERadio::encapsulatePacket(cPacket *frame) {
////   PhyControlInfo *ctrl = dynamic_cast<PhyControlInfo *>(frame->removeControlInfo());
////
////    // Note: we don't set length() of the AirFrame, because duration will be used everywhere instead
////    AirFrame *airframe = new AirFrame();
////    airframe->setName(frame->getName());
//////    airframe->setPSend(transmitterPower);
////    if (ctrl == NULL) {
////    	airframe->setChannelNumber(rs.getChannelNumber());
////    } else {
////    	airframe->setChannelNumber(ctrl->getChannelNumber());
////    	delete ctrl;
////    }
////    airframe->encapsulate(frame);
//////    airframe->setBitrate(ctrl ? ctrl->getBitrate() : rs.getBitrate());
//////    airframe->setDuration(radioModel->calculateDuration(airframe));
//////    airframe->setSenderPos(getMyPosition());
////
////
//////    EV << "Frame (" << frame->getClassName() << ")" << frame->getName()
//////       << " will be transmitted at " << (airframe->getBitrate()/1e6) << "Mbps\n";
////    return airframe;
//}

void LTERadio::sendDown(TransportBlock *tb) {
    PhysicalResourceBlock *prb = new PhysicalResourceBlock();
    switch (tb->getChannel()) {
        case RACH: {
//            RAPreamble *rap = new RAPreamble();
//            rap->setChannelNumber(PRACH);
//            rap->setRapid(ctrl->getRapid());
//            frame = rap;
            prb->setChannelNumber(PRACH);
            break;
        }
        case DLSCH0:
            prb->setChannelNumber(PDSCH0);
            break;
        case DLSCH1:
            prb->setChannelNumber(PDSCH1);
            break;
//        case ULSCH:
//            frame->setChannelNumber(PUSCH);
//            break;
        case BCH:
            prb->setChannelNumber(PBCH);
            break;
        default:
            break;
    }

    prb->encapsulate(tb);
    prb->setName(tb->getName());
    sendToChannel(prb);
}

void LTERadio::handleUpperMessage(cMessage* msg) {

    // no point in sending n Physical Resource Blocks and will send only one Physical Resource
    // Block with the Transport Block encapsulated

    MACProtocolDataUnit *pdu = dynamic_cast<MACProtocolDataUnit*>(msg);
    MACControlInfo *ctrl = dynamic_cast<MACControlInfo*>(msg->removeControlInfo());

    if (ctrl) {
        TransportBlock *tb = new TransportBlock();
        if (ctrl->getChannel() == RACH) {
            RAPControlInfo *rapCtrl = check_and_cast<RAPControlInfo*>(ctrl);
            RandomAccessPreamble *rap = new RandomAccessPreamble();
            rap->setRapid(rapCtrl->getRapid());
            rap->setAttempt(rapCtrl->getAttempt());
            rap->setName("RandomAccessPreamble");
            rap->setKind(RAP_MSG_ID);
            tb = rap;
        } else {
            tb->setName(pdu->getName());
            tb->encapsulate(pdu);
            tb->setKind(pdu->getKind());
        }
        tb->setChannel(ctrl->getChannel());
        tb->setRntiType(ctrl->getRntiType());
        tb->setRnti(ctrl->getRnti());
        tb->setUeId(ctrl->getUeId());
        queue[tb->getKind()] = tb;
    }

//    switch (ctrl->getChannel()) {
//        case RACH: {
//            RAPreamble *rap = new RAPreamble();
//            rap->setChannelNumber(PRACH);
//            rap->setRapid(ctrl->getRapid());
//            frame = rap;
//            prb->setChannelNumber(PRACH);
//            break;
//        }
//        case DLSCH0:
//            prb->setChannelNumber(PDSCH0);
//            break;
//        case DLSCH1:
//            prb->setChannelNumber(PDSCH1);
//            break;
//        case ULSCH:
//            frame->setChannelNumber(PUSCH);
//            break;
//        case BCH:
//            prb->setChannelNumber(PBCH);
//            break;
//        default:
//            break;
//    }

//    prb->encapsulate(tb);
//    prb->setName(tb->getName());
//    sendToChannel(prb);
////    lteFrame->setType(ctrl->getType());
//    frame->setRntiType(ctrl->getRntiType());
//    frame->setRnti(ctrl->getRnti());
////    lteFrame->setRadioType(FDDRadioType);
////    if (!strncmp(this->getParentModule()->getComponentType()->getName(), "UE", 2))
////        lteFrame->setDirection(UplinkDirection);
////    else
////        lteFrame->setDirection(DownlinkDirection);
//    frame->setUeId(ctrl->getUeId());
////    lteFrame->setRapid(ctrl->getRapid());
//    frame->encapsulate(PK(msg));
//    frame->setName(msg->getName());
//    sendToChannel(frame);

//    switch(ctrl->getType()) {
//    case RandomAccessRequest: {
//        AirFrame *frame = new AirFrame(msg->getName());
//        frame->setChannelNumber(ctrl->getChannelNumber());
//        frame->setControlInfo(ctrl);
//        //frame->setRaRnti(ctrl->getRnti());
//        sendToChannel(frame);
//        break;
//    }
//    default:
//        EV << "LTERadio: Unknown LTEPhyControlInfo type. Discarding message.\n";
//        break;
//    }
//    delete msg;
}

void LTERadio::handleRadioMessage(cMessage *msg) {
    PhysicalResourceBlock *prb = check_and_cast<PhysicalResourceBlock*>(msg);
    if (prb->getEncapsulatedPacket()) {
        TransportBlock *tb = check_and_cast<TransportBlock*>(prb->decapsulate());
        if ((tb->getChannel() == BCH) || (tb->getChannel() == DLSCH0)
                || (tb->getChannel() == RACH) || (tb->getUeId() == this->getParentModule()->getId())) {
            if (tb->getChannel() == DLSCH1) {
                checkDCIFormats(tb);
            }
            tb->setTbId(lteSched->getSFN() * 10 + lteSched->getTTI());
            send(tb, gate("upperLayerOut"));
        } else {
            EV << "LTE-Radio: Received unexpected packet on channel = " << tb->getChannel() << ". Dropping it.\n";
        }
        delete prb;
    }

    DCIFormat *dci = dynamic_cast<DCIFormat*>(msg);
    if (dci) {
        dcis.push_back(dci);
    }


//    LTESubFrame *subFrame = check_and_cast<LTESubFrame*>(msg);
//    scheduleAt(simTime() + subFrame->getDuration(), subFrame);
//    if (dynamic_cast<RandomAccessFrame*>(msg)) {
//        RandomAccessFrame *frame = dynamic_cast<RandomAccessFrame*>(msg);
//
//        LTEPhyControlInfo *ctrl = new LTEPhyControlInfo();
//        ctrl->setRnti(frame->getRaRnti());
//        ctrl->setType(RandomAccessRequest);

//    }
//    LTEFrame *frame = check_and_cast<LTEFrame*>(msg);
//    LTEPhyControlInfo *ctrl = new LTEPhyControlInfo();
//    if (frame->getUeId() == ueId || !strncmp(this->getParentModule()->getComponentType()->getName(), "ENB", 3)) {
//        switch (frame->getChannelNumber()) {
//            case PRACH: {
//                RAPreamble *rap = check_and_cast<RAPreamble*>(msg);
//                ctrl->setChannel(RACH);
//                ctrl->setRapid(rap->getRapid());
//                sendUp(frame, ctrl);
//                break;
//            }
//            case PDSCH:
//                ctrl->setChannel(DLSCH);
//                sendUp(frame, ctrl);
//                break;
//            case PUSCH:
//                ctrl->setChannel(ULSCH);
//                scheduleHARQFeedback(HARQ_FEEDBACK_ACK, frame->getUeId());
//                sendUp(frame, ctrl);
//                break;
//            case PHICH:
//                break;
//            default:
//                break;
//        }
//
//    } else
//        delete msg;
}

void LTERadio::checkDCIFormats(TransportBlock *tb) {
    for (DCIFormats::iterator i = dcis.begin(); i != dcis.end();) {
        if ((*i)->getRntiType() == tb->getRntiType() && (*i)->getRnti() == tb->getRnti()) {
            tb->setControlInfo((*i));
            i = dcis.erase(i);
        } else {
            ++i;
        }
    }
}

void LTERadio::sendUp(LTEFrame *frame, LTEPhyControlInfo *ctrl) {
//    //    ctrl->setType(lteFrame->getType());
//        ctrl->setRntiType(frame->getRntiType());
//        ctrl->setRnti(frame->getRnti());
////        ctrl->setRapid(lteFrame->getRapid());
//        ctrl->setUeId(frame->getUeId());
//        cMessage *upMsg = frame->decapsulate();
//        upMsg->setControlInfo(ctrl);
//        send(upMsg, gate("upperLayerOut"));
//
////	EV << "receiving frame " << airframe->getName() << endl;
////	EV << "reception of frame over, preparing to send packet to upper layer\n";
////	sendUp(airframe);
////    delete msg;
}

void LTERadio::scheduleHARQFeedback(bool harqIndicator, unsigned ueId) {
//    HARQFrame *frame = new HARQFrame();
//    harqIndicator == HARQ_FEEDBACK_ACK ? frame->setName("HARQ-ACK") : frame->setName("HARQ-NACK");
//    frame->setHarqIndicator(harqIndicator);
//    frame->setUeId(ueId);
//    frame->setChannelNumber(PHICH);
////    frame->setTtiId(simTime() + HARQ_FEEDBACK_DELAY);
//    this->scheduleAt(simTime() + HARQ_FEEDBACK_DELAY, frame);
////    frame->setContextPointer(this);
////    harqFbs.push_back(frame);
////    sendToChannel(frame);
}

//void LTERadio::sendUp(AirFrame *airframe) {
////    cPacket *frame = airframe->decapsulate();
////    PhyControlInfo *ctrl = new PhyControlInfo();
////    ctrl->setChannelNumber(airframe->getChannelNumber());
////    frame->setControlInfo(ctrl);
////
////    EV << "sending up frame " << frame->getName() << endl;
////    if(airframe->getKind()==control)
////    	send(frame, gate("upperLayerOut"));
////    else
////    	send(frame,gate("netOUT"));
////    delete airframe;
//}

//void LTERadio::sendDown(AirFrame *airframe) {
////	sendToChannel(airframe);
//}
