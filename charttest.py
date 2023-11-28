
import pandas as pd
import os
import glob

#chart related
from plotly.subplots import make_subplots
import plotly.graph_objects as go
import matplotlib.colors as mcolors
import plotly.express as px
import argparse
from glob import glob
from datetime import datetime

colnames =[ "Id", "TimeStamp", "Size", "AvgPx", "ClosePx", "CloseTimeStamp", "PnlRealized", "PnlUnRealized", "State","Remaining",  "FillPx", "FillTimeStamp", "ExitCode", "CloseRemaining", "CloseFillPx", "CloseFillTimeStamp","Strat","EntryType", "Value","Description","StopSize", "TargetSize","ExpiryDate"]

CandleColNames= ["D","T","SYBL","LSTP","LSTS","INTF","INTH","INTL","INTV","NTRD","LSTB","LSBS","LSTA","LSAS","HBID","LBID","HASK","LASK","HBSZ","LBSZ","HASZ","LASZ","HLTS","IVAM","amount_G","count_G","weightedPrice_G","amount_P","count_P","weightedPrice_P","blockCount","specialCount"
]

def ReadPositions(dir, pos=False):
    print("Reading ",  "Positions" if pos else "Open Positions", dir)
    posfile = os.path.join(dir, "positions.txt" if pos else "openpositions.txt")
    retval = pd.read_csv(posfile, names=colnames, index_col=False, parse_dates=[1])
    #print(retval.columns)
    #print(retval.dtypes)
    return retval

def ReadCandles(dn):
    print("calling readcandles", dn)
    df = pd.DataFrame()
    cfs = glob(dn + "/candles*")
    print('candle files', cfs)
    if len(cfs) == 0:
        return None, None
    fileCandles = cfs[0]
    #fileCandles=os.path.join(dn, "candles.txt")
    dfCandles = pd.read_csv(fileCandles,names=CandleColNames, index_col=False)
    dfCandles.D = pd.to_datetime(dfCandles.D)
    file = os.path.join(dn, "all4.txt")

    count = 0

    print("Reading file", file)
    for line in (open(file).readlines()):

        if count % 5 == 0:
            activedate = pd.to_datetime(line.strip())
            count = count + 1
            continue
        row = [activedate]
        if( line.strip()[-1] == ','):
            line=line.strip()[:-1]

        row.extend(line.strip().split(","))
        # print(row)
        df2 = pd.DataFrame([row])
        # print(df2)
        df = pd.concat([df, df2])
        count = count + 1

    df = df.reset_index(drop=True)
    print("spline data line count",count)
    # df.drop(df.columns[102], axis = 1, inplace=True)
    #df = df.drop(df.columns[[101]], axis=1)
    #print(df.head())
    df = df.set_index(0)
    print(df.shape)
    df = df.astype(float)
    #print("all4")
    #print(df.head())
    #print("candles")
    #print(dfCandles.head())
    return df, dfCandles


