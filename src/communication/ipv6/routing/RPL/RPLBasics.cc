/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @author Fabian Krome, Martin Ringwelski, Andreas Weigel
 */

/*INCLUDES-------------------------------------------------------------------*/
#include "RPLBasics.h"
#include "logging.h"

using namespace std;
using namespace cometos;


namespace cometos_v6 {




#define BITS_U8(field,startbit,endbit)     ((((uint8_t)field) >> startbit) & ~(0xff << (endbit - startbit + 1)))
#define BITS_TO_U8(field, startbit) (((uint8_t)field) << startbit)
//Decoding

//Options

//Decode PADN
void decodeOptionPADN(uint16_t &i, const uint8_t *data, uint16_t length){
    i++;
    if(i < length){
        uint8_t numberOfPaddings = data[i];
        i = i + numberOfPaddings;
    }
    if(i >= length){
        ASSERT(i < length);
        return;
    }
}
//Encode PADN
void encodeOptionPADN(uint16_t &i, uint8_t *data, uint8_t padN){
    while(padN){
        data[i] = 0;
        i++;
        padN--;
    }
}
//Decode DAG_METRIC_CONTAINER
void decodeOptionDAG_METRIC_CONTAINER(uint16_t &i, const uint8_t *data, uint16_t length, DIO_Object *currentDIO_info){
    //i++;
//    uint8_t sizeOfMetricContainer = 0;
//    if(i < length){
//        sizeOfMetricContainer = data[i];
//        if(sizeOfMetricContainer != DAG_METRIC_CONTAINER_LENGTH){
//            ASSERT(sizeOfMetricContainer == DAG_METRIC_CONTAINER_LENGTH);
//            return;
//        }
//    }
//    if(i + sizeOfMetricContainer >= length){
//        ASSERT(i + sizeOfMetricContainer < length);
//        return;
//    }
    i++;

    //MC-Type bits 0 to 7
    currentDIO_info->metricContainer.MCType = data[i];
    i++;
    //Res Flags
    currentDIO_info->metricContainer.ResFlags = BITS_U8(data[i],3,7);
    //P Flag
    currentDIO_info->metricContainer.P = BITS_U8(data[i],2,2);
    //C Flag
    currentDIO_info->metricContainer.C = BITS_U8(data[i],1,1);
    //O Flag
    currentDIO_info->metricContainer.O = BITS_U8(data[i],0,0);
    i++;
    //R Flag
    currentDIO_info->metricContainer.R = BITS_U8(data[i],7,7);
    //A Field
    currentDIO_info->metricContainer.AField = BITS_U8(data[i],4,6);
    //Prec Field
    currentDIO_info->metricContainer.PrecField = BITS_U8(data[i],0,3);
    i++;
    //Length
    currentDIO_info->metricContainer.length = data[i];
    i++;
    //Metric option
    if (currentDIO_info->metricContainer.MCType == RPL_DODAG_MC_HOPCOUNT) {
        currentDIO_info->metricContainer.metric.hops = data[i];
    }
    else if (currentDIO_info->metricContainer.MCType == RPL_DODAG_MC_ETX) {
        currentDIO_info->metricContainer.metric.etx = data[i];
    }
    else if (currentDIO_info->metricContainer.MCType == RPL_DODAG_MC_LQL) {
        currentDIO_info->metricContainer.metric.LQL = data[i];
    }
    else {
        //By default the metric is hops
        currentDIO_info->metricContainer.metric.hops = data[i];
    }

}

void encodeOptionDAG_METRIC_CONTAINER(uint16_t &i, uint8_t *data, DAG_MC *metric){

    //MC-Type bits 0 to 7
    data[i] = metric->MCType;
    i++;
    //Res Flags
    data[i] = BITS_TO_U8(metric->ResFlags ,3);
    //P Flag
    data[i] = BITS_TO_U8(metric->P,2);
    //C Flag
    data[i] = BITS_TO_U8(metric->C,1);
    //O Flag
    data[i] = BITS_TO_U8(metric->O,0);
    i++;
    //R Flag
    data[i] = BITS_TO_U8(metric->R,7);
    //A Field
    data[i] = BITS_TO_U8(metric->AField,4);
    //Prec Field
    data[i] = BITS_TO_U8(metric->PrecField,0);
    i++;
    //Length
    data[i] = DAG_METRIC_CONTAINER_LENGTH;
    i++;
    //Metric option
    if (metric->MCType == RPL_DODAG_MC_HOPCOUNT) {
        data[i] = metric->metric.hops;
        i++;
    }
    else if (metric->MCType == RPL_DODAG_MC_ETX) {
        data[i] = metric->metric.etx;
        i++;
    }
    else if (metric->MCType == RPL_DODAG_MC_LQL) {
        data[i] = metric->metric.LQL;
        i++;
    }
    else {
        //By default the metric is hops
        data[i] = metric->metric.hops;
        i++;
    }

}

//Decode ROUTE_INFORMATION
void decodeOptionROUTE_INFORMATION(uint16_t &i, const uint8_t *data, uint16_t length){
    i++;
    uint8_t sizeOfInfo = 0;
    if(i < length){
        sizeOfInfo = data[i];
    }

    if(i + sizeOfInfo >= length){
        ASSERT(i + sizeOfInfo < length);
        return;
    }

    //TODO Route Information
    i = i + sizeOfInfo;

}

void encodeOptionROUTE_INFORMATION(uint16_t &i, uint8_t *data){
    //TODO Route Information
    i++;
}
//Decode DODAG_CONFIGURATION
void decodeOptionDODAG_CONFIGURATION(uint16_t &i, const uint8_t *data, uint16_t length, DIO_Object *currentDIO_info){
    i++;
    uint8_t sizeOfOption = 0;
    if(i < length){
        sizeOfOption = data[i];
        // Has to be DODAG_CONFIG_LENGTH
        if(sizeOfOption != DODAG_CONFIG_LENGTH){
            ASSERT(sizeOfOption == DODAG_CONFIG_LENGTH);
            return;
        }
    }

    if(i + sizeOfOption > length){
        ASSERT(i + sizeOfOption <= length);
        return;
    }

    i++;
    //Securtiy fields:
    //A bits 3 to 3
    currentDIO_info->DAGConfig.authentificationEnable = BITS_U8(data[i],3,3);
    //PCS bits 0 to 2
    currentDIO_info->DAGConfig.PCS = BITS_U8(data[i],0,2);
    i++;
    //Trickle Timer Doublings
    currentDIO_info->DAGConfig.intervalDoublings = data[i];
    i++;
    //Trickle Timer IntMin
    currentDIO_info->DAGConfig.intervalMin = data[i];
    i++;
    //Trickle Timer Redundency
    currentDIO_info->DAGConfig.redundancyConst = data[i];
    i++;
    //Highbyte MaxRankIncrease
    currentDIO_info->DAGConfig.maxRankIncrease = data[i] << 8;
    i++;
    //Lowbyte MaxRankIncrease
    currentDIO_info->DAGConfig.maxRankIncrease += data[i];
    i++;
    //Highbyte MinHopRankIncrease
    currentDIO_info->DAGConfig.minHopRankIncrease = data[i] << 8;
    i++;
    //Lowbyte MinHopRankIncrease
    currentDIO_info->DAGConfig.minHopRankIncrease += data[i];
    i++;
    //Highbyte OCP
    currentDIO_info->DAGConfig.OCP= data[i] << 8;
    i++;
    //Lowbyte OCP
    currentDIO_info->DAGConfig.OCP += data[i];
    i++;
    //Reserved
    i++;
    //Def. Lifetime
    currentDIO_info->DAGConfig.defaultLifetime = data[i];
    i++;
    //Highbyte LifeTime Unit
    currentDIO_info->DAGConfig.lifetimeUnit = data[i] << 8;
    i++;
    //Lowbyte LifeTime Unit
    currentDIO_info->DAGConfig.lifetimeUnit += data[i];

}

void encodeOptionDODAG_CONFIGURATION(uint16_t &i, uint8_t *data, DIO_Object *currentDIO_info){

    data[i] = DODAG_CONFIG_LENGTH;
    i++;

    //Securtiy fields:
    //A bits 3 to 3
    data[i] = BITS_TO_U8(currentDIO_info->DAGConfig.authentificationEnable, 3);
    //PCS bits 0 to 2
    data[i] += BITS_TO_U8(currentDIO_info->DAGConfig.PCS ,0);
    i++;
    //Trickle Timer Doublings
    data[i] = currentDIO_info->DAGConfig.intervalDoublings;
    i++;
    //Trickle Timer IntMin
    data[i] = currentDIO_info->DAGConfig.intervalMin;
    i++;
    //Trickle Timer Redundency
    data[i] = currentDIO_info->DAGConfig.redundancyConst;
    i++;
    //Highbyte MaxRankIncrease
    data[i] = currentDIO_info->DAGConfig.maxRankIncrease >> 8;
    i++;
    //Lowbyte MaxRankIncrease
    data[i] = currentDIO_info->DAGConfig.maxRankIncrease;
    i++;
    //Highbyte MinHopRankIncrease
    data[i] = currentDIO_info->DAGConfig.minHopRankIncrease >> 8;
    i++;
    //Lowbyte MinHopRankIncrease
    data[i] = currentDIO_info->DAGConfig.minHopRankIncrease;
    i++;
    //Highbyte OCP
    data[i] = currentDIO_info->DAGConfig.OCP >> 8;
    i++;
    //Lowbyte OCP
    data[i] = currentDIO_info->DAGConfig.OCP;
    i++;
    //Reserved
    data[i] = 0;
    i++;
    //Def. Lifetime
    data[i] = currentDIO_info->DAGConfig.defaultLifetime;
    i++;
    //Highbyte LifeTime Unit
    data[i] = currentDIO_info->DAGConfig.lifetimeUnit >> 8;
    i++;
    //Lowbyte LifeTime Unit
    data[i] = currentDIO_info->DAGConfig.lifetimeUnit;
    i++;
}

void decodeOptionRPL_TARGET(uint16_t &i, const uint8_t *data, uint16_t length, DAO_Object *currentDAO_info){
    i++;
    uint8_t sizeOfOption = 0;
    if(i < length){
        sizeOfOption = data[i];
        }

    if(i + sizeOfOption >= length){
        ASSERT(i + sizeOfOption < length);
        return;
    }
    i++;
    //flags - unused
    i++;
    //prefix length
    uint8_t prefixLength = data[i];
    //TODO Target input PREFIX, MULTICAST
    if(prefixLength != IP6_BYTES){
        ASSERT(prefixLength == IP6_BYTES);
        return;
    }
    i++;
    //Check if more targets send then possible
    if(currentDAO_info->RPL_TargetSize >= MAX_RPL_TARGETS){
        ASSERT(currentDAO_info->RPL_TargetSize < MAX_RPL_TARGETS);
        return;
    }
    currentDAO_info->RPL_Targets[currentDAO_info->RPL_TargetSize].target = IPv6Address(&data[i]);
    currentDAO_info->RPL_TargetSize++;
    i = i + IP6_BYTES - 1;

}
void encodeOptionRPL_TARGET(uint16_t &i, uint8_t *data, RPL_target *currentRPL_Target){

    //TODO PREFIX - for now always full ip
    uint8_t sizeOfOption = IP6_BYTES;

    //Length of RPL_TARGET option
    data[i] = sizeOfOption + RPL_TARGET_FLAGS_PREFIX;

    i++;
    //flags - unused
    data[i] = 0;


    i++;
    //prefix length
    data[i] = sizeOfOption;

    i++;
    //IPv6Address
    uint16_t i_plus_ip = i;
    while(i < i_plus_ip + IP6_BYTES)
    {
        //2 Bytes read out (range 0-7) 0,1 = 0; 2,3 = 1 etc
        data[i] = (uint8_t)(currentRPL_Target->target.getAddressPart((i - i_plus_ip)/2) >> 8);
        i++;
        data[i] = (uint8_t)((currentRPL_Target->target.getAddressPart((i - i_plus_ip)/2)));
        i++;

    }
}

void decodeOptionTRANSIT_INFORMATION(uint16_t &i, const uint8_t *data, uint16_t length, DAO_Object *currentDAO_info, uint8_t firstTarget){
    i++;
    uint8_t sizeOfOption = 0;
    if(i < length){
        //should be 4 in storing mode
        sizeOfOption = data[i];
        }
    //cout << "sizeOfOptionTRANSIT_INFORMATION :" << (int) sizeOfOption << "\n";
    if(i + sizeOfOption >= length){
        //cout << "length wanted: " << (int)(i + sizeOfOption) << "\n";
        ASSERT(i + sizeOfOption  < length);
        return;
    }
    i++;

    //Check if more Transient informations than possible
    if(currentDAO_info->DAO_TransientInfoSize >= MAX_DAO_TRANSIENT_INFOS){
        ASSERT(currentDAO_info->DAO_TransientInfoSize < MAX_DAO_TRANSIENT_INFOS);
        return;
    }
    //Bit 7
    currentDAO_info->DAO_TransientInfos[currentDAO_info->DAO_TransientInfoSize].E = BITS_U8(data[i],7,7);
    i++;
    currentDAO_info->DAO_TransientInfos[currentDAO_info->DAO_TransientInfoSize].pathControl = data[i];
    i++;
    currentDAO_info->DAO_TransientInfos[currentDAO_info->DAO_TransientInfoSize].pathSequence = data[i];
    i++;
    currentDAO_info->DAO_TransientInfos[currentDAO_info->DAO_TransientInfoSize].pathLifetime = data[i];
    //cout << "decode" << (int) currentDAO_info->DAO_TransientInfos[currentDAO_info->DAO_TransientInfoSize].pathLifetime<< "\n";

    //nonStoring
    if(sizeOfOption != 4){
        i++;
        currentDAO_info->DAO_TransientInfos[currentDAO_info->DAO_TransientInfoSize].parentAddress = IPv6Address(&data[i]);
    }
    currentDAO_info->DAO_TransientInfoSize++;
}

void encodeOptionTRANSIT_INFORMATION(uint16_t &i, uint8_t *data, DAO_TransientInfo *currentTransientInfo, uint8_t MOP){

    uint8_t sizeOfOption;

    ASSERT((MOP) > 0 && (MOP < 4));

    if (MOP == MOP_STORING_MULTICAST || MOP == MOP_STORING_NO_MULTICAST) {
        //should be 4 in storing mode
        sizeOfOption = STORING_TRANSIT_LENGTH;
    }
    else if (MOP == MOP_NON_STORING){
        sizeOfOption = NON_STORING_TRANSIT_LENGTH;
    } else {
        //TODO else{} in order to always initialize sizeOfOption
        sizeOfOption = 0;
        LOG_ERROR("unsupported MOP, invalid sizeOfOption");
        ASSERT(0);
    }


    data[i] = sizeOfOption;
    i++;

    //Bit 7
    data[i] = 0;
    data[i] = BITS_TO_U8(currentTransientInfo->E ,7);
    i++;
    data[i] = currentTransientInfo->pathControl;
    i++;
    data[i] = currentTransientInfo->pathSequence;
    i++;
    data[i] = currentTransientInfo->pathLifetime;
    i++;

    if (MOP == MOP_NON_STORING){
        //Parent IPv6Address
        currentTransientInfo->parentAddress.writeAddress(&(data[i]));
        i += IP6_BYTES;
    }

}


void encodeOptionRPL_TARGET_DESCRIPTOR(uint16_t &i, uint8_t *data, DAO_TransientInfo *currentTransientInfo){

    //Option Length: 4

    data[i] = (uint8_t) 4;
//    The RPL Target Descriptor option is used to qualify a target,
//    something that is sometimes called "tagging".
//    At most, there can be one descriptor per target. The descriptor is
//    set by the node that injects the Target in the RPL network. It MUST
//    be copied but not modified by routers that propagate the Target Up
//    the DODAG in DAO messages.

}

void decodeOptionSOLICITED_INFORMATON(uint16_t &i, const uint8_t *data, uint16_t length){
    //TODO not needed at the moment
}

void decodeOptionPREFIX_INFORMATION(uint16_t &i, const uint8_t *data, uint16_t length){
    i++;
    uint8_t sizeOfOption = 0;
    if(i < length){
        sizeOfOption = data[i];
        // Has to be PREFIX_INFO_LENGTH
        if(sizeOfOption != PREFIX_INFO_LENGTH){
            ASSERT(sizeOfOption == PREFIX_INFO_LENGTH);
            return;
        }
    }

    if(i + sizeOfOption >= length){
        ASSERT(i + sizeOfOption < length);
        return;
    }

    //TODO PREFIX_INFORMATION
}

void encodeOptionPREFIX_INFORMATION(uint16_t &i, const uint8_t *data){
    //TODO PREFIX_INFORMATION
    i++;
}

//TODO not implemented yet
void decodeOptionRPL_TARGET_DESCRIPTOR(uint16_t &i, const uint8_t *data, uint16_t length){
    return;
}


void RPLCodec::decodeDIO(DIO_Object &currentDIO_info, const uint8_t *data, uint16_t length){

    //All needed Data in Message
    if(length < DIO_MIN_LENGTH){
        ASSERT(length >= DIO_MIN_LENGTH);
        return;
    }

    //DECODE DIO-PACKET
    currentDIO_info.RPLinstanceID = data[0];

    currentDIO_info.versionNumber = data[1];

    //ADD
    currentDIO_info.rank = (((uint16_t)data[2]) << 8) + ((uint16_t)data[3]);

    //bit: 7
    currentDIO_info.grounded = BITS_U8(data[4],7,7);

    //bits: 3-5
    currentDIO_info.MOP =  BITS_U8(data[4],3,5);

    //bits 0-2
    currentDIO_info.preference = BITS_U8(data[4],0,2);

    currentDIO_info.DTSN = data[5];

    currentDIO_info.DODAGID = IPv6Address(&data[8]);

    //Options
    if(length >= DIO_MIN_LENGTH){
        uint16_t i = DIO_MIN_LENGTH;
        while(i < length){

            if(data[i] == PAD1){
                i++;
                continue;
            }
            if(data[i] == PADN){
                decodeOptionPADN(i, data, length);
                i++;
                continue;
            }
            if(data[i] == DAG_METRIC_CONTAINER){
                decodeOptionDAG_METRIC_CONTAINER(i, data, length, &currentDIO_info);
                i++;
                continue;
            }
            if(data[i] == ROUTE_INFORMATION){
                decodeOptionROUTE_INFORMATION(i, data, length);
                i++;
            }
            if(data[i] == DODAG_CONFIGURATION){
                decodeOptionDODAG_CONFIGURATION(i, data, length, &currentDIO_info);
                i++;
                continue;
            }
            if(data[i] == PREFIX_INFORMATION){
                decodeOptionPREFIX_INFORMATION(i, data, length);
                i++;
                continue;
            }
            ASSERT(1);
            return;
        }
    }

}

uint16_t RPLCodec::encodeDIO(uint8_t * data, DIO_Object &currentDIO_info, bool pad1, uint8_t padN, bool DAG_MetricContainer, bool routingInfo, bool DODAG_Config, bool prefixInfo){

    //data is DIOBUFFER with size = MAXLENGTH = 500
    uint16_t i = 0;
    //ENCODE DIO-PACKET
    data[i] = currentDIO_info.RPLinstanceID;
    i++;
    data[i] = currentDIO_info.versionNumber;
    i++;

    //ADD
    data[i] = currentDIO_info.rank >> BYTE_SHIFT;
    i++;
    //auto cast
    data[i] = currentDIO_info.rank;
    i++;
    //bit: 7
    data[i] = BITS_TO_U8(currentDIO_info.grounded, 7);

    //bits: 3-5
    data[i] += BITS_TO_U8(currentDIO_info.MOP, 3);

    //bits 0-2
    data[i] += BITS_TO_U8(currentDIO_info.preference, 0);
    i++;

    data[i] = currentDIO_info.DTSN;
    i++;
    data[i] = 0;
    i++;
    data[i] = 0;
    i++;
    //IPv6Address
    currentDIO_info.DODAGID.writeAddress(&(data[i]));
    i += IP6_BYTES;

    //encode Options
    //Options

    if(pad1){
        data[i] = PAD1;
        i++;
    }
    if(padN){
        data[i] = PADN;
        i++;
        encodeOptionPADN(i, data, padN);
    }

    if(DAG_MetricContainer){
        data[i] = DAG_METRIC_CONTAINER;
        i++;
        encodeOptionDAG_METRIC_CONTAINER(i, data, &currentDIO_info.metricContainer);
    }

    if(routingInfo){
        data[i] = ROUTE_INFORMATION;
        i++;
        encodeOptionROUTE_INFORMATION(i, data);
    }

    if(DODAG_Config){
        data[i] = DODAG_CONFIGURATION;
        i++;
        encodeOptionDODAG_CONFIGURATION(i, data, &currentDIO_info);
    }

    if(prefixInfo){
        data[i] = PREFIX_INFORMATION;
        i++;
        encodeOptionPREFIX_INFORMATION(i, data);
    }

    //TODO check for max i!
    return i;
}


void RPLCodec::decodeDAO(DAO_Object &currentDAO_info,  const uint8_t *data, uint16_t length){

    if(length < MIN_DAO_LENGTH){
        ASSERT(length >= MIN_DAO_LENGTH);
        return;
    }

    currentDAO_info.RPL_TargetSize = 0;
    currentDAO_info.DAO_TransientInfoSize = 0;

    currentDAO_info.RPLInstanceID = data[0];

    //bit: 7
    currentDAO_info.k = BITS_U8(data[1],7,7);

    //bit: 6
    currentDAO_info.D = BITS_U8(data[1],6,6);

    currentDAO_info.DAOSequence = data[3];

    //D set -> DODAGId present in package
    if(currentDAO_info.D){
        if(length < IP6_BYTES + MIN_DAO_LENGTH){
            ASSERT(length >= IP6_BYTES + MIN_DAO_LENGTH);
            return;
        }
        currentDAO_info.DODAGID = IPv6Address(&data[8]);
    }

    //Options
    //Target indentifier
    short int firstTarget = -1;

    //cout << "length given :" << (int) length << "\n";


    if(length >=(uint16_t) (MIN_DAO_LENGTH + IP6_BYTES * (uint8_t) currentDAO_info.D)){
        uint16_t i = MIN_DAO_LENGTH + IP6_BYTES * ((uint8_t)currentDAO_info.D);
        while(i < length){
            if(data[i] == PAD1){
                if(firstTarget != -1){
                    i++;
                    continue;
                }else{
                    ASSERT(firstTarget != -1);
                    return;
                }

            }
            if(data[i] == PADN){
                if(firstTarget != -1){
                    decodeOptionPADN(i, data, length);
                    i++;
                    continue;
                }else{
                    ASSERT(firstTarget != -1);
                    return;
                }

            }
            if(data[i] == RPL_TARGET){
                if(firstTarget == -1){
                    firstTarget = currentDAO_info.RPL_TargetSize;
                }

                decodeOptionRPL_TARGET(i, data, length, &currentDAO_info);
                i++;
                //cout << "i after target :" << (int) i << "\n";

            }

            if(data[i] ==  TRANSIT_INFORMATION){
                if(firstTarget == -1){
                    ASSERT(firstTarget != -1);
                }
                decodeOptionTRANSIT_INFORMATION(i, data, length, &currentDAO_info, firstTarget);

                for(uint8_t target_index = firstTarget; target_index < currentDAO_info.RPL_TargetSize; target_index++){
                    currentDAO_info.RPL_Targets[target_index].transientInfoIndex = currentDAO_info.DAO_TransientInfoSize - 1;
                }

                firstTarget = -1;

                i++;
                continue;
            }
            //TODO not implemented so far
            if(data[i] == RPL_TARGET_DESCRIPTOR){
                decodeOptionRPL_TARGET_DESCRIPTOR(i, data, length);
                i++;
                continue;
            }
            //cout << "Test i at end: " << (int) i << " and Data[i] at the end: " <<(int)data[i] <<"\n";
            //ASSERT((int)data[i]);
            return;
        }
    }
}


uint16_t RPLCodec::encodeDAO(DIO_Object &currentDIO_info, DAO_Object &currentDAO_info, uint8_t *data, bool K, bool D, bool pad1, uint8_t padN){

    uint16_t i = 0;
    data[i] = currentDIO_info.RPLinstanceID;
    i++;
    //bit: 7
    data[i] = BITS_TO_U8(K, 7);

    //bit: 6
    data[i] += BITS_TO_U8(D, 6);
    i++;
    data[i] = 0;
    i++;
    data[i] = currentDAO_info.DAOSequence;
    i++;
    //D set -> DODAGId present in package
    if(D){
        //IPv6Address
        currentDIO_info.DODAGID.writeAddress(&(data[i]));
        i += IP6_BYTES;
    }

    //Options
    //Target indentifier
    if(pad1){
        data[i] = PAD1;
        i++;
    }
    if(padN){
        data[i] = PADN;
        i++;
        encodeOptionPADN(i, data, padN);
    }

    //Pack all Targets of particular TransientInfo
    //Add TransientInfo
    for(uint8_t currentTransInfo = 0; currentTransInfo < currentDAO_info.DAO_TransientInfoSize; currentTransInfo++){
        //only add TransientInfo if corresponding Target found else Assert
        bool found = false;

        for(uint8_t currentTargetInfo = 0; currentTargetInfo < currentDAO_info.RPL_TargetSize; currentTargetInfo++){

            //ADD target if it belongs to TransientInfo
            if(currentDAO_info.RPL_Targets[currentTargetInfo].transientInfoIndex == currentTransInfo){
                found = true;
                data[i] = RPL_TARGET;
                i++;
                //ADD Target
                encodeOptionRPL_TARGET(i, data, &currentDAO_info.RPL_Targets[currentTargetInfo]);
            }
        }
        //Add Transit information
        if (found) {
            //cout << "Test i at before encode transit: " << (int) i <<"\n";
            data[i] =  TRANSIT_INFORMATION;
            i++;

            //cout << "El MOP es: " << (int) currentDIO_info.MOP << "\n";
            encodeOptionTRANSIT_INFORMATION(i, data, &currentDAO_info.DAO_TransientInfos[currentTransInfo], currentDIO_info.MOP);

        } else{
            //Return an error
            return 0;
        }
    }


    //TODO not implemented so far
    if(false){
        data[i] = RPL_TARGET_DESCRIPTOR;
        i++;
        //encodeOptionRPL_TARGET_DESCRIPTOR(i, data, &currentDAO_info.DAO_TransientInfos[currentTransInfo]);
    }
    return i;
}



/*
 * RPL_Neighbor
 */
RPLNeighbor::RPLNeighbor():
        isParent(false),
        rank(INFINITE_RANK),
        DODAGVersionNumber(0),
        expireTime(0),
        DAOSequence(0)
{
}

RPLNeighbor::RPLNeighbor(uint16_t rank, uint16_t DODAGVersionNumber,
        DAG_MC metric, IPv6Address ip):
           isParent(false),
           rank(rank),
           DODAGVersionNumber(DODAGVersionNumber),
           metric(metric),
           expireTime(0),
           DAOSequence(0)
{
}

bool RPLNeighbor::getIsParent(){
    return isParent;
}
uint16_t RPLNeighbor::getRank() const{
    return rank;
}
uint16_t RPLNeighbor::getVersion(){
    return DODAGVersionNumber;
}
DAG_MC RPLNeighbor::getMetric() const{
    return metric;
}
IPv6Address& RPLNeighbor::getIpAdress(){
    return ip;
}

uint16_t RPLNeighbor::getExpireTime(){
    return expireTime;
}


void RPLNeighbor::setIsParent(bool isParent){
    this->isParent = isParent;
}
void RPLNeighbor::setRank(uint16_t rank){
    this->rank = rank;
}
void RPLNeighbor::setVersion(uint16_t DODAGVersionNumber){
    this->DODAGVersionNumber = DODAGVersionNumber;
}
void RPLNeighbor::setMetric( DAG_MC metric){
    this->metric = metric;
}
void RPLNeighbor::setIpAdress(IPv6Address ip){
    this->ip = ip;
}


void RPLNeighbor::refreshNeighbor(uint32_t defaultLifetime){
    expireTime = defaultLifetime;
}

bool RPLNeighbor::reduceExpireTime(uint16_t amount){
    if(expireTime > amount){
        expireTime = expireTime - amount;
        return true;
    }else{
        expireTime = 0;
        return false;
    }
}

bool RPLNeighbor::reduceExpireTimeGobal(){
    if(expireTime > ONE_SECOND){
        expireTime = expireTime - ONE_SECOND;
        return true;
    }else{
        expireTime = 0;
        return false;
    }
}


/*
 * RPL_Neighborhood
 */

RPLNeighborhood::RPLNeighborhood():
            size(0),
            prefParentIndex(0),
            defaultLifetime(0),
            timeMultiplier(0)
{
}

RPLNeighborhood::~RPLNeighborhood(){
    removeAllNeighbors();
}

void RPLNeighborhood::init(){
    size = 0;
    prefParentIndex = NO_PREF_PARENT;
    defaultLifetime = EXPIRE_TIME;
}
RPLNeighbor * RPLNeighborhood::getNeighbor(uint8_t index){
    if (index < size){
        return &neighborhood[index];
    } else {
        return NULL;
    }
}
uint8_t RPLNeighborhood::getSize(){
    return size;
}
uint8_t RPLNeighborhood::getPrefParentIndex(){
    return prefParentIndex;
}
RPLNeighbor * RPLNeighborhood::getPrefParent(){
    if(prefParentIndex != NO_PREF_PARENT){
        return &neighborhood[prefParentIndex];
    }else {
        return NULL;
    }
}


// NOTE false if selected neighbor not a parent
bool RPLNeighborhood::setPrefParent(RPLNeighbor *parent){
    return RPLNeighborhood::setPrefParent(findNeighborIndex(parent));
}
bool RPLNeighborhood::setPrefParent(uint8_t index){
    if(index == NOT_A_NEIGHBOR){
        prefParentIndex = NO_PREF_PARENT;
        return false;
    }

    if (index < size){
            prefParentIndex = index;
            if(!neighborhood[index].getIsParent()){
                neighborhood[index].setIsParent(true);
            }
            return true;
    } else {
        prefParentIndex = NO_PREF_PARENT;
        return false;
    }
}

uint8_t RPLNeighborhood::addNeighbor(uint16_t rank, uint16_t DODAGVersionNumber, DAG_MC metric, IPv6Address ip){
    if(size >= MAX_NEIGHBOORHOOD_SIZE){
        return FULL_NEIGHBOORHOOD;
    }
   for(uint8_t i = 0; i < size; i++){
        if(neighborhood[i].getIpAdress() == ip)
            return SAME_IPADDRESS_IN_NEIGHBOORHOOD;
    }
    neighborhood[size].setIpAdress(ip);
    neighborhood[size].setIsParent(false);
    neighborhood[size].setMetric(metric);
    neighborhood[size].setRank(rank);
    neighborhood[size].setVersion(DODAGVersionNumber);

    neighborhood[size].refreshNeighbor(defaultLifetime * timeMultiplier);

    size++;
    return size - 1;


}

bool RPLNeighborhood::removeNeighbor(uint8_t index){
    if (index > size){
        return false;
    }
    //Preferred parent lost
    if(index == prefParentIndex){
        prefParentIndex = NO_PREF_PARENT;
    }
    //adapt to change in the follow
    if(prefParentIndex == size - 1){
        prefParentIndex = index;
    }

    neighborhood[index] = neighborhood[size - 1];
    size--;
    return true;
}

bool RPLNeighborhood::removeAllNeighbors(){
    if(size > 0){
        size = 0;
        return true;
    }
    return false;
}

uint8_t RPLNeighborhood::findNeighborIndex(IPv6Address ip){
    for(uint8_t i = 0; i < size; i++){
        if(neighborhood[i].getIpAdress() == ip){
            return i;
        }
    }
    return NOT_A_NEIGHBOR;
}

uint8_t RPLNeighborhood::findNeighborIndex(RPLNeighbor *neighbor){
    if(neighbor == NULL){
        return NOT_A_NEIGHBOR;
    }
    for(uint8_t i = 0; i < size; i++){
           if(&neighborhood[i] == neighbor){
               return i;
           }
    }

    return NOT_A_NEIGHBOR;
}

bool RPLNeighborhood::reduceGlobalExpireTime(){
    for(uint8_t i = 0; i < size; i++){
        if(!neighborhood[i].reduceExpireTimeGobal()){
            removeNeighbor(i);
            //Change of index order! - size changes as well!

            i--;
        }
    }
    if (getPrefParentIndex() == NO_PREF_PARENT){
        return false;
    }
    return true;
}

void RPLNeighborhood::setDefaultLifetime(uint8_t defaultLifetime){
    this->defaultLifetime = defaultLifetime;

    //TODO reduce Lifetime to MaxValue? - also in change of multiplier?
    /*for(uint8_t i = 0; i < size; i++){
        neighborhood[i].refreshNeighbor(defaultLifetime * timeMultiplier);
    }*/

}

void  RPLNeighborhood::setTimeMultiplier(uint16_t timeMultiplier){
    this->timeMultiplier = timeMultiplier;
}

//Refreshes Neighbor expire time
void RPLNeighborhood::refreshNeighbor(const IPv6Address& ip){
    uint8_t index = findNeighborIndex(ip);
    if(index == NOT_A_NEIGHBOR){
        ASSERT(index!=NOT_A_NEIGHBOR);
    }
    getNeighbor(index)->refreshNeighbor(defaultLifetime * timeMultiplier);
}

}

/*****************************************************************************************************/
