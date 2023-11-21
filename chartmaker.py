#https://plotly.com/python/multiple-axes/#multiple-axes-in-dash
#VERY GOOD Plotly : https://www.youtube.com/watch?v=GGL6U0k8WYA
#Dash Plotly: https://www.youtube.com/watch?v=hSPmj7mK6ng&list=PLh3I780jNsiTXlWYiNWjq2rBgg3UsL1Ub&index=13
#plotly subplots : https://plotly.com/python/subplots/#:~:text=Plotly%20Express%20does%20not%20support,charts%20to%20display%20distribution%20information.
#Refer HCT/SplinePlotNew.py

from dash import Dash, dcc, html, Input, Output
from plotly.subplots import make_subplots
import plotly.graph_objects as go
# TODO : FOR better layout
import dash_bootstrap_components as dbc
import re
import pandas as pd
import os
import glob
import numpy as np
import matplotlib.colors as mcolors
import plotly.express as px


colnames =[ "Id", "TimeStamp", "Size", "AvgPx", "ClosePx", "CloseTimeStamp", "PnlRealized", "PnlUnRealized", "State","Remaining",  "FillPx", "FillTimeStamp", "ExitCode", "CloseRemaining", "CloseFillPx", "CloseFillTimeStamp","Strat","EntryType", "Value","Description","StopSize", "TargetSize","ExpiryDate"]


CandleColNames= ["D","T","SYBL","LSTP","LSTS","INTF","INTH","INTL","INTV","NTRD","LSTB","LSBS","LSTA","LSAS","HBID","LBID","HASK","LASK","HBSZ","LBSZ","HASZ","LASZ","HLTS","IVAM","amount_G","count_G","weightedPrice_G","amount_P","count_P","weightedPrice_P","blockCount","specialCount"
]
dfCandles = pd.DataFrame()

app = Dash(__name__)

"""
Returns data for graphing after applying position timestamp(ts), last and offset to dataframe(data
data: spline and derivative data
offset:eliminate dataframe rows from bottom based on offset
last: get rows from bottom based on last 
"""
def SplineMultiEx(data, last, offset, ts=None):
    print(ts)
    dataGlobal = data[data.index <= pd.to_datetime(ts)]
    if (len(dataGlobal) == 0):
        print("ERROR: NO Spline data for positions date {} . ".format(ts))
        return pd.DataFrame()
        #exit(-1)
    #for char in ['/', ':']:
        #ts = ts.replace(char, "_")
    #dataGlobal.to_csv("C:/BH/Maya/output/dataglobal"+ ts.replace(":", "_") + ".csv",header=False)
    retTS=None
    retLen=None
    if (offset  > 0 ):
        dataGlobal = dataGlobal.drop(dataGlobal.tail(offset*4).index) #remove offset*4 rows starting from bottom
    if (len(dataGlobal) == 0):
        print("ERROR: dataframe empty with offset value {} . EXITTING PROGRAM".format(offset))
        return pd.DataFrame()
    len1=dataGlobal.shape[1]
    print("columns in data", len1)
    print("index: ", dataGlobal.index)
    print("length of unique index ", len(dataGlobal.index.unique()))
    if len(dataGlobal.index.unique()) > len1:
        print(dataGlobal.index.unique()[-len1])
        retTS = dataGlobal.index.unique()[-len1]
        print(type(retTS))
        retLen = len1
    dataGlobal = dataGlobal[-4*last:] # get 4*last rows starting from bottom

    dataGlobal = dataGlobal.transpose()

    dataGlobal = dataGlobal.dropna(axis=0)
    return dataGlobal,retTS, retLen


def ReadPositions(dir):
    print("Reading Positions",dir)
    posfile = os.path.join(dir, "openpositions.txt")
    retval = pd.read_csv(posfile, names=colnames, index_col=False, parse_dates=[1])
    #print(retval.columns)
    #print(retval.dtypes)
    return retval


#def Test():
# BASE_FOLDER = '/lnarayan/dev/output/finalJuly23-Sep23-BXZ-PQ-trailing-Corrected-FX/EURUSD/1m' # REPLACE THIS VALUE
BASE_FOLDER = '/lnarayan/dev/output/'
BASE_FOLDER = 'D:/MyProjects/Spline/'
#/ES/5m/params-0
ALL_FILES = glob.glob(f"{BASE_FOLDER}/*")
print(ALL_FILES)
positions = None
#dn = '/lnarayan/dev/output/runJune23lagFixed/AUL/1m/params-0/20230601'
#dn="C:/MyP/daily/ES/1m/params-0/20230908"
#dn="C:/MyP/daily/CL/5m/params-0/20230920"
#dn="C:/MyProjects/data/daily/US/1m/params-0/20230926"
#dn="Z:/Verify/20231025"
#dn="Z:/view/output/finalJuly23-Sep23-BXZ-PQ-trailing-Corrected-FX/EURUSD/1m/params-0/20230703"
#dn="/lnarayan/dev/output/finalJuly23-Sep23-BXZ-PQ-trailing-Corrected-FX/EURUSD/1m/params-0/20230703"

