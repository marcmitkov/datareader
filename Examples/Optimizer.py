#!/usr/bin/python3.6
import pandas as pd
import argparse
import datetime as dt
import os
import xml.etree.ElementTree as ET


def GetkeyValDict(pvalues):
    #create a dict of key val pairs
    dictParams={}
    for x in pvalues.split(','):
        if x == '':
            print('problem')
            continue
        print(x)
        kv=x.split(':')
        key = kv[0]
        val = kv[1]
        print('z0 {} ,z1 {}'.format(kv[0],kv[1]))
        dictParams[key] = val
    #dictParams=dict(x.split(':') for x in pvalues.split(','))

    return dictParams

def GetSolverParams(pvalues):
    # remove  the frequency
    pvalNoFreq = pvalues[3:]
    # create a dict of key val pairs
    dictParams = dict(x.split(':') for x in pvalNoFreq.split(','))
    '''stop = dictParams['stop'] if 'stop' in dictParams.keys() else ""
    target = dictParams['target'] if 'target' in dictParams.keys() else ""
    candleQuantum = dictParams['candleQuantum'] if 'candleQuantum' in dictParams.keys() else ""
    return stop, target, candleQuantum'''
    return dictParams

def GetKeyValue(dict,key):
    value = dict[key] if key in dict.keys() else ""
    return value

