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

import pandas as pd
import os
import numpy as np
import matplotlib.colors as mcolors

colnames =[ "Id", "TimeStamp", "Size", "AvgPx", "ClosePx", "CloseTimeStamp", "PnlRealized", "PnlUnRealized", "State","Remaining",  "FillPx", "FillTimeStamp", "ExitCode", "CloseRemaining", "CloseFillPx", "CloseFillTimeStamp","Strat","EntryType", "Value","Description","StopSize", "TargetSize","ExpiryDate"]
df = pd.DataFrame()

CandleColNames= ["D","T","SYBL","LSTP","LSTS","INTF","INTH","INTL","INTV","NTRD","LSTB","LSBS","LSTA","LSAS","HBID","LBID","HASK","LASK","HBSZ","LBSZ","HASZ","LASZ","HLTS","IVAM"
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
    dataGlobal = data[data.index <= pd.to_datetime(ts)]
    if (len(dataGlobal) == 0):
        print("ERROR: NO Spline data for positions date {} . ".format(ts))
        return pd.DataFrame()
        #exit(-1)
    #for char in ['/', ':']:
        #ts = ts.replace(char, "_")
    #dataGlobal.to_csv("C:/BH/Maya/output/dataglobal"+ ts.replace(":", "_") + ".csv",header=False)

    if (offset  > 0 ):
        dataGlobal = dataGlobal.drop(dataGlobal.tail(offset*4).index) #remove offset*4 rows starting from bottom
    if (len(dataGlobal) == 0):
        print("ERROR: dataframe empty with offset value {} . EXITTING PROGRAM".format(offset))
        return pd.DataFrame()

    dataGlobal = dataGlobal[-4*last:] # get 4*last rows starting from bottom

    dataGlobal = dataGlobal.transpose()

    dataGlobal = dataGlobal.dropna(axis=0)
    return  dataGlobal


def ReadPositions(dir):
    print("Reading Positions")
    posfile = os.path.join(dir, "positions.txt")
    retval = pd.read_csv(posfile, names=colnames, index_col=False, parse_dates=[1])
    #print(retval.columns)
    #print(retval.dtypes)
    return retval

optionsList =[]


#def Test():

dn = '/lnarayan/dev/output/runJune23lagFixed/AUL/1m/params-0/20230601'
dn="C:\BH\Maya\Data"

fileCandles=os.path.join(dn, "candlesM3.txt")
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
print(count)
# df.drop(df.columns[102], axis = 1, inplace=True)
#df = df.drop(df.columns[[101]], axis=1)
#print(df.head())
df = df.set_index(0)
print(df.shape)
df = df.astype(float)
print(df.head())

positions = ReadPositions(dn)
positions.sort_values(by='TimeStamp',inplace=True)

for ts in positions.TimeStamp:
    dict  ={}
    dict["label"]=ts.strftime("%m/%d/%Y %H:%M:%S")
    dict["value"] = ts.strftime("%m/%d/%Y %H:%M:%S")
    optionsList.append(dict)


app.layout = html.Div([
    html.H4('SPLINE & DERIVATIVE PLOT'),
    html.H4("Select Position:"),
    #dcc.Textarea()
    dcc.Dropdown(id="dd_pos",
                 options=optionsList,
                 value=optionsList[0]['label'],
                 multi=False,
                 style={'width': "40%"}
                 ),
    html.H4("Select Last:"),
    dcc.Dropdown(id="dd_last",
                 options=[{"label": x, "value":x} for x in range(1,10)],
                 value=3,
                 multi=False,
                 style={'width': "40%"}
                 ),
    html.Div(id='textbox'),
    dcc.Graph(id="graph"),

])


@app.callback(
    [Output("textbox", "children"),Output("graph", "figure")],
    [Input("dd_pos", "value"),
    Input("dd_last", "value")]
)

def display_(dd_pos_val, dd_last_val):
    #last=1
    #Test()
    tbox=""
    last = int(dd_last_val)
    offset=0
    print("VAL dd_pos: ", dd_pos_val)
    print("VAL dd_last_val: ", dd_last_val)

    dataGlobal =  SplineMultiEx(df,last,offset,dd_pos_val)
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
        colDate=dataGlobal.columns[i*4]
        t = pd.date_range(end=colDate, periods=int(100), freq="1min")
        mask = (dfCandles['D'] >= t[0]) & (dfCandles['D'] <= t[99])
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
    app.run_server(debug=False)
