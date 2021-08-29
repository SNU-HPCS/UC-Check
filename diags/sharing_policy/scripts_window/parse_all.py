#!/usr/bin/python3
from collections import defaultdict
import argparse
import os
import re
import pandas as pd
import multiprocessing
import socket

HOSTNAME = socket.gethostname()

def failed(why):
    print("[FAILED] ", why)
    exit(1)

def main_parser(log_fname):
    global TEST_DIR

    re_pat = "window_(?P<first_numChunk>\d+)_(?P<window>\d+)_pid_(?P<first_pid>\d+)_(?P<second_pid>\d+)"
    re_filter = re.compile(re_pat)
    mobj = re_filter.match(log_fname)
    if mobj == None:
        return None
    res = mobj.groupdict()

    res_dict = {
            'counter_type':[],
            'counter_val':[],
            'prog_type':[],
            'arch_name':[],
            'num_chunk':[],
            'window':[],
            }
    log_fpath = os.path.join(TEST_DIR, log_fname)
    with open(log_fpath) as f:
        for l in f:
            l_list = l.split()
            if len(l_list) < 3:
                continue
            prog_type = ""
            counter_type = ""
            counter_val = 0
            if l_list[2] == "r0479" or l_list[2] == "r01AA":
                counter_type = "legacy_decoder"
            elif l_list[2] == "r0879" or l_list[2] == "r02AA":
                counter_type = "uop_decoder"
            elif l_list[2] == "cycles" or l_list[2] == "instructions":
                counter_type = l_list[2]
            else:
                continue
            try: 
                counter_val = int(l_list[1])
            except:
                counter_val = int(l_list[1].strip().replace(',',''))
                 
            if l_list[0] == "sharing_policy-%s"%(res["first_pid"]):
                prog_type = "first"
            elif l_list[0] == "sharing_policy-%s"%(res["second_pid"]):
                prog_type = "second"
            else:
                failed("Invalid data l_list => %s"%(l_list))

            res_dict['counter_type'].append(counter_type)
            res_dict['counter_val'].append(counter_val)
            res_dict['prog_type'].append(prog_type)
            res_dict['arch_name'].append(HOSTNAME)
            res_dict['num_chunk'].append(res["first_numChunk"])
            res_dict['window'].append(res["window"])

    res_df = pd.DataFrame.from_dict(res_dict)
    return [res_df]


if __name__ == '__main__':
    global TEST_DIR

    parser = argparse.ArgumentParser()
    parser.add_argument('--test_dir', help='a directory including experimental results for specific testcase (MUST be given)')
    parser.add_argument('--csv_dir', help='a directory where result csv file is created (default: ./)')
    ARGS = parser.parse_args()

    if ((ARGS.test_dir == None) or
            (not os.path.isdir(ARGS.test_dir))):
        print("Invalid [test_dir]. Check -h")
        exit(1)

    TEST_DIR = ARGS.test_dir
    if (ARGS.csv_dir == None):
        CSV_DIR = "./"
    elif (os.path.isdir(ARGS.csv_dir)):
        CSV_DIR = ARGS.csv_dir
    else:
        print("Invalid [csv_dir]. check -h")
        exit(1)

	
    OUTPUT_CSV_FNAME = "%s/%s"%(CSV_DIR, TEST_DIR.rstrip('/').split('/')[-1] + '_df.csv')

    tot_df_list = []
    with multiprocessing.Pool(50) as p:
        res_df_list_pool = p.map(main_parser, os.listdir(TEST_DIR))

    for _res_df_list in res_df_list_pool:
        if (_res_df_list == None):
            continue
        tot_df_list += _res_df_list

    final_df = pd.concat(tot_df_list)
    # Save the processed dataframe in csv format
    print("Save csv to", OUTPUT_CSV_FNAME)
    final_df.to_csv(OUTPUT_CSV_FNAME, index=False)
