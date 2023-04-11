#!/big/svc_wqln/projects/python/conda/bin/python3.6
#https://www.geeksforgeeks.org/xml-parsing-python/

#1.Read alphas file into a dataframe
#2.For each alpha above:
#  a) CreateDataFile() will get datafile from baseDataDir , filter data based on start and end date and create the data file
#     that will be added to <File> node in config.xml File will be located in paramsDir/AssetName,Frequency,Params from alphas file
#  b) Copy params.xml and config.xml from baseDirA or baseDirB into folders created
#  c) create paramlist for TestSimulator  and add to cmds list
#3. make multiprocessing call to TestSimulator
#4. Output from testSimulator in same folder where params.xml and config.xml are copied (baseOut/<AssetName>/<Frequency>/<Param>)

#ARGS DESCRIPTION
#baseOut: Output Directory to create alphas folders (baseOut/<AssetName>/<Frequency>/<Param>) where config.xml and params.xml
#         will be copied. Output from TestSimulator will be in these folders
#ex: C=++ TestSimulator Exe Dir
#alpha:alphas file  with path
#s:start date
#e:end date


import pandas as pd
import argparse
import os
import shutil
import Common as co
import multiprocessing
from functools import partial
from datetime import datetime,timedelta
import xml.etree.ElementTree as et
import numpy as np
from pandas.tseries.offsets import BDay
import logging

simulatorExeDir = ''
currencyList = ["EURUSD", "AUDUSD", "GBPUSD", "EURGBP", "EURJPY", "USDCAD","USDCHF", "USDJPY"]
StrategyBMap ={'Keynes':['CME_TUU7','CME_USU7'], 'Buffet':['CME_ESU7','CME_1YMU7'], 'Smith':['CME_ESU7','CME_NQU7'],
               'Nash':['CME_TYU7','CME_USU7'], 'Friedman':['CME_TUU7','CME_TYU7'], 'Hayek':['CME_FVU7','CME_TYU7'],
               'Marx':['CME_FVU7','CME_USU7'],'Tinbergen':[],'Kondratiev':['CME_CLU7','CME_LCOU7'], 'Bernanke': ['CME_TNU7', 'CME_AULU7']}
def CreateDataFile(asset, frequency, srcDataDir,paramDir, dataDir,sDate, eDate):
    #fromDate= '20170903 22:03:00'
    #toDate = '20170904 01:38:00'
    filePath =[]
    legs=[]

    if asset in StrategyBMap:
        legsArr = StrategyBMap[asset]
        legs.append(legsArr[0].split('_')[1][:-2])
        legs.append(legsArr[1].split('_')[1][:-2])
        filePath.append(os.path.join(srcDataDir, "Futures", "Live", legs[0] + legs[1], legs[0] + ".csv"))
        filePath.append(os.path.join(srcDataDir, "Futures", "Live", legs[0] + legs[1], legs[1] + ".csv"))
    else:
        if asset in currencyList:
            filePath.append(os.path.join(srcDataDir,  "FX","Live",asset, frequency + "_resample.csv"))
        else:
            filePath.append(os.path.join(srcDataDir, "Futures","Live",asset, frequency + "_resample.csv"))
        legs.append(asset)

    for idxFile, val in enumerate(filePath):
        #now = datetime.now()
        #print(now.strftime("%Y%m%d"))
        #startDate = now - timedelta(2)
        ##startDate = now - BDay(2)
        #print(startDate.strftime("%Y%m%d"))
        #endDate= now - timedelta(1)
        ##endDate = now - BDay(1)

        startDate = pd.to_datetime(sDate)
        endDate = pd.to_datetime(eDate)

        fromDate = startDate.strftime("%Y%m%d") + ' 21:00:00'
        toDate =  endDate.strftime("%Y%m%d") + ' 21:00:00'

        print("fromDate: ",fromDate)
        print("toDate: ",toDate)

        df = pd.read_csv(filePath[idxFile])
        print(df.head())
        df.rename(columns={'date':'D'},inplace=True)
        df['D'] = pd.to_datetime(df.D)
        #start = fromDate
        #end = toDate
        #print(start, '::::', end)

        df=df.set_index('D')
        df.sort_index(inplace=True)
        start = pd.to_datetime(fromDate)
        end = pd.to_datetime(toDate)
        print(start, '::::', end)
        #print(fromDate, '::::', toDate)

        file = os.path.join(dataDir, legs[idxFile] + ".csv")

        if asset in StrategyBMap:
            startFourDAgo = start - BDay(2)
            startStr = startFourDAgo.strftime("%Y%m%d") + ' 21:00:00'
            start = pd.to_datetime(startStr)
            dfNew = df[(df.index >= start) & (df.index <= end)]
            #print(dfNew.head())

            print("Snipped file: ", file)
            dfNew.to_csv(file, index=True)

        else:
            if start in df.index:
                idx = df.index.get_loc(start)
            else:
                idx = np.argmax(df.index > start)
            dfSlice = df.iloc[idx -100:idx]
            # dfNew = df[(df['date'] > '2017-9-2') & (df['date'] <= '2017-9-10')]
            #dfNew = df[(df['D'] > start) & (df['D'] <= end)]
            dfNew = df[(df.index >= start) & (df.index <= end)]
            #print(dfNew.head())

            dfConcatenated = pd.concat([dfSlice,dfNew])
            dfConcatenated.sort_index(inplace=True)
            #print(dfConcatenated.head())

            print("Snipped file: ", file)
            dfConcatenated.to_csv(file, index=True)

