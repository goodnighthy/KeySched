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
//=============================================================================================================
#include <stdint.h>
#include <std/hash.h>
#include <nfp/me.h>
#include <nfp/mem_atomic.h>
#include <pif_common.h>
#include <pif_plugin.h>
//=============================================================================================================
#define SKETCH_COLUMN_COUNT 1024
#define SKETCH_COLUMN_COUNT_MASK (SKETCH_COLUMN_COUNT-1)

#define PARTITION_COUNT 32
#define PARTITION_COUNT_MASK (PARTITION_COUNT-1)
#define SCHEDULE_COUNT 1024
#define SCHEDULE_COUNT_MASK (SCHEDULE_COUNT-1)
#define HIGH_LEVEL_COUNT 128
#define LOW_LEVEL_COUNT 112
//#define HISTORY_TO_CURRENT_RATIO 0.2

__export __mem static uint32_t sketch[3][SKETCH_COLUMN_COUNT][10] = {0};

__export __mem static uint32_t partition[PARTITION_COUNT][3] = {0};//the current term count, last term and total count
__export __mem static uint32_t schedule[SCHEDULE_COUNT] = {0};
//__export __mem uint32_t lock = 0;
//=============================================================================================================
uint32_t hash_func1(uint32_t key1, uint32_t key2)
{
	return (key1 ^ key2) & SKETCH_COLUMN_COUNT_MASK;
}

uint32_t hash_func2(uint32_t key1, uint32_t key2)
{
	return (key1 | key2) & SKETCH_COLUMN_COUNT_MASK;
}

uint32_t hash_func3(uint32_t key1, uint32_t key2)
{
	return (key1 & key2) & SKETCH_COLUMN_COUNT_MASK;
}

uint32_t hash_partition(uint32_t key1, uint32_t key2)
{
	return key1 & PARTITION_COUNT_MASK;
}

