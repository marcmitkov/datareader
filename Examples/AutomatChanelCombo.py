import sys
from multiprocessing import Process
import velidb.automat
import datetime as dt
import os
import argparse
import pandas as pd
import time
import itertools


#Futures = ['ESZ0','NQZ0','CLZ0','GCZ0','YMZ0','ZNZ0','ZBZ0','DX  FMZ0020!','BRN FMZ0020!','Z   FMZ0020!']
Futures = ['ES','NQ','CL','GC','YM','ZN','ZB','DX','BRN','Z','6E','6J','SI','TN','UB','ZF','ZT']

#rho 4 to 8
#minlen 50 to 150 increments of 25
#maxexp: 5 , 7,9 | 500000 700000 1000000

#NOTE:minlen 50 to 150 , step:25
#rho 4,6,8
#sessionstart 2,4,6
#sessionend 21.30, 22.30,23.30
#maxexp: 5,10,15
# aggressor: 0, 1

'''AllparamsCombinations={'FUTURES':{'minlen':'50,75,100,125,150','rho':'4,6,8','sessionStart':'2,4,6','sessionEnd':'21.30,22.30,23.30','maxExposure':'5,10,15','aggressor': '0, 1'},
                'FX': {'minlen':'50,75,100,125,150','rho':'4,6,8','sessionStart':'2,4,6','sessionEnd':'21.30,22.30,23.30','maxExposure':'500000,1000000','aggressor': '0, 1'}
                }'''

#
#Sep-Dec
# Quarterly
FuturesDeliverable = ['ZN','ZB', 'UB','ZT', 'TN','ZF']  #roll U0-Z0  -  Aug 28, 2020  Z0-H1 -  Nov 27, 2020
FuturesCashSettled = ['YM','ES','NQ','6E','6J','DX','Z']  #roll U0-Z0  -  Sep 11, 2020  Z0-H1 -  Dec 18, 2020
FuturesEquityIndex = ['YM','ES','NQ']
FuturesForeignExchange=['6E','6J']
#Monthly (Deliverables)
FuturesMonthly = ['BRN', 'GC', 'CL', 'SI'] #FG H JK M NQ U VX Z
#July 1, Dec 31 2020  period
#NQ U VX Z  for 2020  and FGH for 2021
#GC roll schedule  July 1 - July 27 -  GCQ0   July 28 - ?  -GCZ0
#CL July 1 - Q,  July 15 - U, Aug 18 - V, Sep 18 -X, Oct 16 - Z, ?
#BRN July 1 - U, July 29 - V, Aug 26 - X, Sep 24 - Z

def isFutures(instr):
    isFuture= False
    if instr in FuturesDeliverable:
        isFuture = True
    elif instr in FuturesCashSettled:
        isFuture = True
    elif instr in FuturesMonthly:
        isFuture = True

    return isFuture


rollDatesMap = {}


class rollDatesTuple:
    def __init__(self, rolldate, suffix):
        self.rollDate = rolldate
        self.suffix = suffix