def CreateDailyDataFile(asset, frequency, srcDataDir,paramDir, dataDir,sDate, eDate):
    filePath = []
    legs = []

    if asset in StrategyBMap:
        legsArr = StrategyBMap[asset]
        legs.append(legsArr[0].split('_')[1][:-2])
        legs.append(legsArr[1].split('_')[1][:-2])
        filePath.append(os.path.join(srcDataDir, "Futures", "Live", legs[0] + legs[1], legs[0] + ".csv"))
        filePath.append(os.path.join(srcDataDir, "Futures", "Live", legs[0] + legs[1], legs[1] + ".csv"))

    else:
        if asset in currencyList:
            filePath.append(os.path.join(srcDataDir, "FX", "Live", asset, frequency + "_resample.csv"))
        else:
            filePath.append(os.path.join(srcDataDir, "Futures", "Live", asset, frequency + "_resample.csv"))
        legs.append(asset)
    for idxFile, val in enumerate(filePath):
        file = os.path.join(dataDir, legs[idxFile] + ".csv")
        print("Copying ",filePath[idxFile] , " to ", file)
        shutil.copyfile(filePath[idxFile], file)



def AddNodesToConfig(asset,paramDir,dataDir, frequency):
    legs = []
    if asset in StrategyBMap:
        legsArr = StrategyBMap[asset]
        legs.append(legsArr[0].split('_')[1][:-2])
        legs.append(legsArr[1].split('_')[1][:-2])
    else:
        legs.append(asset)

    treeCfg = et.parse(os.path.join(paramDir,"config.xml"))
    rootCfg = treeCfg.getroot()
    for elem in rootCfg.iter('Instrument1'):
        #Add frequency node if missing
        '''node = elem.find('Frequency')
        print(node)
        if node == None:
            print("Adding Frequency")
            FrequencyEl = et.Element("Frequency")
            FrequencyEl.text = frequency
            elem.append(FrequencyEl)
        else:
            print("Frequency exists for Instrument1")'''

        # update File node
        node = elem.find('File')
        if asset in StrategyBMap:
            prodNode = elem.find('Product')
            if prodNode.text == legsArr[0].split('_')[1]:
                node.text = os.path.join(dataDir, legs[0]+ ".csv")
            else:
                node.text = os.path.join(dataDir, legs[1]+ ".csv")

        else:
            node.text = os.path.join(dataDir, legs[0]+ ".csv")
    for elem in rootCfg.iter('Instrument2'):
        # Add frequency node if missing
        '''node = elem.find('Frequency')
        print(node)
        if node == None:
            # elem.append('Frequency')
            FrequencyEl = et.Element("Frequency")
            FrequencyEl.text = frequency
            elem.append(FrequencyEl)
        else:
            print("Frequency exists for Instrument2")'''

        # update File node
        node = elem.find('File')
        prodNode = elem.find('Product')
        if prodNode.text == legsArr[1].split('_')[1]:
            node.text = os.path.join(dataDir, legs[1]+ ".csv")
        else:
            node.text = os.path.join(dataDir, legs[0] + ".csv")

    shutil.copyfile(os.path.join(paramDir, 'config.xml'), os.path.join(paramDir, 'configOld.xml'))

    treeCfg.write(os.path.join(paramDir, 'config.xml'))