def GenerateGraphs(abspath, relpath, availabledates, loaddate, loadpositions, output=None):
    dn = os.path.join(abspath, relpath)
    retval1=""
    #dn = 'Z:/view/output/finalJuly23-Sep23-BXZ-PQ-trailing-Corrected-FX/EURUSD/1m/params-0/20230703'
    #dn = "D:/MyProjects/Spline/data/ES/5m/params-0/20230720"

    try:
        index = availabledates.index(loaddate)
        print(index)
    except ValueError:
        print(f"{loaddate} is not found in the list.")

    lookback = 2
    all4 = pd.DataFrame()
    candles = pd.DataFrame()
    for d in (availabledates[index - lookback:index] if index - lookback > 0 else [availabledates[index]]):
        fn = os.path.join(os.path.basename(dn), d.strftime("%Y%m%d"))
        all4temp, candlestemp = ReadCandles(dn)
        if candlestemp is None:
            print("no candles found at", fn)
            return
        all4 = pd.concat([all4, all4temp], ignore_index=False)
        candles = pd.concat([candles, candlestemp], ignore_index=False)
    pos = ReadPositions(dn, loadpositions)
    #print(pos.columns)
    #print(pos.dtypes)

    if len(all4) == 0:
        print(" no spline found for ", dn)

    if len(candles) == 0:
        print(" no candles found for ", dn)

    all4 = all4.drop_duplicates()
    candles = candles.drop_duplicates()
    #print(pos.TimeStamp)
    #print(all4.head())
    #print(candles.head())


    for ts in pos.TimeStamp:
        if output is None:
            dirname = os.path.join(abspath, "graphs")
            filename = relpath.replace('\\', '-') + ts.strftime("-%H-%M-%S") + ".html"
            filenameFull = os.path.join(dirname, filename)
        else:
            dirname = os.path.join(output, "graphs")
            filename = relpath.replace('\\', '-') + ts.strftime("-%H-%M-%S") + ".html"
            filenameFull = os.path.join(dirname, filename)

        print("creating", dirname, "if it doesn't exist")
        os.makedirs(dirname, exist_ok=True)
        print(filename)
    #        ts = pos.TimeStamp[-1:].values[0]

        candles = candles.set_index(candles.D)
        #print(type(candles.index))
        #print(candles.dtypes)
        #print(all4.dtypes)
        filteredCandles=candles[candles.index <= ts]
        filteredAll4=all4[all4.index <= ts]

        minlen = filteredAll4.shape[1]
        if minlen == 0:
            print("no spline data")
            continue
        else:
            print("will need lookback of ", minlen)
        # get the last 3 curves
        last = 3
        uniqueTS = filteredAll4.index.unique()
        if len(uniqueTS) < minlen:
            print("not enough data", len(uniqueTS))
            continue

        retval = []
        for i in range(last):
            print("iteration",i, "for position", filename)
            firstcurve = filteredAll4.iloc[-4:, :]
            firstcurve.columns = uniqueTS[-100:]
            #print(filteredAll4.shape)
            filteredAll4.drop(filteredAll4.index[-4:], inplace=True)
            #print(filteredAll4.shape)
            uniqueTS =  uniqueTS[:-1]
            firstcurve =  firstcurve.transpose()
            firstcurve.columns = ["points", "spline", "deriv", "deriv2"]
            firstcurve = pd.merge(firstcurve, filteredCandles,  left_index=True, right_index=True,how='inner')
            #print(firstcurve[["points", "spline", "deriv", "deriv2","INTV"]])
            retval.append(firstcurve)

        #chart related
        names = sorted(mcolors.CSS4_COLORS, key=lambda c: tuple(mcolors.rgb_to_hsv(mcolors.to_rgb(c))))
        splinecolor = names.index('gold')
        derivcolor = names.index('blue')
        fig = make_subplots(shared_xaxes=True,rows=2, cols=1,specs=[[{"secondary_y": True}],[{"secondary_y": False}]])
        for i in range(last):
            c = retval[i]
            data0 = c["points"]
            data1 = c["spline"]
            data2 = c["deriv"]
            data3 = c["INTV"]
            #print(data0.to_numpy())

            fig.add_trace(go.Scatter(x=c.index, y=data0, name=c.index[-1].strftime("%m/%d/%Y %H:%M:%S"),mode='markers'), secondary_y=False,row=1, col=1)

            fig.add_trace(
                go.Scatter(x=c.index, y=data1, name="Spline data" + str(i),marker_color=mcolors.CSS4_COLORS[names[splinecolor+i]]),
                secondary_y=False, row=1, col=1
            )

            fig.add_trace(
                go.Scatter(x=c.index, y=data2, name="Derivative data" + str(i),marker_color=mcolors.CSS4_COLORS[names[derivcolor+i]]),
                secondary_y=True, row=1, col=1
            )

            fig.add_trace(
                go.Bar(x=c.index, y=data3, marker_color="darkgreen",showlegend=False), row=2, col=1
            )
            buckets = [0] * len(c.index)
            fig.add_trace(
                go.Scatter(x=c.index, y=buckets,
                           name="0 AXIS", marker_color="red"), secondary_y=True,row=1, col=1
            )

            fig.update_layout(height=1500)
            # Set x-axis title
            fig.update_xaxes(title_text="TimeStamp")
            fig.update_layout(xaxis_tickangle=-45)

            # Set y-axes titles
            fig.update_yaxes(
                title_text="<b>Spline</b>",
                secondary_y=False)
            fig.update_yaxes(
                title_text="<b>Derivative</b>",
                secondary_y=True)
            # Update yaxis properties
            fig.update_yaxes(title_text="Volume", row=2, col=1)
        print("writing figure", filename)
        fig.write_html(filenameFull)
        filename = os.path.normpath(filenameFull)
        retval1 += f'<tr><td>{filename}</td><td><a href="{filenameFull}">graph</a></td></tr>'

    return retval1