def InitRollDates():
    rolldatesFutures = []  # DeliverableQ ZB UB ZN bond futures

    d = pd.to_datetime('20200828')
    s = 'Z0'
    r = rollDatesTuple(d, s)
    rolldatesFutures.append(r)

    d = pd.to_datetime('20201127')  # roll date from Z0 to H1
    s = 'H1'
    r = rollDatesTuple(d, s)
    rolldatesFutures.append(r)
    d = pd.to_datetime('20210224')  # roll date from H1 to  M1
    s = 'M1'
    r = rollDatesTuple(d, s)
    rolldatesFutures.append(r)

    rollDatesMap['rolldatesFuturesDeliverableQ'] = rolldatesFutures

    # monthly  GC
    rolldatesFutures = []
    r = rollDatesTuple(pd.to_datetime('20201125'), 'G1')
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20210127'), 'J1')
    rolldatesFutures.append(r)
    
    r = rollDatesTuple(pd.to_datetime('20210328'), 'M1')
    rolldatesFutures.append(r)

    rollDatesMap['GC'] = rolldatesFutures

    # monthly  CL
    rolldatesFutures = []
    # prior active was sep
    r = rollDatesTuple(pd.to_datetime('20200618'), 'V0')  # october
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20200918'), 'X0')
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20201016'), 'Z0')
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20201118'), 'F1', )
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20201216'), 'G1')
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20210115'), 'H1')
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20210216'), 'J1')
    rolldatesFutures.append(r)

    r = rollDatesTuple(pd.to_datetime('20210318'), 'K1')
    rolldatesFutures.append(r)

    rollDatesMap['CL'] = rolldatesFutures

    # monthly SI
    rolldatesFutures = []
    # prior active was july
    r = rollDatesTuple(pd.to_datetime('20200628'), 'U0')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20200826'), 'Z0')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20201125'), 'H1')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20210223'), 'K1')
    rolldatesFutures.append(r)
    rollDatesMap['SI'] = rolldatesFutures

    # quarterly  Equity Index
    rolldatesFutures = []
    r = rollDatesTuple(pd.to_datetime('20200910'), 'Z0')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20201210'), 'H1')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20210311'), 'M1')
    rolldatesFutures.append(r)
    rollDatesMap['rolldatesEquityIndexQ'] = rolldatesFutures

    # quarterly Foreign Exchange
    rolldatesFutures = []
    r = rollDatesTuple(pd.to_datetime('20200611'), 'U0')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20200911'), 'Z0')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20201210'), 'H1')
    rolldatesFutures.append(r)
    r = rollDatesTuple(pd.to_datetime('20210310'), 'M1')
    rolldatesFutures.append(r)
    rollDatesMap['rolldatesForeignExchangeQ'] = rolldatesFutures


def GetContract(instr, d):
    dtReq = dt.datetime.strptime(d, '%Y%m%d')

    if instr in FuturesDeliverable:
        suffix = 'U0'  # anything earlier than roll dates available
        m = rollDatesMap['rolldatesFuturesDeliverableQ']
    elif instr in FuturesEquityIndex:
        suffix = 'U0'  # anything earlier than roll dates available
        m = rollDatesMap['rolldatesEquityIndexQ']
    elif instr in FuturesForeignExchange:
        suffix = 'M0'  # anything earlier than roll dates available
        m = rollDatesMap['rolldatesForeignExchangeQ']
    elif instr == 'GC':
        suffix = 'Z0'
        m = rollDatesMap['GC']
    elif instr == 'CL':
        suffix = 'U0'
        m = rollDatesMap['CL']
    elif instr == 'SI':
        suffix = 'N0'
        m = rollDatesMap['SI']
    else:
        print('Instrument is FX')

    for tup in m:
        if dtReq >= tup.rollDate:
            suffix = tup.suffix

    return instr + suffix

def GetQuarterlyContract(instr,d):
    suffix = ''
    dtReq = dt.datetime.strptime(d, '%Y%m%d')
    singleSpace = ' '

    if instr in FuturesDeliverable:
        if dtReq <  pd.to_datetime('20200828'):
            suffix='U0'

        elif dtReq >=  pd.to_datetime('20200828') and dtReq <  pd.to_datetime('20201127'):
            suffix = 'Z0'
        else:
            suffix = 'H1'
    elif instr in FuturesCashSettled:
        if dtReq <  pd.to_datetime('20200911'):
            if instr in ['DX']:
                suffix = singleSpace + singleSpace + 'FMU0020!'
            elif  instr in ['Z']:
                suffix = singleSpace + singleSpace + singleSpace + 'FMU0020!'
            else:
                suffix='U0'
        elif dtReq >=  pd.to_datetime('20200911') and dtReq <  pd.to_datetime('20201218'):
            if instr in ['DX']:
                suffix = singleSpace + singleSpace + 'FMZ0020!'
            elif instr in ['Z']:
                suffix = singleSpace + singleSpace + singleSpace + 'FMZ0020!'
            else:
                suffix = 'Z0'
        else:
            if instr in ['DX']:
                suffix = singleSpace + singleSpace +  'FMH0021!'
            elif instr in ['Z']:
                suffix = singleSpace + singleSpace + singleSpace + 'FMH0021!'
            else:
                suffix = 'H1'
    elif instr in FuturesMonthly:
        print('Fut Monthly')
    else:
        print('Instrument is FX')

    return instr + suffix



