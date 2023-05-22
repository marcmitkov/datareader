import numpy as np
from scipy.stats import norm
import matplotlib.pyplot as plt
#import seaborn as sns
import math
import pandas as pd
import os
from math import sqrt, expm1
from functools import partial
import time
import argparse
from pandas.tseries.holiday import USFederalHolidayCalendar
from pandas.tseries.offsets import CustomBusinessDay

def _percent_format(n):
    return '{:{width}.{prec}f} %'.format(n, width=5, prec=2)


def _double_format(n):
    return '{:{width}.{prec}f}'.format(n, width=5, prec=2)

def _percent_format(n):
    return '{:{width}.{prec}f} %'.format(n, width=5, prec=2)

def _to_simple_ret_str(logret):
    return _double_format(_to_simple_ret(logret))

def _to_simple_ret(logret):
    return expm1(logret)


def _sharpe(df):
    aa = df.agg(['mean','std'])
    return sqrt(252) * aa['lret'][0] / aa['lret'][1]


def _updown_months(df):
    m = df.groupby(pd.Grouper(freq='M'))['lret'].sum()
    n = float(len(m[m.values > 0]))
    d = float(len(m[m.values < 0]))
    r=12
    if d != 0:
        r=n/d
    return r


def _updown_month_rets(df):
    m = df.groupby(pd.Grouper(freq='M'))['lret'].sum()
    up_ret = m[m.values > 0].mean() * 100
    down_ret = m[m.values < 0].mean() * 100
    return _double_format(up_ret) + " / " + _double_format(down_ret) + " %"


def _winloss_days(df):
    t = float(len(df))
    w = float(len(df[df['lret'] > 0]))
    l = float(len(df[df['lret'] < 0]))
    f = t - (w + l)
    return _double_format(100 * w / t) + " / " + \
           _double_format(100 * l / t) + \
           " [" + _double_format(100 * f / t) + " ]"


def _risk_of_ruin(df):
    m = df.agg('mean').lret
    s = df.agg('std').lret
    r = sqrt(m ** 2 + s ** 2)
    return ((2 / (1 + (m / r))) - 1) ** (s / r)


# defining sharpe as something annualised
def max_dd(r):
    return np.max(r.expanding().sum().expanding().max() - r.expanding().sum())

def _draw_table1(df, axis, dirname):
    colLabels = ('', '')
    rowLabels = ('Start Date - End Date',
                 'Total Return since inception',
                 'Average Annual ROR',
                 'Winning / Losing Months',
                 'Winning Month Gain / Losing Month Loss',
                 'Sharpe Ratio',
                 'Risk of Ruin',
                 'Return to Drawdown (Calmar)',
                 '% Winning / Losing Days [Flat Days]')
    data = [[pd.to_datetime(df.head(1).index.values[0]).strftime('%Y-%m-%d') + " - " + pd.to_datetime(
        df.tail(1).index.values[0]).strftime('%Y-%m-%d')],
            [_percent_format((df.agg('sum').lret) * 100)],
            [_percent_format(_to_simple_ret(252 * df.agg('mean').lret) * 100)],
            [_double_format(_updown_months(df))],
            [_updown_month_rets(df)],
            [_double_format(_sharpe(df))],
            [_double_format(_risk_of_ruin(df))],
            [_double_format(252 * df.agg('mean').lret / max_dd(df.lret))],
            [_winloss_days(df)]]
    # print('Mytable:')
    # print(data)
    # Return and Sharpe Ratio since inception
    RetAndSharpeList = []
    totReturnStr = '{:{width}.{prec}f}'.format((df.agg('sum').lret * 100), width=5, prec=2)
    # RetAndSharpeList.append([data[1][0],data[5][0],dirname])
    RetAndSharpeList.append([float(totReturnStr), float(data[5][0]), dirname])
    RetAndSharpeDF = pd.DataFrame(RetAndSharpeList, columns=['TotalReturn', 'SharpeRatio', 'pname'])
    # pd.DataFrame(lst, columns=cols)
    axis.axis('off')
    # axis.axis('tight')
    # axis.get_frame_on()
    # plt.subplots_adjust(bottom=0.1, right=0.8, top=2.9)
    axis.set_title('Performance Analysis')
    # Styles for rows and cells
    colorsrow = []
    colorscell = []
    for x in range(len(data)):
        tmpList = []
        if x % 2 == 0:
            tmpList.append("#D0DDAF")
            colorsrow.append("#D0DDAF")
        else:
            tmpList.append("#E6E9E9")
            colorsrow.append("#E6E9E9")
        colorscell.append(tmpList)

    # colorscell = [["#D0DDAF"],["#E6E9E9"],["#D0DDAF"],["#E6E9E9"],["#D0DDAF"],["#E6E9E9"],["#D0DDAF"],["#E6E9E9"],["#D0DDAF"]]
    # colorsrow = ["#D0DDAF", "#E6E9E9", "#D0DDAF", "#E6E9E9", "#D0DDAF", "#E6E9E9", "#D0DDAF", "#E6E9E9", "#D0DDAF"]
    the_table = axis.table(cellText=data,
                           rowLabels=rowLabels,
                           colLabels=None,
                           loc='center right',
                           cellColours=colorscell,
                           rowColours=colorsrow,
                           # bbox=[0.3, -0.6, 0.5, 1.5])
                           # bbox=[0.3, -0.2, 0.5, 1.5])
                           bbox=[0.25, -0.2, 0.7, 1.1])
    '''the_table._cells[(0, 0)].set_facecolor("#56b5fd")
    the_table._cells[(2, 0)].set_facecolor("#1ac3f5")'''
    table_props = the_table.properties()
    # table_cells = table_props['children']
    # for cell in table_cells: cell.set_width(0.5)
    return RetAndSharpeDF