def Test():

    # Create the argument parser
    parser = argparse.ArgumentParser(description='A simple program with command-line arguments.')

    # Add command-line arguments
    parser.add_argument('--base', help='base directory',  default='D:/MyProjects/Spline/data')
    parser.add_argument('--output', help='output directory',  default='D:/MyProjects/Spline/data')
    parser.add_argument('--assets', help='asset',  nargs='+')
    parser.add_argument('--freq', help='frequencies',  nargs='+')
    parser.add_argument('--params', help='params',  nargs='+')
    parser.add_argument('--dates', help='dates',  nargs='+', default=[])

    parser.add_argument('--loadpositions', help='choose positions or openpositions', action='store_true')

    # Parse the command-line arguments
    args = parser.parse_args()
    print(args)
    dynamic_html_table = ""
    #dn = os.path.join(args.base, args.assets[0], args.freq[0], args.params[0], args.dates[0])
    #print(dn)

    assets = glob(args.base + '/*')
    if len(args.assets) != 0:
        for a in args.assets:
            assets = []
            assets.append(os.path.join(args.base, a))
    for p in assets:
        path = os.path.normpath(p)
        print("asset", path)
        freqs = glob(path + '/*')
        if len(args.freq) != 0:
            for f in args.freq:
                freqs = []
                freqs.append(os.path.join(path, f))
        for f in freqs:
            path = os.path.normpath(f)
            print("freq", path)
            params = glob(path + '/*')
            if len(args.params) != 0:
                for r in args.params:
                    params = []
                    params.append(os.path.join(path, r))
            for p in params:
                path = os.path.normpath(p)
                print("param", path)
                dates = glob(path + '/*')
                if len(args.dates) != 0:
                    for e in args.dates:
                        dates = []
                        dates.append(os.path.join(path, e))
                date_objects = [datetime.strptime(os.path.basename(date_str), "%Y%m%d") for date_str in dates]

                # Sort the list of datetime objects
                sorted_dates = sorted(date_objects)

                # Print the sorted dates
                for date in sorted_dates:
                    print(date.strftime("%Y-%m-%d"))
                for d in dates:
                    path = os.path.normpath(d)
                    print("dates", path)
                    relpath = os.path.relpath(path, args.base)
                    dynamic_html_table = GenerateGraphs(args.base, relpath, sorted_dates, datetime.strptime(os.path.basename(d), "%Y%m%d"), args.loadpositions, args.output)

    # Main page for dynamic (Interactive)
    dynamic_html_table = f'<table border="1px solid black">{dynamic_html_table}</table>'
    with open(args.base + "/" + "dynamic.html", "w") as f:
        f.write(dynamic_html_table)

    exit(0)


# Test 1 for a specific date
# --dates 20230720 --assets ES --base D:/MyProjects/Spline/data --freq 5m  --params params-0 --output D:/MyProjects/Spline/


if __name__ == '__main__':
    Test()