def GetFuturesSymbol(instrument,d,rollFile):
    instr= instrument
    if " " in instrument:
        instr = instrument.split()[0]
    rolloverFile = os.path.join(rollFile, instr, '1D', instrument + "_Volume.csv")
    rolloverdf = pd.read_csv(rolloverFile)
    rolloverdf['timestamp'] = pd.to_datetime(rolloverdf['timestamp'])
    rolloverdf.set_index(['timestamp'], inplace=True)

    reqdt = dt.datetime.strptime(d, '%Y%m%d')
    rowmaxname = ''
    pos = rolloverdf.index.get_loc(reqdt )
    if pos != 0:
        row = rolloverdf.iloc[pos - 1]
        if row.max() != 0:
            rowmaxname = row.idxmax()
    else:
        rowmaxname = rolloverdf.columns[0]

    return rowmaxname

#FROM CQG
def GetMonthlyContract(instr,d):
    suffix = ''
    singleSpace = ' '

    dtReq = dt.datetime.strptime(d, '%Y%m%d')
    if instr in ['GC']:
        if dtReq <  pd.to_datetime('20201125'):
            suffix='Z0'
        elif dtReq >= pd.to_datetime('20201125') and dtReq < pd.to_datetime('20210121'):
            suffix = 'G1'
        else:
            suffix='J1' #current

    if instr in ['CL']:
        if dtReq <  pd.to_datetime('20200918'):
            suffix='V0'
        elif dtReq >= pd.to_datetime('20200918') and dtReq < pd.to_datetime('20201016'):
            suffix='X0'
        elif dtReq >= pd.to_datetime('20201016') and dtReq < pd.to_datetime('20201118'):
            suffix='Z0'
        elif dtReq >= pd.to_datetime('20201118') and dtReq < pd.to_datetime('20201217'):
            suffix='F1'
        elif dtReq >= pd.to_datetime('20201217') and dtReq < pd.to_datetime('20210115'):
            suffix='G1'  
        elif dtReq >= pd.to_datetime('20210116') and dtReq < pd.to_datetime('20210216'):
            suffix='H1'
        elif dtReq >= pd.to_datetime('20210216') and dtReq < pd.to_datetime('20210318'):
            suffix='J1'              
        else: #TODO :later dates
            suffix = 'K1'
    if instr in ['BRN']:
        if dtReq < pd.to_datetime('20200924'):
            suffix = singleSpace +  'FMX0020!'
        elif dtReq >= pd.to_datetime('20200925') and dtReq < pd.to_datetime('20201029'):
            suffix = singleSpace + 'FMZ0020!'
        elif dtReq >= pd.to_datetime('20201029') and dtReq < pd.to_datetime('20201125'):
            suffix = singleSpace + 'FMF0021!'
        elif dtReq >= pd.to_datetime('20201125') and dtReq < pd.to_datetime('20201229'):
            suffix = singleSpace + 'FMG0021!'
        elif dtReq >= pd.to_datetime('20201229') and dtReq < pd.to_datetime('20210128'):
            suffix = singleSpace + 'FMH0021!'
        elif dtReq >= pd.to_datetime('20210128') and dtReq < pd.to_datetime('20210222'):
            suffix = singleSpace + 'FMJ0021!'
        else: #TODO :later dates
            suffix = singleSpace + 'FMK0021!'
    if instr in ['SI']:#TODO :later dates
        if dtReq <  pd.to_datetime('20201127'):
            suffix='Z0'
        elif dtReq >= pd.to_datetime('20201217') and dtReq < pd.to_datetime('20210223'):
            suffix='H1'           
        else:
            suffix = 'K1'

    return instr + suffix

def GetParamsVal(instrument, frequency,df):
    #if instrument in Futures:
        #instrument = instrument[:-2]
    if " " in instrument:
        key = instrument.split()[0] + '_' + frequency
    else:
        key = instrument + '_' + frequency
    #return paramsDict[key]
    #z=paramsDictNew[key]
    pval =df.loc[key,'Params']
    pval2=pval.strip()
    #return paramsDictNew[key]
    return pval2



