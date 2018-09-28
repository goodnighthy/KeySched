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
#define SKETCH_COLUMN_COUNT 1024 //slots number of count-min sketch
#define SKETCH_COLUMN_COUNT_MASK (SKETCH_COLUMN_COUNT-1)

#define PARTITION_COUNT 8 //the core of key-value store
#define PARTITION_COUNT_MASK (PARTITION_COUNT-1)
#define SCHEDULE_COUNT 128 //entries number of schedule mapping table
#define SCHEDULE_COUNT_MASK (SCHEDULE_COUNT-1)
#define OVERLOAD 32 //heavy key detect threshold

__export __mem static uint32_t sketch[3][SKETCH_COLUMN_COUNT][2] = {0}; //count and timeslot stored in each entry
__export __mem static uint32_t partition[PARTITION_COUNT] = {0}; //load of each core
__export __mem static uint32_t schedule[SCHEDULE_COUNT][2] = {0}; //mapping and timeslot stored in each entry
//=============================================================================================================
uint32_t hash_func1(uint32_t key1)
{
	return (((key1 >> 8) & 0xfff) ^ (key1 >> 20)) & SKETCH_COLUMN_COUNT_MASK;
}

uint32_t hash_func2(uint32_t key1)
{
	return (((key1 >> 8) & 0xfff) | (key1 >> 20)) & SKETCH_COLUMN_COUNT_MASK;
}

uint32_t hash_func3(uint32_t key1)
{
	return (((key1 >> 8) & 0xfff) & (key1 >> 20)) & SKETCH_COLUMN_COUNT_MASK;
}

uint32_t hash_partition(uint32_t key1)
{
	return (key1 >> 16) & PARTITION_COUNT_MASK;
}

uint32_t hash_schedule(uint32_t key1)
{
	return hash_me_crc32((void *)&key1, sizeof(uint32_t), 1) & SCHEDULE_COUNT_MASK;
}
//=============================================================================================================
int pif_plugin_primitive_cm_finder(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *match_data)
{
	__xread uint32_t in_xfer_sketch, in_xfer_term;
	__gpr uint32_t out_reg_sketch0, out_reg_sketch1, out_reg_sketch2, out_reg_sketch_min, out_reg_term;
	__xwrite uint32_t out_xfer_sketch, out_xfer_term;
	uint32_t key1, hv0, hv1, hv2;
	uint32_t timestamp, term;
	PIF_PLUGIN_req_T *req_header;
	

	req_header = pif_plugin_hdr_get_req(headers);
	key1 = PIF_HEADER_GET_req___key1(req_header);
	hv0 = hash_func1(key1);
	hv1 = hash_func2(key1);
	hv2 = hash_func3(key1);
	
	timestamp = pif_plugin_meta_get__intrinsic_metadata__ingress_global_tstamp(headers);
	term = (timestamp >> 21) & 0x7; //about 1.5ms

	mem_read32(&in_xfer_sketch, &sketch[0][hv0][0], sizeof(uint32_t));
	mem_read32(&in_xfer_term, &sketch[0][hv0][1], sizeof(uint32_t));
	out_reg_sketch0 = in_xfer_sketch;
	out_reg_term = in_xfer_term;
	if (term == out_reg_term) {
		out_reg_sketch0 += 1;
		out_xfer_sketch = out_reg_sketch0;
		mem_write32(&out_xfer_sketch, &sketch[0][hv0][0], sizeof(uint32_t));
	} else {
		out_reg_sketch0 = 1;
		out_xfer_sketch = out_reg_sketch0;
		mem_write32(&out_xfer_sketch, &sketch[0][hv0][0], sizeof(uint32_t));
		out_xfer_term = term;
		mem_write32(&out_xfer_term, &sketch[0][hv0][1], sizeof(uint32_t));
	}
	
	
	mem_read32(&in_xfer_sketch, &sketch[1][hv1][0], sizeof(uint32_t));
	mem_read32(&in_xfer_term, &sketch[1][hv1][1], sizeof(uint32_t));
	out_reg_sketch1 = in_xfer_sketch;
	out_reg_term = in_xfer_term;
	if (term == out_reg_term) {
		out_reg_sketch1 += 1;
		out_xfer_sketch = out_reg_sketch1;
		mem_write32(&out_xfer_sketch, &sketch[1][hv1][0], sizeof(uint32_t));
	} else {
		out_reg_sketch1 = 1;
		out_xfer_sketch = out_reg_sketch1;
		mem_write32(&out_xfer_sketch, &sketch[1][hv1][0], sizeof(uint32_t));
		out_xfer_term = term;
		mem_write32(&out_xfer_term, &sketch[1][hv1][1], sizeof(uint32_t));
	}
	
	
	mem_read32(&in_xfer_sketch, &sketch[2][hv2][0], sizeof(uint32_t));
	mem_read32(&in_xfer_term, &sketch[2][hv2][1], sizeof(uint32_t));
	out_reg_sketch2 = in_xfer_sketch;
	out_reg_term = in_xfer_term;
	if (term == out_reg_term) {
		out_reg_sketch2 += 1;
		out_xfer_sketch = out_reg_sketch2;
		mem_write32(&out_xfer_sketch, &sketch[2][hv2][0], sizeof(uint32_t));
	} else {
		out_reg_sketch2 = 1;
		out_xfer_sketch = out_reg_sketch2;
		mem_write32(&out_xfer_sketch, &sketch[2][hv2][0], sizeof(uint32_t));
		out_xfer_term = term;
		mem_write32(&out_xfer_term, &sketch[2][hv2][1], sizeof(uint32_t));
	}
	
	
	out_reg_sketch_min = out_reg_sketch0;
	if(out_reg_sketch_min > out_reg_sketch1) { out_reg_sketch_min = out_reg_sketch1; }
	if(out_reg_sketch_min > out_reg_sketch2) { out_reg_sketch_min = out_reg_sketch2; }
	
	
	pif_plugin_meta_set__cm_metadata__cm_count(headers, out_reg_sketch_min);

	return PIF_PLUGIN_RETURN_FORWARD;
}