def best_21(series):
    r = series.rolling(21, min_periods=21).sum()
    return _percent_format(_to_simple_ret(r.max()) * 100)


def worst_21(series):
    r = series.rolling(21, min_periods=21).sum()
    return _percent_format(_to_simple_ret(r.min()) * 100)



def ann_return(series):
    a = _to_simple_ret(np.sum(series)) * 100
    return _percent_format(a)


def ann_sharpe(series):
    a=0
    if np.std(series) != 0:
        a = sqrt(252) * (np.mean(series) / np.std(series))
    return _double_format(a)



def _draw_year_analysis(df, axis, dirname):
    rowLabels = ('Annual return',
                 'Best 21 days',
                 'Worst 21 days',
                 '1 year sharpe')
    g = df.groupby(pd.Grouper(freq='A'))
    v = g.agg([np.mean, np.std, ann_return, best_21, worst_21, ann_sharpe])
    v['lret', 'sharpe'] = sqrt(252) * (v['lret', 'mean'] / v['lret', 'std'])

    # Create Annual Return Sharpe dataframe
    retSharpeDF = v['lret'][['ann_return', 'ann_sharpe']]
    retSharpeDF['pval'] = dirname
    ###
    colLabels = list(map(lambda x: x.year, v['lret'].index.tolist()))
    # deprecated
    # data = v['lret'][['ann_return', 'best_21', 'worst_21', 'ann_sharpe']].T.as_matrix()
    data = v['lret'][['ann_return', 'best_21', 'worst_21', 'ann_sharpe']].T.values

    # print('YA:')
    # print(colLabels)
    # print(data)
    axis.axis('off')
    # axis.axis('tight')
    axis.set_title('Annual Return')
    colLabelsTemp = colLabels
    ############
    colorscell = []
    colorsrow = []
    for i in range(len(rowLabels)):
        tmpList = []
        for x in range(len(colLabels)):
            if x % 2 == 0:
                tmpList.append("#D0DDAF")
            else:
                tmpList.append("#E6E9E9")
        colorscell.append(tmpList)
        if i % 2 == 0:
            colorsrow.append("#D0DDAF")
        else:
            colorsrow.append("#E6E9E9")
    colorscolumn = colorscell[0]
    ###############

    # axis.text(0.5, 1.5,'Annual Returns',fontsize=12)
    # axis.set_title('Year Analysis')
    # colorscell = [["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]]
    # colorsrow = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]
    # colorscolumn = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]#plt.cm.BuPu(np.linspace(0,0.5, len(colLabels)))
    '''colLabelsList =[]
    for label in colLabels:
        print(label)
        colLabelsList.append(label)'''

    the_table = axis.table(cellText=data,
                           rowLabels=rowLabels,
                           colLabels=colLabels,
                           cellColours=colorscell,
                           colColours=colorscolumn,
                           rowColours=colorsrow,
                           loc='left',
                           bbox=[0.0, -0.0, 1.0, 0.8])
    # bbox=[0.0, -0.2, 1.0, 1.1])
    # bbox=[0.0, -0.1, 1.0, 1.5])
    table_props = the_table.properties()
    table_cells = table_props['children']

    return retSharpeDF