def test_automat(date1, root, resdir,datedir,instrumentNoSuffix,ftbase,ftFilter,ftTickFilter,rollFile,debugValue,outputDir,candles,pc,paramdDirIdx,frequencies,dfAutomatParams,smpEnabled,rmLiquidity):

    print('Test : Running  ' + instrumentNoSuffix + " date: " + date1)

    if isFutures(instrumentNoSuffix):
        instrument = GetContract(instrumentNoSuffix, date1)
    else:
        # Must be FX
        instrument = instrumentNoSuffix

    '''if instrumentNoSuffix in FuturesDeliverable:
        instrument = GetQuarterlyContract(instrumentNoSuffix,date1)
    elif instrumentNoSuffix in FuturesCashSettled:
        instrument = GetQuarterlyContract(instrumentNoSuffix,date1)
    elif instrumentNoSuffix in FuturesMonthly:
        instrument = GetMonthlyContract(instrumentNoSuffix, date1)
    else:
        instrument = instrumentNoSuffix'''

    print('Test : Running  ' + instrument + " date: " + date1)


    btparams = {'STARTDATE': date1,
                'ENDDATE': date1,
                'INSTRUMENTS': '',
                'FX': ''
                }

    #Futures = ['ESZ0']
    if instrumentNoSuffix in Futures:
        btparams['INSTRUMENTS'] = instrument
        # required in backtest.properties to get Futures. Cannot be left blank or invalid FX
        btparams['FX'] = 'EURUSD'
    else:
        btparams['FX'] = instrument

    btparams['FUTBASEPATH'] = ftbase
    btparams['FUTFILTER'] = ftFilter
    btparams['FUTTICKFILTER'] = ftTickFilter
    btparams['SMPENABLED'] = smpEnabled
    btparams['RMLIQUIDITY']=rmLiquidity


    print("backtest params :", btparams)

    modelparams = {
        'VERBOSE': 0,
        'INSTRUMENTS': instruments,
    }

    #frequencies = '1,5'
    modelparams['STRATA'] = instrument
    modelparams['INST_FreqKey'] = instrument + '_Freq'
    modelparams['INST_FreqVal'] = frequencies
    #modelparams['INST_ParamsKey'] = instrument + '_' + frequency + '_Params'
    #paramsVal = GetParamsVal(instrumentNoSuffix, frequency)
    #paramsVal = paramsVal + pc
    #modelparams['INST_ParamsVal'] = paramsVal
    modelparams['OUTPUT_Directory'] = resdir
    modelparams['CHANEL_Debug'] = debugValue
    modelparams['CHANEL_WriteCandles'] = candles
    modelparams['CHANEL_ParamNumber'] = str(paramdDirIdx)

    # <Property name="ESZ0_Freq" value="1,5"/>
    # <Property name="ESZ0_1_Params" value="contracts:1,cushion:0.25"/>
    #<Property name="ESZ0_5_Params" value="contracts:1,cushion:0.25"/>

    freqString=''
    resultsJsonDir='hc-'
    for frequency in frequencies.split(','):
        key = instrument + '_' + frequency + '_Params'
        val = GetParamsVal(instrumentNoSuffix, frequency,dfAutomatParams) + pc
        freqString = freqString + '<Property name="' + key + '"  value="' + val +'"/>\n'
        #subfolder creation for results.json, model, backtest prpty files # e.g hc-1m5m
        resultsJsonDir = resultsJsonDir + frequency +'m'

    modelparams['FREQUENCY_PRPTY'] = freqString
    if isFutures(instrumentNoSuffix):
        modelparams['MARKET_type'] = 'FUTURES'
    else:
        modelparams['MARKET_type'] = 'FX'



    #modelparams['INST_ParamsVal'] = 'minlen:100,rho:6,contracts:10,cushion:0.25,riskfactor:3'

    print("model params :", modelparams)

    #dirname = os.path.join(instrumentNoSuffix.replace('/', ''), datedir)
    dirname = os.path.join(instrumentNoSuffix.replace('/', ''),resultsJsonDir,'params-' + str(paramdDirIdx), datedir)


    '''if " " in instrument:
        instrSplit = instrument.split()[0]
        #dirname = os.path.join(instrSplit, datedir)
        dirname = os.path.join(instrSplit,'1m','params-0', datedir)'''


    #if del:
        #os.remove()

    # results will be in dest + name dir
    auto = velidb.automat.Automat(modbin=root + '/chanelv0_debug', dest=resdir,
                                  modeltemplate= root + '/model-chanel_01Combo.xml',
                                  modelparams=modelparams,
                                  bttemplate=root + '/backtest-chanel_01.properties',
                                  btparams=btparams, name=dirname)

    fpath = os.path.join(resdir, instr.replace('/', ''), resultsJsonDir, 'params-' + str(paramdDirIdx),
                         'params-' + str(paramdDirIdx) + '.txt')
    fhNew = open(fpath, 'w')
    for frequency in frequencies.split(','):
        fhNew.write(frequency + 'm::' + GetParamsVal(instr, frequency,dfAutomatParams) + pc + '\n')
    fhNew.close()
    '''auto.run()
    while auto.poll() is None:  
        auto.wait()
    auto.results()'''

