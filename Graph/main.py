# This is a sample Python script.

# Press Shift+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.
from scipy.stats import pearson3
import matplotlib.pyplot as plt
import np

import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
#import statsmodels.formula.api as sm

def SplineIT(f):
    df1 = pd.read_csv(f)
    df1=df1.transpose()
    print(df1.columns)
    print(df1)
    df1.columns=['0','2','6']
    df1.dropna().plot()
    plt.show()

def  SplineMulti(data1, data2):
    # Create some mock data
    #t = np.arange(0.01, 10.0, 0.01)
    #data1 = np.exp(t)
    #data2 = np.sin(2 * np.pi * t)
    data2 = data2.reset_index(drop=True)
    print(data2.columns)

    print(data2.dtypes)
    data2=data2.dropna()
    data2['0'] = pd.to_numeric(data2['0'])
    t = data2.iloc[:, 0].to_numpy()
    data1 = data2.iloc[:, 1].to_numpy()
    data2 = data2.iloc[:, 2].to_numpy()

    fig, ax1 = plt.subplots()

    color = 'tab:red'
    ax1.set_xlabel('time (s)')
    #ax1.set_ylabel('exp', color=color)
    ax1.set_ylabel('spline', color=color)
    ax1.plot(t, data1, color=color)
    ax1.tick_params(axis='y', labelcolor=color)

    ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis

    color = 'tab:blue'
    #ax2.set_ylabel('sin', color=color)  # we already handled the x-label with ax1
    ax2.set_ylabel('derivative', color=color)  # we already handled the x-label with ax1
    ax2.plot(t, data2, color=color)

    buckets = [0] * len(t)
    ax2.plot(t, buckets, 'C1')

    ax2.tick_params(axis='y', labelcolor=color)

    fig.tight_layout()  # otherwise the right y-label is slightly clipped
    plt.show()

def print_hi(name):
    # Use a breakpoint in the code line below to debug your script.
    print(f'Hi, {name}')  # Press Ctrl+F8 to toggle the breakpoint.


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    #print_hi('PyCharm')
    #SplineIT("D:/MyProjects/spline312.txt")
    #exit(0)
    df1 = pd.read_csv("D:/MyProjects/spline312.txt", index_col=False)
    df1=df1.transpose()
    df1 = df1.reset_index()
    print(df1.shape)
    print(df1.columns)
    df1.rename(columns=str,inplace=True)
    print(df1.columns)
    df1.rename(columns={'index': '0', '0': '1', '1': '2', '2': '3'}, inplace=True)
    print(df1.columns)

    df2 = pd.read_csv("D:/MyProjects/SplineDeriv.txt", index_col=False)
    df2 = df2.transpose()
    df2 = df2.reset_index()
    print(df2.shape)
    print(df2.columns)
    df2.rename(columns=str,inplace=True)
    df2.rename(index=str,columns={'index': '0', '0': '1', '1': '2'}, inplace=True)
    print(df2.columns)

    SplineMulti(df1, df2)

    exit(0)

    fig, ax = plt.subplots(1, 1)
    skew = 0.1
    mean, var, skew, kurt = pearson3.stats(skew, moments='mvsk')
    x = np.linspace(pearson3.ppf(0.01, skew),
                    pearson3.ppf(0.99, skew), 100)
    ax.plot(x, pearson3.pdf(x, skew),
            'r-', lw=5, alpha=0.6, label='pearson3 pdf')
    rv = pearson3(skew)
    ax.plot(x, rv.pdf(x), 'k-', lw=2, label='frozen pdf')
    vals = pearson3.ppf([0.001, 0.5, 0.999], skew)
    np.allclose([0.001, 0.5, 0.999], pearson3.cdf(vals, skew))
    r = pearson3.rvs(skew, size=1000)
    ax.hist(r, density=True, histtype='stepfilled', alpha=0.2)
    ax.legend(loc='best', frameon=False)
    plt.show()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