def _n_day_analysis(n, df):
    r = df.rolling(n, min_periods=n).sum()
    best = _to_simple_ret(r.max().lret) * 100
    worst = _to_simple_ret(r.min().lret) * 100
    avg = _to_simple_ret(r.mean().lret) * 100
    curr = _to_simple_ret(r.tail(1).lret.values[0]) * 100
    return [_percent_format(best), _percent_format(worst),
            _percent_format(avg), _percent_format(curr)]

def _draw_day_analysis_table(df, axis):
    days = [1, 5, 21, 63, 126, 252]
    rowLabels = list(map(lambda x: str(x) + " days", days))
    colLabels = ['Best', 'Worst', 'Avg', 'Curr']
    f = partial(_n_day_analysis, df=df)
    data = list(map(f, days))
    # print('DAY')
    # print(data)
    axis.axis('off')
    axis.axis('tight')
    axis.set_title('Time analysis')

    # set styles for rows, cols and cells
    colorscell = []
    colorsrow = []
    for i in range(len(rowLabels)):
        tmpList = []
        for x in range(len(colLabels)):
            if x % 2 == 0:
                tmpList.append("#D0DDAF")
            else:
                tmpList.append("#E6E9E9")
        colorscell.append(tmpList)
        if i % 2 == 0:
            colorsrow.append("#D0DDAF")
        else:
            colorsrow.append("#E6E9E9")
    colorscolumn = colorscell[0]
    # colorscell = [["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]]
    # colorsrow = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]
    # colorscolumn = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]

    # owLabels = getColumnsList(rowLabels)
    the_table = axis.table(cellText=data,
                           rowLabels=rowLabels,
                           colLabels=colLabels,
                           cellColours=colorscell,
                           rowColours=colorsrow,
                           colColours=colorscolumn,
                           loc='center',
                           bbox=[0.0, -0.2, 0.5, 1.0])
    # bbox=[0.0, -0.2, 0.5, 1.5])
    # bbox=[0.0, -0.2, 0.5, 1.5])

def drawMonthlyReturns(md,mode,baseOutDir):
    f, axarr = plt.subplots(1)
    axis=  axarr
    rowLabels = list(md.keys()) #list(map(lambda x: str(x) + " days", days))
    colLabels = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'June', 'July','Aug', 'Sep','Oct', 'Nov' ,'Dec','Total']
    colwidthList= [0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2]
    data=[]
    for k,v in md.items():
        v1=v.multiply(other=100)


        v2=round(v1,2).tolist()
        if len(v2) < 12:
            numNa = 12 - len(v2)
            for x in range (numNa):
                v2.append(np.NaN)
        v2.append(round(v1.sum(skipna=True),2))
        data.append(v2)
        #data.append(round(v2,2).tolist())

        #data.append(tmp)



    #data = list(map(f, days))
    # print('DAY')
    # print(data)
    axis.axis('off')
    axis.axis('tight')
    axis.set_title('Monthly Returns')

    # set styles for rows, cols and cells
    colorscell = []
    colorsrow = []
    for i in range(len(rowLabels)):
        tmpList = []
        for x in range(len(colLabels)):
            if x % 2 == 0:
                tmpList.append("#D0DDAF")
            else:
                tmpList.append("#E6E9E9")
        colorscell.append(tmpList)
        if i % 2 == 0:
            colorsrow.append("#D0DDAF")
        else:
            colorsrow.append("#E6E9E9")
    colorscolumn = colorscell[0]
    if len(md) ==1:
        bboxVal= [0.0, -0.2, 0.9, 0.5]
    else:
        bboxVal = [0.0, -0.2, 0.9, 1.0]

    the_table = axis.table(cellText=data,
                           rowLabels=rowLabels,
                           colLabels=colLabels,
                           cellColours=colorscell,
                           rowColours=colorsrow,
                           colColours=colorscolumn,
                           colWidths=colwidthList,
                           loc='center',
                           bbox=bboxVal
                           #bbox=[0.0, -0.2, 0.9, 1.0]
                            )
    #the_table.scale(1.5,1.5)
    #plt.subplots_adjust(hspace=0.6,left=0.1, bottom=0.5,top=0.9)
    #the_table.auto_set_font_size(False)

    #the_table.set_fontsize(12)
    #the_table.scale(2,2)
    #the_table.auto_set_column_width(col=list(range))

    if mode == 'q':
        figurePath = os.path.join(baseOutDir, 'monthlyRetTable.png')
        plt.savefig(figurePath, dpi=300, bbox_inches="tight")
    else:
        plt.show()


