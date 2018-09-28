#!/bin/bash
CTL_SCRIPT=/opt/nfp_pif/scripts/pif_ctl_nfd.sh
FW_FILE=acm.nffw
DESIGN_FILE=./out/pif_design.json
CFG_FILE=user_config.json
sudo /opt/nfp_pif/bin/pif_rte -n 0 -p 20206 -I -z -s $CTL_SCRIPT -f $FW_FILE -d $DESIGN_FILE -c $CFG_FILE --log_file /var/log/nfp-sdk6-rte.log