def ModifyConfig(xmlConfig,dfParamsFit):
    tree = ET.parse(xmlConfig)

    root = tree.getroot()
    # dfConfig = pd.DataFrame(columns=['alpha', 'target', 'stop','signals','candleQuantum','maxExposure','pnl'])
    dfConfig = pd.DataFrame()

    # Read Property elements attributes name and value to create dfConfig
    for tag in root.findall('Property'):
        name = tag.get('name')

        if name is not None and name.endswith('_Params'):
            print(name)
            value = tag.get('value')
            if value is not None:
                #print(value)
                dftmp = dfParamsFit.loc[dfParamsFit['Alpha'].str.contains(name)]
                if len(dftmp) == 0:
                    continue
                if len(dftmp) > 1:
                    print('Error: dfParamsFit has more than one match for {}'.format(name))
                tag.attrib['value']= dftmp['Params'].iloc[0]
                print(tag.get('value'))

    print(ET.tostring(root))
    return tree

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-baseConfig", "--baseConfig", default='/home/mrai/Results',
                        help="Base dir")
    parser.add_argument("-baseSolver", "--baseSolver", default='/home/mrai/Runs/a42/OutSolver/Weekly/',
                        help="Base dir")
    parser.add_argument('-baseOut', '--baseOut', default='/home/mrai/ResultsOut', help="base Out")
    parser.add_argument('-f', '--startDate', default='20201123', help="from date")
    parser.add_argument('-t', '--endDate', default='20201123', help="to date")

    parser.add_argument('-v', '--verbose', default='False', help="verbose")

    parser.add_argument("-l", "--logLevel", dest="logLevel", default="DEBUG",
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        help="Set the logging level")
    grp=parser.add_mutually_exclusive_group()

    grp.add_argument('-alphaFile', '--alphaFile', default='/home/mrai/input/Alphas.txt',
                        help="location of alpha file")

    grp.add_argument('-alphas', '--alphas', default=[], help="alphas list e.g ['ES', 'EURUSD']", nargs='*')
    # -params and -frequency will be used only when -alphas provided else -alphaFile will contain this information
    parser.add_argument('-params', '--params', default='', help="0:2 for params-0,params-1, params-2")
    parser.add_argument('-frequency', '--frequency', default='1m', help="1m,5m")
    parser.add_argument('-runtype', '--runtype', default='FT', help="FT,FX")
    parser.add_argument('-rankFile', '--rankFile', default='ConsolidatedMaxExpTop.csv', help="ConsolidatedMaxExpTop.csv or ConsolidatedTopFour.csv")
    parser.add_argument('-showParams', '--showParams', action='store_true', help="True - include params columne")

    parser.add_argument('-mode', '--mode', default='q',
                        help="q - not display  or v - display each alpha HTML report on browser")
    parser.add_argument('-log', '--log', default='/home/mrai/Logs', help="log file  path")

    startTime = dt.datetime.today()

    args = parser.parse_args()
    print(args)
    print(os.getcwd())
    #exit(0)


    args.baseOut = os.path.abspath(args.baseOut)
    if not os.path.exists(args.baseOut):
        print("Creating output folder :" + args.baseOut)
        os.makedirs(args.baseOut)

    print("cd to :",args.baseOut)
    os.chdir(args.baseOut)
    print("pwd:",os.getcwd())
    ColumnsList =['Alpha', 'Combo', 'TotalPnl', 'Stop', 'Target', 'MaxExposure', 'Quantum', 'Params']

    xml_file=''
    if args.runtype == 'FT':
        xml_file = os.path.join(args.baseConfig, "chanel_02_aur.xml")
        dirList= ['PositionsFT_1m','PositionsFT_1m_Q','PositionsFT_5m','PositionsFT_5m_Q']
    else:
        xml_file = os.path.join(args.baseConfig, "chanel_03.xml")
        dirList = ['PositionsFX_1m', 'PositionsFX_1m_Q', 'PositionsFX_5m', 'PositionsFX_5m_Q']

    tree=ET.parse(xml_file)

    root=tree.getroot()
    #dfConfig = pd.DataFrame(columns=['alpha', 'target', 'stop','signals','candleQuantum','maxExposure','pnl'])
    dfConfig = pd.DataFrame()

    #Read Property elements attributes name and value to create dfConfig
    for tag in root.findall('Property'):
        name = tag.get('name')

        if name is not None and name.endswith('_Params'):
            print(name)
            value=tag.get('value')
            if value is not None:
                print(value)
                #create dictionary for all params i.e value attribute
                dictConfig = GetkeyValDict(value)
                stop=GetKeyValue(dictConfig,'stop')
                target = GetKeyValue(dictConfig,'target')
                candleQuantum = GetKeyValue(dictConfig,'candleQuantum')
                signals = GetKeyValue(dictConfig,'signals')
                maxExposure = GetKeyValue(dictConfig,'maxExposure')

                # ['Alpha','Combo','TotalPnl','Ratio1','Ratio2','Stop','Target','MaxExposure','Quantum']

                dfConfig = dfConfig.append({'Alpha': name,  'Combo':signals, 'TotalPnl':'','Target': target, 'Stop': stop,'MaxExposure':maxExposure,'Quantum':candleQuantum,'Params':''},
                                           ignore_index=True)
                if args.showParams:
                    dfConfig=dfConfig[ColumnsList]
                else:
                    dfConfig=dfConfig[ColumnsList[:-1]]



    print(dfConfig)
    dfConfig.to_csv(os.path.join(args.baseOut,  "Config" +args.runtype + ".csv"),index=False)

    #Map with key freqQuant combination (e.g 1m ,1m_Q) and value= dataframe for FX(orFT)_freq_quant combo from Solver output
    dfMap={}
    for dir in dirList:
        if not os.path.isfile(os.path.join(args.baseSolver,dir,args.rankFile)):
            print('Alert - Missing {}'.format(os.path.join(args.baseSolver,dir,args.rankFile)))
            continue;

        dfSolver = pd.read_csv(os.path.join(args.baseSolver,dir,args.rankFile), sep=';', index_col=False)
        dfSolver= dfSolver[ColumnsList]
        #replace comma with space from LH,HL etc to match config combo LH HL
        dfSolver['Combo']= dfSolver['Combo'].str.replace(',',' ')
        key=dir.split('_',maxsplit=1)[1]
        dfMap[key]=dfSolver

    #
    dfOptimizer= pd.DataFrame()
    dfParamsFit= pd.DataFrame()
    for index, row in dfConfig.iterrows():
        if '_5_' in row['Alpha']:
            continue
        dfOptimizer=dfOptimizer.append(row)
        alpha = row['Alpha']
        isQuant=row['Quantum']
        #GCM1_1_Params , EUR/GBP_1_Params in config
        alphaComponents = alpha.split('_')
        if args.runtype == 'FT':
            instr = alphaComponents[0][0:2]
        else:
            instr = alphaComponents[0].replace('/','')
        freq = alphaComponents[1]
        #generate key (1m , 1m_Q,5m ,5m_Q) to get value from dfMap
        dictKey = freq + 'm'
        if isQuant:
            dictKey= dictKey + '_Q'

        if dictKey not in dfMap.keys():
            print('Alert - Missing key {} in dfMap'.format(dictKey))
            continue

        df= dfMap[dictKey]
        #Get row for alpha
        dftmp = df.loc[df['Alpha'].str.contains(instr)]
        dftmp = dftmp[ColumnsList]
        dfOptimizer=dfOptimizer.append(dftmp)

        if len(dftmp) >0:
            #Get first row from dftmp for dfParamsFit
            #1m-tradeable:1,ticksize:0.1,contracts:1,cushion:0.01,riskfactor:3,test:0,testBS:0,toowide:3,skipmissedsignal:0,intv:150,minlen:100,rho:6,sessionStart:4,sessionEnd:20,maxExposure:5,aggressorin:1,aggressorout:1,usemid:0,target:40,stop:40,continuous:1,scale:3,intvMultiplier:15
            dfrowOne= dftmp.iloc[0]
            ##remove  the frequency 1m- from dfrowOne.Params
            paramsNew= dfrowOne.Params[3:] + ",signals:" + dfrowOne.Combo
            dfParamsFit = dfParamsFit.append({'Alpha': alpha, 'Params':paramsNew},
                                               ignore_index=True)
        print()

    print(dfOptimizer)

    if args.showParams:
        dfOptimizer = dfOptimizer[ColumnsList]
    else:
        dfOptimizer = dfOptimizer[ColumnsList[:-1]]

    #dfNew = dfNew[ColumnsList]
    dfOptimizer.to_csv(os.path.join(args.baseOut, "ConfigNew" + args.runtype + ".csv"), index=False)


    #Use dfParamsFit to change config
    xmlTreeNew = ModifyConfig(xml_file,dfParamsFit)
    if args.runtype == 'FT':
        xmlTreeNew.write(os.path.join(args.baseOut, "chanel_02_aurNew.xml"),xml_declaration=True,encoding='utf-8')
    else:
        xmlTreeNew.write(os.path.join(args.baseOut, "chanel_03New.xml"),xml_declaration=True,encoding='utf-8')


    print("ConfigNew.csv output in : {} ".format(args.baseOut))
    print("New xml Config output in : {} ".format(args.baseOut))

    print("Done")
if __name__ == '__main__':
    main()