int pif_plugin_primitive_schedule(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *match_data)
{
	__xread uint32_t in_xfer_schedule, in_xfer_partition, in_xfer_term;
	__gpr uint32_t out_reg_schedule, out_reg_partition, out_reg_term;
	__xwrite uint32_t out_xfer_schedule, out_xfer_partition, out_xfer_term;
	uint32_t i, count, key1, hp, hs, timestamp, term, min_partiton, min_value = 0xffffffff;
	PIF_PLUGIN_req_T *req_header;

	req_header = pif_plugin_hdr_get_req(headers);
	key1 = PIF_HEADER_GET_req___key1(req_header);
	count = pif_plugin_meta_get__cm_metadata__cm_count(headers);
	if (count < OVERLOAD) {
		hp = hash_partition(key1);
		pif_plugin_meta_set__cm_metadata__cm_partition(headers, hp); 
		mem_read32(&in_xfer_partition, &partition[hp], sizeof(uint32_t));
		out_reg_partition = in_xfer_partition;
		out_reg_partition += 1;
		out_xfer_partition = out_reg_partition;
		mem_write32(&out_xfer_partition, &partition[hp], sizeof(uint32_t));
	} else {
		timestamp = pif_plugin_meta_get__intrinsic_metadata__ingress_global_tstamp(headers);
		term = (timestamp >> 21) & 0x7; //about 1.5ms
		hs = hash_schedule(key1);
		mem_read32(&in_xfer_schedule, &schedule[hs][0], sizeof(uint32_t));
		out_reg_schedule = in_xfer_schedule;
		mem_read32(&in_xfer_term, &schedule[hs][1], sizeof(uint32_t));
		out_reg_term = in_xfer_term;
		if (term == out_reg_term) {
			pif_plugin_meta_set__cm_metadata__cm_partition(headers, out_reg_schedule); 
			mem_read32(&in_xfer_partition, &partition[out_reg_schedule], sizeof(uint32_t));
			out_reg_partition = in_xfer_partition;
			out_reg_partition += 1;
			out_xfer_partition = out_reg_partition;
			mem_write32(&out_xfer_partition, &partition[out_reg_schedule], sizeof(uint32_t));
		} else {
			for(i = 0; i < PARTITION_COUNT; i++) {
				mem_read32(&in_xfer_partition, &partition[i], sizeof(uint32_t));
				out_reg_partition = in_xfer_partition;
				if (min_value > out_reg_partition) {
					min_value = out_reg_partition;
					min_partiton = i;
				}
			}
			pif_plugin_meta_set__cm_metadata__cm_partition(headers, min_partiton);
			mem_read32(&in_xfer_partition, &partition[min_partiton], sizeof(uint32_t));
			out_reg_partition = in_xfer_partition;
			out_reg_partition += 1;
			out_xfer_partition = out_reg_partition;
			mem_write32(&out_xfer_partition, &partition[min_partiton], sizeof(uint32_t));
			out_reg_schedule = min_partiton;
			out_xfer_schedule = out_reg_schedule;
			mem_write32(&out_xfer_schedule, &schedule[hs][0], sizeof(uint32_t));
			out_reg_term = term;
			out_xfer_term = out_reg_term;
			mem_write32(&out_xfer_term, &schedule[hs][1], sizeof(uint32_t));
		}	
	}
	return PIF_PLUGIN_RETURN_FORWARD;
}