# Browse level 1 folder and populate level 2 dropdown
@app.callback(
    [Output("file_dropdown_base", "options"), Output("file_dropdown_base", "value")],
    [Input("file_dropdown_lvl0", "value")],
)
def update_graph(value):
    print("level 0 input", value)
    level_2_folders = glob.glob(f"{value}/*")
    print(level_2_folders)
    return level_2_folders, level_2_folders[0]


# Browse level 1 folder and populate level 2 dropdown
@app.callback(
    [Output("file_dropdown_lvl3", "options"), Output("file_dropdown_lvl3", "value")],
    [Input("file_dropdown_base", "value")],
)
def update_graph(value):
    print("level 3 input ", value)
    level_2_folders = glob.glob(f"{value}/*")
    if len(level_2_folders) < 1:
        return level_2_folders, ""
    return level_2_folders, level_2_folders[0]

# Browse level 1 folder and populate level 2 dropdown
@app.callback(
    [Output("file_dropdown_lvl4", "options"), Output("file_dropdown_lvl4", "value")],
    [Input("file_dropdown_lvl3", "value")],
)
def update_graph(value):
    print("level 4 input ", value)
    level_2_folders = glob.glob(f"{value}/*")
    if len(level_2_folders) < 1:
        return level_2_folders, ""
    return level_2_folders, level_2_folders[0]

# Browse level 1 folder and populate level 2 dropdown
@app.callback(
    [Output("file_dropdown_lvl5", "options"), Output("file_dropdown_lvl5", "value")],
    [Input("file_dropdown_lvl4", "value")],
)
def update_graph(value):
    print("level 5 input ", value)
    level_2_folders = glob.glob(f"{value}/*")
    level_2_folders = [""] + level_2_folders
    if len(level_2_folders) < 1:
        return level_2_folders, ""
    return level_2_folders, level_2_folders[0]


#fileCandles=os.path.join(dn, "candlesZ3.txt")
def ReadCandles(dn):
    print("calling readcandles", dn)
    if dn == "":
        return None, None
    df = pd.DataFrame()
    cfs = glob.glob(dn + "/candles*")
    print('candle files', cfs)
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




app.layout = html.Div([
    html.H4('DASH PLOT'),
    html.H4("Select Position:"),
    #dcc.Textarea()
    dcc.Dropdown(id='file_dropdown_lvl0',options=ALL_FILES, value=ALL_FILES[0]),
    dcc.Dropdown(id='file_dropdown_base'),
    dcc.Dropdown(id='file_dropdown_lvl3'),
    dcc.Dropdown(id='file_dropdown_lvl4'),
    dcc.Dropdown(id='file_dropdown_lvl5'),
dcc.Dropdown(id="dd_pos",
                 style={'width': "40%"}
                 ),
    dcc.Loading(
        id="loading-2",
        children=[
    html.H4("Select Last:"),
    dcc.Dropdown(id="dd_last",
                 options=[{"label": x, "value": x} for x in range(1, 10)],
                 value=3,
                 multi=False,
                 style={'width': "40%"}
                 ),
    html.Div(id='textbox'),
    dcc.Graph(id="graph"),],
        type="circle",
    )


])


@app.callback([Output("dd_pos", "options"), Output("dd_pos", "value")], [Input("file_dropdown_lvl5", "value")])
def PopulatePositions(dn):
    if dn == "":
        return [], None
    optionsList = []
    print("clearing options for ", dn)

    positions = ReadPositions(dn)
    positions.sort_values(by='TimeStamp', inplace=True)

    print("populated positions", positions)

    for ts in positions.TimeStamp:
        dict  ={}
        dict["label"]=ts.strftime("%m/%d/%Y %H:%M:%S")
        dict["value"] = ts.strftime("%m/%d/%Y %H:%M:%S")
        optionsList.append(dict)
    return optionsList, optionsList[0]


@app.callback(
    [Output("textbox", "children"),Output("graph", "figure")],
    [Input("dd_pos", "value"),
    Input("dd_last", "value"),
    Input('file_dropdown_lvl5', 'value')],
)