#<Property name="OutputDirectory" value="/home/chanel"/>
def run_automat(date1, root, resdir, datedir, instrumentNoSuffix,ftbase,ftFilter,ftTickFilter,rollFile,debugValue,outputDir,candles,pc,paramdDirIdx,frequencies,dfAutomatParams,smpEnabled,rmLiquidity):


    print('Running  ' + instrumentNoSuffix + " date: " + date1)

    if isFutures(instrumentNoSuffix):
        instrument = GetContract(instrumentNoSuffix, date1)
    else:
        # Must be FX
        instrument = instrumentNoSuffix

    '''if instrumentNoSuffix in FuturesDeliverable:
        instrument = GetQuarterlyContract(instrumentNoSuffix, date1)
    elif instrumentNoSuffix in FuturesCashSettled:
        instrument = GetQuarterlyContract(instrumentNoSuffix, date1)
    elif instrumentNoSuffix in FuturesMonthly:
        instrument = GetMonthlyContract(instrumentNoSuffix, date1)
    else:
        # Must be FX
        instrument = instrumentNoSuffix'''

    print('Running  ' + instrument + " date: " + date1)


    btparams = {'STARTDATE': date1,
                'ENDDATE': date1,
                'INSTRUMENTS': '',
                'FX': ''
                }


    if instrumentNoSuffix in Futures:
        btparams['INSTRUMENTS'] = instrument
        #required in backtest.properties to get Futures. Cannot be left blank or invalid FX
        btparams['FX'] = 'EURUSD'
    else:
        btparams['FX'] = instrument

    # value required  for substitution else will get error
    btparams['FUTBASEPATH'] = ftbase
    btparams['FUTFILTER'] = ftFilter
    btparams['FUTTICKFILTER'] = ftTickFilter
    btparams['SMPENABLED'] = smpEnabled
    btparams['RMLIQUIDITY']=rmLiquidity



    print("backtest params :", btparams)

    modelparams = {
        'VERBOSE': 0,
        'INSTRUMENTS': instruments
        #'ALPHA': i
    }
    #frequencies = '1,5'
    modelparams['STRATA'] = instrument
    modelparams['INST_FreqKey'] = instrument + '_Freq'
    modelparams['INST_FreqVal'] = frequencies
    modelparams['OUTPUT_Directory'] = resdir
    modelparams['CHANEL_Debug'] = debugValue
    modelparams['CHANEL_WriteCandles'] = candles
    modelparams['CHANEL_ParamNumber'] = str(paramdDirIdx)


    # <Property name="ESZ0_Freq" value="1,5"/>
    # <Property name="ESZ0_1_Params" value="contracts:1,cushion:0.25"/>
    # <Property name="ESZ0_5_Params" value="contracts:1,cushion:0.25"/>

    freqString = ''
    resultsJsonDir='hc-'

    for frequency in frequencies.split(','):
        key = instrument + '_' + frequency + '_Params'
        val = GetParamsVal(instrumentNoSuffix, frequency,dfAutomatParams) + pc
        freqString = freqString + '<Property name="' + key + '"  value="' + val + '"/>\n'
        # subfolder creation for results.json, model, backtest prpty files # e.g hc-1m5m
        resultsJsonDir = resultsJsonDir + frequency + 'm'

    modelparams['FREQUENCY_PRPTY'] = freqString

    if isFutures(instrumentNoSuffix):
        modelparams['MARKET_type'] = 'FUTURES'
    else:
        modelparams['MARKET_type'] = 'FX'


    #modelparams['INST_ParamsVal'] = 'minlen:100,rho:6,contracts:10,cushion:0.25,riskfactor:3'

    print("model params :", modelparams)

    '''auto = velidb.automat.automat(modbin=root + '/order_test_debug', dest=resdir, modeltemplate=root + '/model.xml',
                                  modelparams=modelparams, bttemplate=root + '/backtest.properties',
                                  btparams=btparams)'''

    #dirname = os.path.join(instrumentNoSuffix.replace('/', ''), datedir)
    dirname = os.path.join(instrumentNoSuffix.replace('/', ''),resultsJsonDir,'params-' + str(paramdDirIdx), datedir)

    '''if " " in instrument:
        instrSplit = instrument.split()[0]
        #dirname = os.path.join(instrSplit, datedir)
        dirname = os.path.join(instrSplit,'1m','params-0', datedir)'''


    # results will be in dest + name dir
    auto = velidb.automat.Automat(modbin=root + '/chanelv0_debug', dest=resdir,
                                  modeltemplate=root + '/model-chanel_01Combo.xml',
                                  modelparams=modelparams,
                                  bttemplate=root + '/backtest-chanel_01.properties',
                                  btparams=btparams, name=dirname)

    fpath = os.path.join(resdir, instr.replace('/', ''), resultsJsonDir, 'params-' + str(paramdDirIdx),
                         'params-' + str(paramdDirIdx) + '.txt')
    fhNew = open(fpath, 'w')
    for frequency in frequencies.split(','):
        fhNew.write(frequency + 'm::' + GetParamsVal(instr, frequency,dfAutomatParams) + pc +'\n')
    fhNew.close()

    auto.run()
    while auto.poll() is None:
        auto.wait()
    auto.results()
    #to_csv(

