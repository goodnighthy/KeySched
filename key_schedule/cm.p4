/*
 * Copyright (C) 2017, ACANETS LAB, University of Massachusetts Lowell
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#define ETHERTYPE_IPV4 0x0800
#define TCP_PROTO 0x06
#define UDP_PROTO 0x11

//==========================================================================================================
//Header
//==========================================================================================================
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}
header ethernet_t ethernet;
//==========================================================================================================
header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr : 32;
    }
}
header ipv4_t ipv4;
//==========================================================================================================
header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}
header tcp_t tcp;
//==========================================================================================================
header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        udplen : 16;
        udpchk : 16;
    }
}
header udp_t udp;
//==========================================================================================================
header_type req_t {
    fields {
        key1 : 32;
        key2 : 32;
    }
}
header req_t req;
//==========================================================================================================
header_type cm_metadata_t {
    fields {
        cm_count : 32; //count of this key
        cm_partition: 32; //assigned core of this key
    }
}
metadata cm_metadata_t cm_metadata;
//==========================================================================================================
header_type intrinsic_metadata_t {
    fields {
        ingress_global_tstamp : 32;
    }
}
metadata intrinsic_metadata_t intrinsic_metadata;
//==========================================================================================================
//Parser
//==========================================================================================================
parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        //No default, so drop it if not ipv4 packet
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.protocol) {
        TCP_PROTO : parse_tcp;
        UDP_PROTO : parse_udp;
        //No default, so drop it if not tcp or udp
    }
}

parser parse_tcp {
    extract(tcp);
    return parse_req; 
}

parser parse_udp {
    extract(udp);
    return parse_req; 
}

parser parse_req {
    extract(req);
    return ingress;
}
//==========================================================================================================
//Ingress
//==========================================================================================================
primitive_action primitive_cm_finder();
action do_cm_finder() { 
    primitive_cm_finder();
    modify_field(req.key1, cm_metadata.cm_count); //for test and comment off this bank will make a mistake
}
@pragma netro no_lookup_caching do_cm_finder;
table cm_finder {
    actions {
        do_cm_finder;
    }
}
//==========================================================================================================
primitive_action primitive_schedule();
action do_schedule() {
    primitive_schedule();
    modify_field(req.key2, cm_metadata.cm_partition); //for test and comment off this bank will make a mistake
}
@pragma netro no_lookup_caching do_schedule;
table schedule {
    actions {
        do_schedule;
    }
}
//==========================================================================================================
action do_forward(port) {
    modify_field(standard_metadata.egress_spec, port);
}

action do_drop()
{
    drop();
}

table forward {
    reads {
        standard_metadata.ingress_port : exact;
    }
    actions {
        do_forward;
        do_drop;
    }
}
//==========================================================================================================
control ingress {
    if (standard_metadata.ingress_port == 1) {
        apply(cm_finder);
        apply(schedule);
    }
    apply(forward);
}

//==========================================================================================================
//Egress
//==========================================================================================================