def _draw_year_analysis(df, axis, dirname):
    rowLabels = ('Annual return',
                 'Best 21 days',
                 'Worst 21 days',
                 '1 year sharpe')
    g = df.groupby(pd.Grouper(freq='A'))
    v = g.agg([np.mean, np.std, ann_return, best_21, worst_21, ann_sharpe])
    v['lret', 'sharpe'] = sqrt(252) * (v['lret', 'mean'] / v['lret', 'std'])

    # Create Annual Return Sharpe dataframe
    retSharpeDF = v['lret'][['ann_return', 'ann_sharpe']]
    retSharpeDF['pval'] = dirname
    ###
    colLabels = list(map(lambda x: x.year, v['lret'].index.tolist()))
    # deprecated
    # data = v['lret'][['ann_return', 'best_21', 'worst_21', 'ann_sharpe']].T.as_matrix()
    data = v['lret'][['ann_return', 'best_21', 'worst_21', 'ann_sharpe']].T.values

    # print('YA:')
    # print(colLabels)
    # print(data)
    axis.axis('off')
    # axis.axis('tight')
    axis.set_title('Annual Return')
    colLabelsTemp = colLabels
    ############
    colorscell = []
    colorsrow = []
    for i in range(len(rowLabels)):
        tmpList = []
        for x in range(len(colLabels)):
            if x % 2 == 0:
                tmpList.append("#D0DDAF")
            else:
                tmpList.append("#E6E9E9")
        colorscell.append(tmpList)
        if i % 2 == 0:
            colorsrow.append("#D0DDAF")
        else:
            colorsrow.append("#E6E9E9")
    colorscolumn = colorscell[0]
    ###############

    # axis.text(0.5, 1.5,'Annual Returns',fontsize=12)
    # axis.set_title('Year Analysis')
    # colorscell = [["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"],["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]]
    # colorsrow = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]
    # colorscolumn = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]#plt.cm.BuPu(np.linspace(0,0.5, len(colLabels)))
    '''colLabelsList =[]
    for label in colLabels:
        print(label)
        colLabelsList.append(label)'''

    the_table = axis.table(cellText=data,
                           rowLabels=rowLabels,
                           colLabels=colLabels,
                           cellColours=colorscell,
                           colColours=colorscolumn,
                           rowColours=colorsrow,
                           loc='left',
                           bbox=[0.0, -0.0, 1.0, 0.8])
    # bbox=[0.0, -0.2, 1.0, 1.1])
    # bbox=[0.0, -0.1, 1.0, 1.5])
    table_props = the_table.properties()
    table_cells = table_props['children']

    return retSharpeDF

def depth_dd(series):
    return -100 * _to_simple_ret(np.max(series))


def start_dd(series):
    return series.head(1).index.date


def end_dd(series):
    return series.tail(1).index.date


def length_dd(series):
    return len(series)