def display_(dd_pos_val, dd_last_val, dn):

    #if dd_pos_val is None:
    #   return None, None

    print("display called with", dd_pos_val, dd_last_val, dn)

    if dn == "":
        return "", px.line()

    dn = dn.replace('\\', '/')
    positions = ReadPositions(dn)
    positions.sort_values(by='TimeStamp', inplace=True)

    if type(dd_pos_val) == dict:
        dd_pos_val = dd_pos_val['value']

    #last=1
    #Test()
    tbox=""
    last = int(dd_last_val)
    offset=0
    print("VAL dd_pos: ", dd_pos_val)
    print("VAL dd_last_val: ", dd_last_val)
    print(type(dd_pos_val))

    # values =  dd_pos_val.values()
    # print(list(values)[0])
    # dd_pos_val = list(values)[0]



    if not positions.empty:
        print("First Position ", positions.TimeStamp[0])
        print("Last Position ", positions.TimeStamp.values[-1])
        range2 =  pd.date_range(start=positions.TimeStamp[0], end=positions.TimeStamp.values[-1],freq='1D')
        print('range2',range)
        dfCandles = pd.DataFrame()
        all4 = pd.DataFrame()
        for date in range2:
            new_tail = date.strftime('%Y%m%d')
            dn = re.sub(r'/[^/]+$', '/' + new_tail, dn)
            print(dn)
            all,cdls =ReadCandles(dn)
            all4 = pd.concat([all4, all], ignore_index=False)
            dfCandles = pd.concat([dfCandles, cdls], ignore_index=False)



    print("calling Spline with ", all4.shape, last,offset, dd_pos_val)

    dataGlobal,retTS,retLen =  SplineMultiEx(all4,last,offset, dd_pos_val)

    print("returned timestamp and length", retTS, retLen)
    dfCandlesFiltered = dfCandles[dfCandles['D'] <= pd.to_datetime(dd_pos_val)]

    #fig2 = go.Figure([go.Bar(x=dfCandles['D'], y=dfCandles['INTV'],marker_color="darkgreen")])

    # Create figure with secondary y-axis
    fig = make_subplots(shared_xaxes=True,rows=2, cols=1,specs=[[{"secondary_y": True}],[{"secondary_y": False}]])
    if (len(dataGlobal) == 0):
        print("Nothing to display. Return Empty figure. Possibly no spline data for position date {} and offset {}".format(dd_pos_val,offset))
        tbox = "No Data to display. Possibly no spline data for position date {} and offset {}.".format(dd_pos_val,offset)
        return tbox,fig
    names = sorted(
        mcolors.CSS4_COLORS, key=lambda c: tuple(mcolors.rgb_to_hsv(mcolors.to_rgb(c))))
    splinecolor = names.index('gold')
    derivcolor = names.index('blue')

    for i in range(last):
        if (len(dataGlobal.columns) < (3+(i*4))):
            print("last value {} exceeds data available. Displaying {} curve".format(last, i))
            tbox = "last value {} exceeds data available. Displaying {} curve".format(last,i)
            break;

        data0 = dataGlobal.iloc[:, (i*4)]
        data1 = dataGlobal.iloc[:, 1+(i*4)]
        data2 = dataGlobal.iloc[:, 2+(i*4)]

        #UN
        # get date of spline
        minsize=100
        colDate=dataGlobal.columns[i*4]
        t = pd.date_range(end=colDate, periods=int(minsize), freq="5min")
        mask = (dfCandles['D'] >= t[0]) & (dfCandles['D'] <= t[minsize -1])
        dfCandlesFiltered2 = dfCandles.loc[mask]

        data0 = data0.to_numpy()
        data1 = data1.to_numpy()
        data2 = data2.to_numpy()

        data0 = np.append([np.nan]*i, data0)
        data1 = np.append([np.nan]*i, data1)
        data2 = np.append([np.nan]*i, data2)

        data0 = np.append(data0, [np.nan]*(last - i - 1))
        data1 = np.append(data1, [np.nan]*(last - i - 1))
        data2 = np.append(data2, [np.nan]*(last - i - 1))

        tmpList=[]
        tmpList.append(data0)
        tmpList.append(data1)
        tmpList.append(data2)
        #tmpdf=pd.DataFrame(tmpList)
        #f, s= dd_pos_val.split()
        #tmpdf.to_csv("C:/BH/Maya/output/tmpdf_" + s.replace(":","_") + ".csv")

        # Add traces
        fig.add_trace(
            go.Scatter(x=t, y=data0,
            name=colDate.strftime("%m/%d/%Y %H:%M:%S"),mode='markers'), secondary_y=False,row=1, col=1
        )

        fig.add_trace(
            go.Scatter(x=t, y=data1, name="Spline data" + str(i),marker_color=mcolors.CSS4_COLORS[names[splinecolor+i]]),
            secondary_y=False,row=1, col=1
        )

        fig.add_trace(
            go.Scatter(x=t, y=data2, name="Derivative data" + str(i),marker_color=mcolors.CSS4_COLORS[names[derivcolor+i]]),
            secondary_y=True,row=1, col=1
        )

    fig.add_trace(
        go.Bar(x=dfCandlesFiltered2['D'], y=dfCandlesFiltered2['INTV'], marker_color="darkgreen",showlegend=False), row=2, col=1
    )
    buckets = [0] * len(t)
    fig.add_trace(
        go.Scatter(x=t, y=buckets,
                   name="0 AXIS", marker_color="red"), secondary_y=True,row=1, col=1
    )

    # Add figure title
    #fig.update_layout(title_text="Spline & Derivative Chart")
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



    return tbox,fig


if __name__ == '__main__':
    #Test()
    app.run_server(debug=False,port=5556, host='0.0.0.0')
