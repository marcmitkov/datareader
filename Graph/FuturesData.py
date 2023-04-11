#!/export/home/wqpackages/local/prod/python/2.7.9/bin/python

import pandas as pd
import logging
import subprocess
from io import StringIO
import os
from os.path import isfile, join
from os import listdir
from functools import partial
import argparse
from multiprocessing import Pool

CMD = "perl /big/svc_wqln/git/Svarga/Chitragupta/read_binary_prices.pl "

COLUMNS = ['SYBL',  'LSTP', 'LSTS', 'INTF', 'INTH', 'INTL', 'INTV', 'NTRD',
           'SPRC', 'SPSQ', 'SPRV', 'ABDE', 'MEDN', 'LSTB', 'LSBS',
           'LSTA', 'LSAS', 'HBID', 'LBID', 'HASK', 'LASK', 'HBSZ',
           'LBSZ', 'HASZ', 'LASZ', 'HLTS', 'TASK', 'TASZ', 'TBID',
           'TBSZ', 'TLSP', 'NBID', 'NASK', 'MLRT', 'TLRT', 'IVAM',
           'INAM', 'INSM', 'MBID', 'MBSZ', 'MASK', 'MASZ']

INTERESTING_COLUMNS = ['D','T','SYBL', 'INTV', 'LSTP', 'LSTS', 'INTF', 'INTH', 'INTL', 'NTRD', 'LSTB',
                       'LSBS', 'LSTA', 'LSAS', 'HBID', 'LBID', 'HASK', 'LASK', 'HBSZ', 'LBSZ', 'HASZ', 'LASZ',
                       'HLTS', 'IVAM']


def _final_result(all_data, instrument_list):
    toret = all_data[all_data['SYBL'].isin(instrument_list)]
    toret["T_int"] = toret["T"].apply(int)
    #return toret.sort_values(["SYBL", "T_int"], ascending=[True, True])
    #toret = toret.sort(["SYBL", "T_int"], ascending=[True, True])
    toret = toret.sort_values(["SYBL", "T_int"], ascending=[True, True])
    return toret[INTERESTING_COLUMNS]


def _read_interval_file(filename, instrument_list, date):
    logging.debug("processing %s" % filename)
    cmd = "%s %s" % (CMD, filename)
    logging.debug(cmd)
    T = filename.split('.')[-2]
    result = subprocess.check_output(cmd, shell=True)
    #df = pd.read_csv(StringIO(unicode(result, "utf-8")), sep=" ", names=COLUMNS)
    df = pd.read_csv(StringIO(str(result, "utf-8")), sep=" ", names=COLUMNS)
    df["T"] = T
    df["D"] = date.strftime('%Y-%m-%d') + " " + str(T[0:2]+":"+T[2:4]+":"+T[4:6])
    #filter by symbols
    filtered = df[df['SYBL'].isin(instrument_list)]
    return filtered


def _get_folder(args, d):
    return os.path.join(args.base_folder, d.strftime("%Y"), d.strftime("%m"), d.strftime("%d"))


def _kickoff(args):
    date = pd.to_datetime(str(args.date), format='%Y%m%d')
    folder = _get_folder(args, date)
    if not os.path.isdir(folder):
	    logging.info("No data found for date %s" % args.date)
	    return
    logging.debug("Base folder : %s" % folder)
    interval_files = [os.path.join(folder,f) for f in listdir(folder) if isfile(join(folder, f)) and f.startswith("interval.")]
    logging.debug("Using %d parallel processes" % args.num_processes)
    process = Pool(args.num_processes)
    f = partial(_read_interval_file, instrument_list=[args.instrument], date=date)
    mm = process.map(f, interval_files)
    #mm = map(f, interval_files)
    all_data = pd.concat(mm)
    final_result = _final_result(all_data, instrument_list=[args.instrument])
    #print('/big/svc_wqln/data/ES/'+str(args.date)[:4]+'/'+str(args.date)[4:6]+'/'+args.instrument+"_"+str(args.date)+".csv")
    final_result.to_csv('/big/svc_wqln/data/Futures/'+str(args.instrument)[:-2]+'/'+str(args.date)[:4]+'/'+str(args.date)[4:6]+'/'+args.instrument+"_"+str(args.date)+".csv", header=True, index=False, na_rep='NA')
    #logging.debug(final_result)
    logging.info("Done")


def main():
    logging.basicConfig(
                        level=logging.INFO,
                        #level=logging.DEBUG,
                        format='%(asctime)s %(levelname)s %(message)s')
    parser = argparse.ArgumentParser()
    parser.add_argument("-b", "--base_folder", default='/dat/futuresbar/GLOBAL/1MinBar/',
                      help="base folder far data: "
                           "/dat/futuresbar/GLOBAL/1MinBar/")
    parser.add_argument('-n', '--num_processes',  type=int, default=20,
                      help='Number of parallel processes. Defaults to 20.')
    parser.add_argument('-d', '--date',  type=int,
                        help='Date as YYYYMMDD')
    parser.add_argument('-i', '--instrument',  type=str,
                        help='Instrument eg. ESU7')
    args = parser.parse_args()
    _kickoff(args)


if __name__ == "__main__":
    main()