uint32_t hash_schedule(uint32_t key1, uint32_t key2)
{
	return hash_me_crc32((void *)&key2, sizeof(uint32_t), 1) & SCHEDULE_COUNT_MASK;
}
//=============================================================================================================
int pif_plugin_primitive_acm_finder(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *match_data)
{
	__xread uint32_t in_xfer_sketch, in_xfer_term, in_xfer_total, in_xfer_tmp;
	__gpr uint32_t out_reg_sketch0, out_reg_sketch0_acm, out_reg_sketch1, out_reg_sketch1_acm, out_reg_sketch2, out_reg_sketch2_acm, out_reg_sketch_min, out_reg_term, out_reg_total, out_reg_tmp;
	__xwrite uint32_t out_xfer_sketch, out_xfer_term, out_xfer_total, out_xfer_tmp;
	uint32_t key1, key2, hv0, hv1, hv2;
	uint32_t i, timestamp, term, tmp1, tmp2;
	PIF_PLUGIN_req_T *req_header;

	// __xwrite uint32_t xfer_out = 0;
	// __xrw uint32_t xfer = 1;
	// mem_test_set(&xfer, &lock, sizeof(uint32_t));
	// while(xfer == 1)
	// {
	// 	mem_test_set(&xfer, &lock, sizeof(uint32_t));
	// }
	//update sketch
	

	req_header = pif_plugin_hdr_get_req(headers);
	key1 = PIF_HEADER_GET_req___key1(req_header);
	key2 = PIF_HEADER_GET_req___key2(req_header);
	hv0 = hash_func1(key1, key2);
	hv1 = hash_func2(key1, key2);
	hv2 = hash_func3(key1, key2);
	
	timestamp = pif_plugin_meta_get__intrinsic_metadata__ingress_global_tstamp(headers);
	term = (timestamp >> 16) & 0x7;//大约1.5ms

	mem_read32(&in_xfer_sketch, &sketch[0][hv0][term], sizeof(uint32_t));
	mem_read32(&in_xfer_term, &sketch[0][hv0][8], sizeof(uint32_t));
	mem_read32(&in_xfer_total, &sketch[0][hv0][9], sizeof(uint32_t));
	out_reg_sketch0 = in_xfer_sketch;
	out_reg_term = in_xfer_term;
	out_reg_total = in_xfer_total;
	if (term == out_reg_term) {
		out_reg_sketch0 += 1;
		out_reg_sketch0_acm = out_reg_sketch0 + out_reg_total;
		out_xfer_sketch = out_reg_sketch0;
		mem_write32(&out_xfer_sketch, &sketch[0][hv0][term], sizeof(uint32_t));
	} else {
		tmp1 = (term + 8 - out_reg_term) & 0x7;//置0的个数
        tmp2 = 8 - tmp1;//相加的个数
		for(i = 1; i < tmp1; i++) {
			mem_read32(&in_xfer_tmp, &sketch[0][hv0][(out_reg_term + i) & 0x7], sizeof(uint32_t));
			out_reg_tmp = in_xfer_tmp;
			out_reg_tmp = 0;
			out_xfer_tmp = out_reg_tmp;
			mem_write32(&out_xfer_tmp, &sketch[0][hv0][(out_reg_term + i) & 0x7], sizeof(uint32_t));
		}
		out_reg_sketch0 = 1;
		out_xfer_sketch = out_reg_sketch0;
		mem_write32(&out_xfer_sketch, &sketch[0][hv0][term], sizeof(uint32_t));
		out_reg_total = 0;
		for(i = 1; i <= tmp2; i++) {
			mem_read32(&in_xfer_tmp, &sketch[0][hv0][(term + i) & 0x7], sizeof(uint32_t));
			out_reg_tmp = in_xfer_tmp;
			out_reg_total += out_reg_tmp;
		}
		out_reg_sketch0_acm = out_reg_sketch0 + out_reg_total;
		out_xfer_term = term;
		out_xfer_total = out_reg_total;
		mem_write32(&out_xfer_term, &sketch[0][hv0][8], sizeof(uint32_t));
		mem_write32(&out_xfer_total, &sketch[0][hv0][9], sizeof(uint32_t));
	}
	
	
	mem_read32(&in_xfer_sketch, &sketch[1][hv1][term], sizeof(uint32_t));
	mem_read32(&in_xfer_term, &sketch[1][hv1][8], sizeof(uint32_t));
	mem_read32(&in_xfer_total, &sketch[1][hv1][9], sizeof(uint32_t));
	out_reg_sketch1 = in_xfer_sketch;
	out_reg_term = in_xfer_term;
	out_reg_total = in_xfer_total;
	if (term == out_reg_term) {
		out_reg_sketch1 += 1;
		out_reg_sketch1_acm = out_reg_sketch1 + out_reg_total;
		out_xfer_sketch = out_reg_sketch1;
		mem_write32(&out_xfer_sketch, &sketch[1][hv1][term], sizeof(uint32_t));
	} else {
		tmp1 = (term + 8 - out_reg_term) & 0x7;//置0的个数
        tmp2 = 8 - tmp1;//相加的个数
		for(i = 1; i < tmp1; i++) {
			mem_read32(&in_xfer_tmp, &sketch[1][hv1][(out_reg_term + i) & 0x7], sizeof(uint32_t));
			out_reg_tmp = in_xfer_tmp;
			out_reg_tmp = 0;
			out_xfer_tmp = out_reg_tmp;
			mem_write32(&out_xfer_tmp, &sketch[1][hv1][(out_reg_term + i) & 0x7], sizeof(uint32_t));
		}
		out_reg_sketch1 = 1;
		out_xfer_sketch = out_reg_sketch1;
		mem_write32(&out_xfer_sketch, &sketch[1][hv1][term], sizeof(uint32_t));
		out_reg_total = 0;
		for(i = 1; i <= tmp2; i++) {
			mem_read32(&in_xfer_tmp, &sketch[1][hv1][(term + i) & 0x7], sizeof(uint32_t));
			out_reg_tmp = in_xfer_tmp;
			out_reg_total += out_reg_tmp;
		}
		out_reg_sketch1_acm = out_reg_sketch1 + out_reg_total;
		out_xfer_term = term;
		out_xfer_total = out_reg_total;
		mem_write32(&out_xfer_term, &sketch[1][hv1][8], sizeof(uint32_t));
		mem_write32(&out_xfer_total, &sketch[1][hv1][9], sizeof(uint32_t));
	}
	
	
	mem_read32(&in_xfer_sketch, &sketch[2][hv2][term], sizeof(uint32_t));
	mem_read32(&in_xfer_term, &sketch[2][hv2][8], sizeof(uint32_t));
	mem_read32(&in_xfer_total, &sketch[2][hv2][9], sizeof(uint32_t));
	out_reg_sketch2 = in_xfer_sketch;
	out_reg_term = in_xfer_term;
	out_reg_total = in_xfer_total;
	if (term == out_reg_term) {
		out_reg_sketch2 += 1;
		out_reg_sketch2_acm = out_reg_sketch2 + out_reg_total;
		out_xfer_sketch = out_reg_sketch2;
		mem_write32(&out_xfer_sketch, &sketch[2][hv2][term], sizeof(uint32_t));
	} else {
		tmp1 = (term + 8 - out_reg_term) & 0x7;//置0的个数
        tmp2 = 8 - tmp1;//相加的个数
		for(i = 1; i < tmp1; i++) {
			mem_read32(&in_xfer_tmp, &sketch[2][hv2][(out_reg_term + i) & 0x7], sizeof(uint32_t));
			out_reg_tmp = in_xfer_tmp;
			out_reg_tmp = 0;
			out_xfer_tmp = out_reg_tmp;
			mem_write32(&out_xfer_tmp, &sketch[2][hv2][(out_reg_term + i) & 0x7], sizeof(uint32_t));
		}
		out_reg_sketch2 = 1;
		out_xfer_sketch = out_reg_sketch2;
		mem_write32(&out_xfer_sketch, &sketch[2][hv2][term], sizeof(uint32_t));
		out_reg_total = 0;
		for(i = 1; i <= tmp2; i++) {
			mem_read32(&in_xfer_tmp, &sketch[2][hv2][(term + i) & 0x7], sizeof(uint32_t));
			out_reg_tmp = in_xfer_tmp;
			out_reg_total += out_reg_tmp;
		}
		out_reg_sketch2_acm = out_reg_sketch2 + out_reg_total;
		out_xfer_term = term;
		out_xfer_total = out_reg_total;
		mem_write32(&out_xfer_term, &sketch[2][hv2][8], sizeof(uint32_t));
		mem_write32(&out_xfer_total, &sketch[2][hv2][9], sizeof(uint32_t));
	}
	
	
	out_reg_sketch_min = out_reg_sketch0_acm;
	if(out_reg_sketch_min > out_reg_sketch1_acm) { out_reg_sketch_min = out_reg_sketch1_acm; }
	if(out_reg_sketch_min > out_reg_sketch2_acm) { out_reg_sketch_min = out_reg_sketch2_acm; }
	
	
	pif_plugin_meta_set__acm_metadata__acm_count(headers, out_reg_sketch_min);

	//mem_write32(&xfer_out, &lock, sizeof(uint32_t));
	return PIF_PLUGIN_RETURN_FORWARD;
}
void pif_plugin_update_partition_information(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *match_data, uint32_t partition_index)
{
	__xread uint32_t in_xfer_partition, in_xfer_term, in_xfer_total;
	__gpr uint32_t out_reg_partition, out_reg_term, out_reg_total;
	__xwrite uint32_t out_xfer_partition, out_xfer_term, out_xfer_total;
	uint32_t timestamp, term;

	timestamp = pif_plugin_meta_get__intrinsic_metadata__ingress_global_tstamp(headers);
	term = (timestamp >> 16) & 0x7;//大约1.5ms

	mem_read32(&in_xfer_partition, &partition[partition_index][0], sizeof(uint32_t));
	mem_read32(&in_xfer_term, &partition[partition_index][1], sizeof(uint32_t));
	out_reg_partition = in_xfer_partition;
	out_reg_term = in_xfer_term;
	if (term == out_reg_term) {
		out_reg_partition += 1;
		out_xfer_partition = out_reg_partition;
		mem_write32(&out_xfer_partition, &partition[partition_index][0], sizeof(uint32_t));
	} else {
		mem_read32(&in_xfer_total, &partition[partition_index][2], sizeof(uint32_t));
		out_reg_total = in_xfer_total;
		out_reg_total = out_reg_total * 2 / 10 + out_reg_partition * 8 / 10;
		out_reg_partition = 1;
		out_reg_term = term;
		out_xfer_partition = out_reg_partition;
		out_xfer_term = out_reg_term;
		out_xfer_total = out_reg_total;
		mem_write32(&out_xfer_partition, &partition[partition_index][0], sizeof(uint32_t));
		mem_write32(&out_xfer_term, &partition[partition_index][1], sizeof(uint32_t));
		mem_write32(&out_xfer_total, &partition[partition_index][2], sizeof(uint32_t));
	}
}
int pif_plugin_primitive_schedule(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *match_data)
{
	__xread uint32_t in_xfer_schedule, in_xfer_partition, in_xfer_total;
	__gpr uint32_t out_reg_schedule, out_reg_partition, out_reg_total;
	__xwrite uint32_t out_xfer_schedule;
	uint32_t i, count, key1, key2, hp, hs, min_partiton, min_value = 0xffffffff;
	PIF_PLUGIN_req_T *req_header;

	req_header = pif_plugin_hdr_get_req(headers);
	key1 = PIF_HEADER_GET_req___key1(req_header);
	key2 = PIF_HEADER_GET_req___key2(req_header);
	count = pif_plugin_meta_get__acm_metadata__acm_count(headers);
	if (count < LOW_LEVEL_COUNT) {
		hp = hash_partition(key1, key2);
		pif_plugin_meta_set__acm_metadata__acm_partition(headers, hp); 
		pif_plugin_update_partition_information(headers, match_data, hp);
	} else if (count < HIGH_LEVEL_COUNT) {
		hs = hash_schedule(key1, key2);
		mem_read32(&in_xfer_schedule, &schedule[hs], sizeof(uint32_t));
		out_reg_schedule = in_xfer_schedule;
		if (out_reg_schedule == 0) {
			hp = hash_partition(key1, key2);
			pif_plugin_meta_set__acm_metadata__acm_partition(headers, hp);
			pif_plugin_update_partition_information(headers, match_data, hp);
		} else {
			pif_plugin_meta_set__acm_metadata__acm_partition(headers, out_reg_schedule - 1);
			pif_plugin_update_partition_information(headers, match_data, out_reg_schedule - 1);
		}
	} else {
		hs = hash_schedule(key1, key2);
		mem_read32(&in_xfer_schedule, &schedule[hs], sizeof(uint32_t));
		out_reg_schedule = in_xfer_schedule;
		if (out_reg_schedule == 0) {
			for(i = 0; i < PARTITION_COUNT; i++) {
				mem_read32(&in_xfer_partition, &partition[i][0], sizeof(uint32_t));
				mem_read32(&in_xfer_total, &partition[i][2], sizeof(uint32_t));
				out_reg_partition = in_xfer_partition;
				out_reg_total = in_xfer_total;
				if (min_value > (out_reg_partition + out_reg_total)) {
					min_value = out_reg_partition + out_reg_total;
					min_partiton = i;
				}
			}
			out_reg_schedule = min_partiton + 1;
			out_xfer_schedule = out_reg_schedule;
			mem_write32(&out_xfer_schedule, &schedule[hs], sizeof(uint32_t));
			pif_plugin_meta_set__acm_metadata__acm_partition(headers, min_partiton);
			pif_plugin_update_partition_information(headers, match_data, min_partiton);
		} else {
			pif_plugin_meta_set__acm_metadata__acm_partition(headers, out_reg_schedule - 1);
			pif_plugin_update_partition_information(headers, match_data, out_reg_schedule - 1);
		}
	}
	return PIF_PLUGIN_RETURN_FORWARD;
}
