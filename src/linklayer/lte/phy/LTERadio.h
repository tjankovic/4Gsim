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

#ifndef LTERADIO_H_
#define LTERADIO_H_

#include "ChannelAccess.h"
#include "RadioState.h"
#include "AirFrame_m.h"
#include "IRadioModel.h"
#include "IReceptionModel.h"
#include "SnrList.h"
#include "ObstacleControl.h"
#include "IPowerControl.h"
#include "INoiseGenerator.h"
#include "InterfaceTable.h"
#include "LTEPhyControlInfo_m.h"

enum kind {
	control = 0,
	user = 1
};

enum LTEChannels {
    PRACH,
    PDCCH,
};

enum LTEDevices {
    ENB = 0,
    UE = 1
};

class LTERadio : public ChannelAccess {
public:
	LTERadio();
	virtual ~LTERadio();
protected:
	RadioState rs;

    IRadioModel *radioModel;
    IReceptionModel *receptionModel;
    IInterfaceTable *ift;

	virtual IReceptionModel *createReceptionModel() {return (IReceptionModel *)createOne("FreeSpaceModel");}
    virtual IRadioModel *createRadioModel() {return (IRadioModel *)createOne("LTERadioModel");}
	virtual int numInitStages() const {
		return 5;
	}
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

    virtual void handleUpperMessage(cMessage *msg);
    virtual void handleRadioMessage(cMessage *msg);

    //virtual AirFrame *encapsulatePacket(cPacket *frame);

    void sendToRadio(cMessage *msg, int channel);

    //virtual void sendDown(AirFrame *airframe);
    virtual void sendUp(AirFrame *airframe);
};

#endif /* LTERADIO_H_ */