def createParamsString(p):
    paramsStr=''
    for i in range(0, len(p), 2):
        #print(p[i])
        #print(p[i + 1])
        #print('a :', paramsStr)
        paramsStr = paramsStr + ','
        paramsStr= paramsStr + p[i] + ':' + p[i + 1]
        #print('b :',paramsStr)
    print('final: ',paramsStr)
    return paramsStr

def get_params_input(paramsDict):

    arguments=[]
    for key,val  in paramsDict.items():
        #print(key, '::', val)
        #if not (key in excludeList):
            #print(key)
        if val != None :
            arguments.append([key])
            valList =val.split(',')
            #check if any element is empty e.g empty minlen second element 'FX': {'minlen':'50,','rho':'4'}
            if not all(valList):
                print('empty element in  {}:{} '.format(key,valList))
                exit(1)
            #arguments.append(val.split(','))
            arguments.append(valList)

            print(key + '::' + val )

    return arguments

if __name__ == '__main__':

    #scriptStartTime = time.time()
    parser = argparse.ArgumentParser(description="Automat Chanel Script")
    parser.add_argument('-f', '--startDate', default='20201001', help="from date")
    parser.add_argument('-t', '--endDate', default='20201002', help="to date")
    parser.add_argument('-root', '--root', default='/home/mrai/input', help="exe and template location")
    #parser.add_argument('-template', '--template', default='/home/mrai/Scripts', help="to date")

    parser.add_argument('-resultsDir', '--resultsDir', default='/home/mrai/Results', help="to date")
    parser.add_argument('-i', '--instrumentList', default=['ESZ0','EURUSD'], help="contract",nargs='*')
    parser.add_argument('-ftbase', '--ftbase', default='/history/AUR1/CME', help="contract")
    parser.add_argument('-ftFilter', '--ftFilter', default='CME_FUT', help="CME_FUT or ICE")
    parser.add_argument('-ftTickFilter', '--ftTickFilter', default='CME', help="CME or ICE")
    parser.add_argument('-rollFile', '--rollFile', default='/home/mrai/output/Volumezz/ConsolidatedRollover.csv', help="to date")
    parser.add_argument('-chanelDebug', '--chanelDebug', default='false', help="true or false")
    parser.add_argument('-outputDir', '--outputDir', default='/home/mrai/Results/', help="outputDir for positions")
    parser.add_argument('-candles', '--candles', action='store_true', help="bool flag, specify to create candles")
    parser.add_argument('-frequency', '--frequency', default='1', help="1,5")
    parser.add_argument('-smpEnabled', '--smpEnabled', default='false', help="true or false")
    parser.add_argument('-rmLiquidity', '--rmLiquidity', default='true', help="true or false")
    parser.add_argument('-quantum', '--quantum', action='store_true', help="bool flag, specify to use AutomatParamsQuantum.txt")
    parser.add_argument('-test', '--test', action='store_true', help="bool flag, specify if testing")

    args = parser.parse_args()
    print(args)

    # root = '/home/bpeng'
    start=args.startDate
    end = args.endDate
    #root = '/home/mrai/sw/chanel/chanel-core/src/dist/bin/debug'
    root=args.root
    #resdir = '/home/mrai/ResultsChanel'
    resdir =args.resultsDir
    #templateDir=args

    InitRollDates()

    dfAutomatComboParams = pd.read_csv(os.path.join(args.root,'AutomatParamsCombo.txt'),sep='-',names=['Type','Params'], index_col='Type')

    if args.quantum:
        dfAutomatParams = pd.read_csv(os.path.join(args.root, 'AutomatParamsQuantum.txt'), sep=';',
                                      names=['Instrument', 'Params'], index_col='Instrument')

    else:
        dfAutomatParams = pd.read_csv(os.path.join(args.root, 'AutomatParams.txt'), sep=';',
                                      names=['Instrument', 'Params'], index_col='Instrument')

    #pd.Series(dfAutomatParams.Params.values, index=dfAutomatParams.Instrument).to_dict()


    startDate = dt.datetime.strptime(args.startDate, '%Y%m%d')
    endDate = dt.datetime.strptime(args.endDate, '%Y%m%d')
    delta = endDate - startDate  # timedelta
    dateInputList = []
    for i in range(delta.days + 1):
        #TODO:skip if holiday
        dtToAdd = startDate + dt.timedelta(days=i)
        # skip Sunday
        if (dtToAdd.weekday() >= 5):
            continue;
        dateInputList.append((startDate + dt.timedelta(days=i)).strftime("%Y%m%d"))

    instruments = args.instrumentList
    if args.candles:
        candles = "true"
    else:
        candles = "false"

    #dfMonthlyContract = pd.DataFrame()
    #if any( item in instruments for item in FuturesMonthly):
       # dfMonthlyContract=pd.read_csv(args.rollFile)

    jobs = []
    for instr in instruments:
        print("Start :",instr)
        if instr in itertools.chain(FuturesMonthly,FuturesDeliverable, FuturesCashSettled):
            #pinput = paramsCombinations['FUTURES']
            s =dfAutomatComboParams.loc['FUTURES','Params']
            pinput=dict(item.split(':') for item in s.split(';'))

        else:
            #pinputz = paramsCombinations['FX']
            s =dfAutomatComboParams.loc['FX','Params']
            pinput=dict(item.split(':') for item in s.split(';'))

        #inputz = get_params_input(pinputz)
        input = get_params_input(pinput)

        product = list(itertools.product(*input))
        paramsCombo=[]
        for (idx, p) in enumerate(product):
            print("p: ", p)
            val = createParamsString(p)
            paramsCombo.append(val)
        paramdDirIdx = 0
        print("Number of params combinations: ", len(paramsCombo))
        for pc in paramsCombo:
            for d in dateInputList:
                print("Running {} date {} with paramIdx {} params{}".format(instr,d,paramdDirIdx,pc))
                if args.test:
                    #print('Test Run')
                    test_automat(d, root, resdir, d, instr, args.ftbase, args.ftFilter, args.ftTickFilter,
                                 args.rollFile, args.chanelDebug, args.outputDir, candles, pc,paramdDirIdx,args.frequency,dfAutomatParams,args.smpEnabled,args.rmLiquidity)
                else:
                    p = Process(target=run_automat, args=(
                    d, root, resdir, d, instr, args.ftbase, args.ftFilter, args.ftTickFilter, args.rollFile,
                    args.chanelDebug, args.outputDir, candles,pc,paramdDirIdx,args.frequency,dfAutomatParams,args.smpEnabled,args.rmLiquidity))
                    jobs.append(p)
                    p.start()
            '''fpath = os.path.join(resdir, instr.replace('/', ''), 'hc-1m5m', 'params-' + str(paramdDirIdx),'params-'+str(paramdDirIdx)+'.txt')
            fhNew = open(fpath, 'w')
            fhNew.write('1m::'+GetParamsVal(instr, '1')+ pc)
            fhNew.write('5m::' +GetParamsVal(instr, '5')+ pc)
            fhNew.close()'''
            print("All dates done for {}  params-{} params:{}".format(instr,paramdDirIdx,pc))
            paramdDirIdx = paramdDirIdx + 1

    print('Run completed')
    exit(0)
    #date_list = ['20201001', '20201002']