def _draw_drawdowns_table(df, axis):
    df['expanding_sum'] = df['lret'].expanding().sum()
    df['expanding_max'] = df['expanding_sum'].expanding().max()
    df['drawdowns'] = df['expanding_max'] - df['expanding_sum']
    g = df.drawdowns[df['drawdowns'] > 0].groupby(df['expanding_max'])
    dds = g.agg([depth_dd, start_dd, end_dd, length_dd])
    print('dds:', dds)
    dds = dds.sort_values(by='depth_dd')
    print('dds2:', dds)
    dds['depth_dd'] = dds['depth_dd'].apply(_percent_format)
    # deprecated
    # data = dds.head(6).as_matrix()
    data = dds.head(6).values

    print('data:', len(data))
    print('Draw')
    print(data)
    colLabels = ('Depth', 'Start', 'End', 'Length (days)')
    axis.axis('off')
    axis.axis('tight')
    axis.set_title('Time Analysis                                                                Drawdown Report')

    colorscell = []
    for i in range(len(data)):
        tmpList = []
        for x in range(4):
            if x % 2 == 0:
                tmpList.append("#D0DDAF")
            else:
                tmpList.append("#E6E9E9")
        colorscell.append(tmpList)
    colorscolumn = colorscell[0]
    # colorsrow = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]
    # colorscolumn = ["#D0DDAF","#E6E9E9", "#D0DDAF","#E6E9E9"]#plt.cm.BuPu(np.linspace(0,0.5, len(colLabels)))

    the_table = axis.table(cellText=data,
                           colLabels=colLabels,
                           cellColours=colorscell,
                           colColours=colorscolumn,
                           loc='right',
                           bbox=[0.52, -0.2, 0.5, 1.0])
    # bbox=[0.52, -0.2, 0.5, 1.5])
    # bbox=[0.52, -0.2, 0.5, 1.5])


def plot_returns(df,baseOutDir,name,mode,dirname):
    f, axarr = plt.subplots(3)
    #print(type(axarr))
    #print((axarr[0]))
    #f.patch.set_visible(False)
    retSharpeInceptionDF = _draw_table1(df, axarr[0],dirname)
    retSharpeDF = _draw_year_analysis(df, axarr[1],dirname)
    _draw_day_analysis_table(df, axarr[2])
    _draw_drawdowns_table(df, axarr[2])
    #_draw_cum_returns(df, axarr[3])
    #_draw_returns_hist(df, axarr[4])
    #f.suptitle('test title', fontsize=20)
    #plt.subplots_adjust(left=0.1, bottom=0.1,top=1.5)
    plt.subplots_adjust(hspace=0.6,left=0.1, bottom=0.1,top=0.9)
    #f.tight_layout()
    if mode == 'q':
        figurePath = os.path.join(baseOutDir, 'tables_' + name + '.png')
        plt.savefig(figurePath,dpi=300, bbox_inches = "tight")
    else:
        plt.show()

    return retSharpeDF, retSharpeInceptionDF

def _draw_returns_hist(daily_returns, axis,baseOutDir,name):
    axis.hist(daily_returns.values, bins=50, facecolor='b', alpha=0.5, ec='black')
    axis.set_ylabel('Days')
    # plt.set_xlabel('% Returns')
    axis.set_title('# Daily Returns')
    plt.rc('grid', linestyle=":", color='gray')
    plt.grid(True)

    #figurePath = os.path.join(baseOutDir, 'charts_' + name + '.png')
    #plt.savefig(figurePath)


def _draw_cum_returns(daily_returns, axis):
    #xs = daily_returns.values.cumsum()
    daily_returns = daily_returns.dropna()
    xs = daily_returns['lret'].cumsum()

    # helpful for max dd
    i = np.argmax(np.maximum.accumulate(xs) - xs)
    j = np.argmax(xs[:i])
    axis.plot(xs, linewidth=0.8, linestyle='-', c='black')
    # max dd red dots
    print(type(xs.index))
    print(xs.index)
    print(xs.index[i])
    print(type(xs.index[i]))
    axis.plot([pd.Timestamp(xs.index[i]), pd.Timestamp(xs.index[j])],[xs[i],xs[j]] , 'o', color='Red', markersize=5)

    #axis.plot([i, j], [xs.index[i], xs.index[j]], 'o', color='Red', markersize=5)
    axis.set_title('Cumulative Daily Returns')
    axis.set_ylabel('Cumulative % Returns')
    plt.rc('grid', linestyle=":", color='gray')

    plt.grid(True)