def GetParamsFileKey(asset):
    #Hayek,1m,params-0 key=CME:FVU7,CME:TYU7
    #GBPUSD,15m,params-0 key=FXOTC:GBP/USD
    #ES,4H,params-3 key=CME:ESU7
    if asset in StrategyBMap:
        #key = StrategyBMap[asset][0].replace('_',':') + ',' + StrategyBMap[asset][1].replace('_',':')
        key = StrategyBMap[asset][0] + ',' + StrategyBMap[asset][1]
    elif asset in currencyList:
        #key = "FXOTC:" + asset[:3] + '/' + asset[3:]
        #key = "FXOTC_" + asset[:3] + '/' + asset[3:]
        key = "FXOTC_" + asset[:3] + '_' + asset[3:]
    else:
        #key = "CME:" + asset + "U7"
        key = "CME_" + asset + "U7"

    return key
def EditParamsFile(asset,paramDir, frequency):
    paramKey = GetParamsFileKey(asset)
    print("Param key to match: ",paramKey)
    #file = os.path.join("C:/Temp","params.xml")
    file = os.path.join(paramDir,"params.xml")
    shutil.copyfile(os.path.join(paramDir, 'params.xml'), os.path.join(paramDir, 'paramsOld.xml'))
    tree = et.parse(file)

    root = tree.getroot()
    #print(root.tag)
    #print(root[0][0][0].text)
    #for child in root:
        #print(child.tag)

    for elem in root.findall("./npVector/m_npVector"):
        node = elem.find("count")
        node.text = str(1)
    for node in root.findall("./npVector/m_npVector"):
        for item in node.findall("item"):
            Key = item.find('Key').text
            #print("Key: ", paramKey)
            if Key != paramKey:
                node.remove(item)
            else:
                print("Keeping Key: ", paramKey)
    #root[0][0][0].text = str(1)
    '''for item in root.iter('item'):
        Key = item.find('Key').text
        print("Key: ", Key)
        if Key != "CME:ESU7":
            root.remove(item)'''
    for elem in root.iter('item'):
        node = elem.find('Frequency')
        print(node)
        if node == None:
            print("Adding Frequency")
            FrequencyEl = et.Element("Frequency")
            FrequencyEl.text = frequency
            elem.insert(2,FrequencyEl)
        else:
            print("Frequency exists for Instrument1")
   # tree.write(os.path.join("C:/Temp","paramsNew.xml"),xml_declaration=True,encoding="UTF-8")
    #tree.write(os.path.join("C:/Temp","paramsNew.xml"))
    tree.write(os.path.join(paramDir, "params.xml"))
    #f = open(os.path.join("C:/Temp","paramsNew.xml"), "r")
    f = open(os.path.join(paramDir,"params.xml"), "r")
    contents = f.readlines()
    f.close()

    contents.insert(0, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>")

    contents.insert(1, "<!DOCTYPE boost_serialization>")

    f = open(os.path.join(paramDir,"params.xml"), "w")
    contents = "".join(contents)
    f.write(contents)
    f.close()


def main():
    parser = argparse.ArgumentParser()

    # Note: paramDir is input dir and also the output dir
    parser.add_argument('-paramsDir', '--paramsDir', default='/home/lanarayan/MLData/', help="Base Directory for alphas params.xml and config.xml")
    parser.add_argument('-ex', '--exeDir', default='/home/lanarayan/MLData/UATDev/Vishnu/build', help="C=++ TestSimulator Exe Dir")
    parser.add_argument('-alpha', '--alpha', default='/home/lanarayan/MLData/Backtests/alphasAB.txt', help="alphas file location")
    parser.add_argument('-s', '--startDate', default='20180101', help="start date")
    parser.add_argument('-e', '--endDate', default='20190101', help="end date")
    parser.add_argument('-baseDataDir', '--baseDataDir', default='/home/lanarayan/MLData/', help="base Directory containing Futures and FX data")
    parser.add_argument('-baseOutDir', '--baseOutDir', default='', help="output Directory if specified will be used for output")

    parser.add_argument('-ignore', '--ignoreTOD', action='store_true',
                        help="ignore time of day, for example, for daily candles")
    parser.add_argument('-test', '--test', action='store_true',
                        help="enable verbose message by passing -g flag to TestSimulator")
    parser.add_argument('-daily', '--daily', action='store_true',
                        help="if flag provided create daily data file")
    parser.add_argument('-norun', '--norun', action='store_true',
                        help="if flag provided do not run test simulator")

    parser.add_argument("-l", "--log", dest="logLevel", default="DEBUG",
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        help="Set the logging level")

    #parser.add_argument('-log', '--logPath', default='C:/MyProjects/PyCharmProjects/HelloWorldProj',
    #                 help="log file  path")
    parser.add_argument('-log', '--logPath', default='/home/lanarayan/MLData/Backtests/Logs/',
                        help="log file  path")

    args = parser.parse_args()
    print(args)
    dateForLog = datetime.now().strftime("%Y%m%d-%H%M%S.%f")

    if args.daily:
        logging.basicConfig(filename=os.path.join(args.logPath,'RunAlphaListDaily-' + dateForLog + '.log'), filemode='w', level=getattr(logging, args.logLevel),
                        format='%(asctime)s %(levelname)s %(message)s')
    else:
        logging.basicConfig(filename=os.path.join(args.logPath, 'RunAlphaList-' + dateForLog + '.log'),
                            filemode='w', level=getattr(logging, args.logLevel),
                            format='%(asctime)s %(levelname)s %(message)s')

    columnNames = ['AssetName', 'Frequency', 'Param']
    #print("CWD :", os.getcwd())
    #logging.debug("CWD: {} ".format(os.getcwd()))
    if args.baseOutDir != '':
        #base folder where folders will be created for alphas
        if not os.path.exists(args.baseOutDir):
            print("Creating output folder :" + args.baseOutDir)
            os.makedirs(args.baseOutDir)

    alphaFile = os.path.abspath(args.alpha)
    dfAlphas = pd.read_csv(alphaFile, names=columnNames, index_col=False)


    cmds=[]
    # Traverse alpha list dataframe
    for index, row in dfAlphas.iterrows():
        #paramDir should already exist (created by RunSimulatorSetup)
        paramDir = os.path.join(args.paramsDir, row['AssetName'], row['Frequency'], row['Param'])
        # dir where data file ( config <File> val)  will be located for each alpha
        dataDir = os.path.join(args.paramsDir, row['AssetName'], row['Frequency'])

        if args.daily:
            print("Creating daily data file")
            logging.debug("Creating daily data file")
            #Note args.baseDataDir is MLDATA/Sim
            CreateDailyDataFile(row['AssetName'], row['Frequency'],args.baseDataDir,paramDir,dataDir, args.startDate, args.endDate)
        else:
            #create datafile for config file in the paramsDir
            CreateDataFile(row['AssetName'], row['Frequency'],args.baseDataDir,paramDir,dataDir, args.startDate, args.endDate)

        # Params file: Remove all <item> except the one that corresponds to row['AssetName'] ; Add frequency node if doesnt exist.
        EditParamsFile(row['AssetName'], paramDir, row['Frequency'])

        # Add Frequency nodes to config.xml; update <File> node to point to  file created in CreateDataFile function
        AddNodesToConfig(row['AssetName'], paramDir, dataDir, row['Frequency'])

        if args.baseOutDir != '':
            baseOutDir = os.path.join(args.baseOutDir, row['AssetName'], row['Frequency'], row['Param'])
        else:
            baseOutDir = paramDir
        #cmd= ' -s ' + args.startDate + ' -e ' + args.endDate + ' -r ' + paramDir + ' -o ' + paramDir
        cmd= ' -s ' + args.startDate + ' -e ' + args.endDate + ' -r ' + paramDir + ' -o ' + baseOutDir
        if args.ignoreTOD:
            cmd += ' -i'

        if args.test:
            cmd += ' -g'
        print("Adding cmd :", cmd)
        logging.debug("Adding {}".format(cmd))
        cmds.append(cmd)

    if not args.norun:
        global simulatorExeDir
        simulatorExeDir = args.exeDir
        num = multiprocessing.cpu_count()
        p = multiprocessing.Pool(num - 1)
        #p.map(invokeSimulator,cmds)
        prod_x = partial(invokeSimulator, exeDir=simulatorExeDir)
        p.map(prod_x, cmds)
    else:
        #just print entire command
        print("Not executing "  + os.path.join(args.exeDir,"TestSimulator") + cmd)

def invokeSimulator(cmd , exeDir):
    exePath = os.path.join(exeDir,"TestSimulator")
    print("\nExecuting " + exePath + cmd)
    logging.debug("Executing {}".format(exePath + cmd))

    os.system(exePath + cmd)

if __name__ == '__main__':
    main()