def plot_returns2(df,baseOutDir,name,mode):
    plt.clf()
    f, axarr = plt.subplots(2)
    #print(type(axarr))
    #print((axarr[0]))
    #f.patch.set_visible(False)
    '''_draw_table1(df, axarr[0])
    _draw_year_analysis(df, axarr[1])
    _draw_day_analysis_table(df, axarr[2])
    _draw_drawdowns_table(df, axarr[2])'''
    _draw_cum_returns(df, axarr[0])
    _draw_returns_hist(df['lret'], axarr[1],baseOutDir,name)
    plt.subplots_adjust(left=0.1, bottom=0.1,top=1.5)
    f.tight_layout()
    if mode == 'q':
        figurePath = os.path.join(baseOutDir, 'charts' + name + '.png')
        plt.savefig(figurePath,dpi=300, bbox_inches = "tight")
    else:
        plt.show()

def rotate(l, n):
    return l[n:] + l[:n]

def getBusinessDays(startYear,endYear):

    daysInYearDict={}

    for year in range(int(startYear), int(endYear) + 1):
        busDayList=[]
        day1 = str(year) + '-01-01'
        day2 = str(year) + '-12-31'
        us_bd = CustomBusinessDay(calendar=USFederalHolidayCalendar())

        busDayList = pd.date_range(start=day1, end=day2, freq=us_bd)
        daysInYearDict[str(year)] = busDayList
    #print(pd.date_range(start=day1, end=day2, freq=us_bd))
    return daysInYearDict


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-baseDir", "--baseDir", default='',
                        help="Base dir")
    parser.add_argument("-retSeries", "--retSeries", default='D:/MyMr/in/',
                        help="Base dir")
    parser.add_argument("-ts", "--ts", default='',
                        help="Base dir")
    parser.add_argument('-baseOut', '--baseOut', default='C:/MyMr/OutBH', help="base Out")
    parser.add_argument('-f', '--startYear', default='2008', help="start year")
    parser.add_argument('-t', '--endYear', default='2021', help="end Year")

    parser.add_argument('-c', dest='calendarDays', action='store_true')

    args = parser.parse_args()
    print(args)
    # output path
    timestr = time.strftime("%Y%m%d-%H%M%S")
    path = os.path.join(args.baseOut, timestr)
    os.makedirs(path)


    daysInYearDict = getBusinessDays(args.startYear,args.endYear)

    if args.calendarDays:
        for key, value in daysInYearDict.items():
            print(key)
            print(len(value))
            print(type(value))
            value  = pd.Series(value.strftime('%d/%m/%Y'))

            value.to_csv(os.path.join(path,str(key) + 'Date.csv'))
        exit(0)
    #numBusDays= len(busDaysList)





    numOfYears = int(args.endYear) - int(args.startYear) + 1

    if args.retSeries != '':
        print("Reading returnseries from: {}".format(args.retSeries))
        dfRetSeries= pd.read_csv(os.path.join(args.retSeries,"returnseries.csv"))
        returnSeries = dfRetSeries.to_numpy()

    else:
        annualSharpe = 2.4
        dailySharpe = annualSharpe / 16
        meanAnnualReturn = 12

        meanDailyReturn = 12 / 25600

        meanDailySTD = meanDailyReturn / dailySharpe
        returnSeries = np.zeros((numOfYears, 256))

        for i in range(returnSeries.shape[0]):
            for j in range(returnSeries.shape[1]):
                returnSeries[i, j] = meanDailyReturn + \
                                     np.random.normal() * meanDailySTD
            print("mean daily return for year  ", str(i), " is ", round(np.mean(returnSeries[i]), 6))
            print("standard daily deviation of return is ", round((np.var(returnSeries[i])) ** 0.5, 6))
            print("annualized ", np.mean(returnSeries[i]) * 25600)

        print("mean daily return for entire set is ", round(np.mean(returnSeries), 6))
        print("standard deviation of daily return is ", round((np.var(returnSeries)) ** 0.5, 6))
        print("annualized ", np.mean(returnSeries) * 25600)

        dfRetSeries = pd.DataFrame(returnSeries)

    amtSeries = np.zeros((14, 256))
    for i in range(amtSeries.shape[0]):
        for j in range(amtSeries.shape[1]):
            if j == 0:
                amtSeries[i, j] = 1000000
            else:
                amtSeries[i, j] = amtSeries[i, j - 1] * math.exp(returnSeries[i, j - 1])
        print("year ", str(i), " last amount ", amtSeries[i][-1])

    dfAmountSeries = pd.DataFrame(amtSeries)

    dfAmountSeriesT = dfAmountSeries.T
    colList= [item for item in range(int(args.startYear ), int(args.endYear )+1 )]
    dfAmountSeriesT.columns=colList
    dfAmountSeriesT.plot(kind='line')
    #for colname, colData in dfAmountSeriesT.iteritems():
    #for colname in dfAmountSeriesT:
        #print(colname)



    # dfAmountSeries.T.plot(kind='line')
    # plt.show()
    figurePath = os.path.join(path, 'yearlyComparison.png')
    plt.savefig(figurePath, dpi=300, bbox_inches="tight")

    yearlyReturns = {}
    monthlyReturns = {}

    if args.ts != '':
        print("Reading ts from: {}".format(args.ts))
        dfts = pd.read_csv(os.path.join(args.ts),names=['ts','lret'])
        dfts.dropna(inplace=True)
        dfts['ts'] =pd.to_datetime(dfts['ts'],format='%d/%m/%Y')
        dfts = dfts.set_index('ts')
        dfts.sort_index(inplace=True)
        m = dfts.groupby(pd.Grouper(freq='M'))['lret'].sum()
        year=os.path.basename(args.ts).split('.')[0]
        monthlyReturns[str(year)] = m
        yearlyReturns[str(year)] = dfts

    else:
        for index, row in dfRetSeries.iterrows():
            year = int(args.startYear) + index
            #bdaysInYear = pd.bdate_range(str(year) + '-01-01', str(year) + '-12-31')
            bdaysInYear = daysInYearDict[str(year)]

            diff = len(bdaysInYear) - len(row)
            dfRow = row.to_frame()
            dfRow =  dfRow[:diff]
            dfRow = dfRow.set_index(bdaysInYear)
            print(dfRow)
            dfRow.rename(columns={index: 'lret'}, inplace=True)
            m = dfRow.groupby(pd.Grouper(freq='M'))['lret'].sum()
            monthlyReturns[str(year)] = m

            yearlyReturns[str(year)] = dfRow

    drawMonthlyReturns(monthlyReturns,'q',path)

    outfile = os.path.join(path, 'returnseries.csv')
    dfRetSeries.to_csv(outfile, index=False)
    outfile = os.path.join(path, 'returnseriesT.csv')
    dfRetSeries.T.to_csv(outfile, index=False)

    print(path)

    for key, val in yearlyReturns.items():
        print(key)
        print(val)
        plot_returns(val, path, key, 'q', 'params-0')
        plot_returns2(val, path, key, 'q')

    # organize as dataframe
    df = pd.DataFrame(amtSeries)
    pnlDF = df.shift(-1, axis=1) - df  # pnl for each day

    pnlDF = pnlDF.dropna(axis=1)
    print(pnlDF.describe())  # describe  by column
    print(pnlDF.apply(pd.DataFrame.describe, axis=1))

    pnlDF = pnlDF.T  # days in rows , columns are years (0..13)
    pnlDF.columns=colList

    grouped = pnlDF.groupby(pd.cut(pnlDF.index, 12))  # bucket index into 256 /12 rows == 12 month

    for name, group in grouped:
        print(name)

    final = grouped.sum().stack()

    final = final.reset_index()

    final.columns = ['index', 'columns', 'values']

    print(final)

    final.pivot_table(index='index', columns='columns', values='values').plot(kind='bar')
    figurePath = os.path.join(path, 'yearlyComparisonByMonth.png')
    plt.savefig(figurePath, dpi=300, bbox_inches="tight")
    # plt.show()

    if args.retSeries != '':
        print("Read  returnseries from: {}".format(args.retSeries))

    print("Output in : {}".format(path))

    exit


if __name__ == '__main__':
    main()